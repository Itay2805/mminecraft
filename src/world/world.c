#include <stdlib.h>
#include <assert.h>
#include "world.h"
#include "util/stb_ds.h"
#include "data/block.h"

void section_add_block_state_id(section_t* chunk, block_state_id_t block) {
    // this is not using a palette
    if (chunk->palette == NULL) {
        return;
    }

    section_palette_t* palette = chunk->palette;

    // find an empty index
    int empty_index = -1;
    int empty_bit = -1;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 64; j++) {
            if ((palette->entries[i] & (1 << j)) == 0) {
                empty_index = i;
                empty_bit = j;
                break;
            }
        }
    }

    if (empty_index >= 0) {
        // populate it
        palette->entries[empty_index] = 1 << empty_bit;
        hmput(palette->lookup, block, empty_bit + empty_index * 64);

        // increase the bits per block if needed
        if (empty_index >= (1 << palette->bits)) {
            palette->bits++;
            assert(empty_index < (1 << palette->bits));
        }
    } else {
        // out of space, free the palette we are no longer using it
        hmfree(palette->lookup);
        free(chunk->palette);
        chunk->palette = NULL;
    }
}

void section_compact_palette(section_t* section) {
    if (section->palette == NULL) {
        return;
    }

    // set everything to no values
    section->palette->bits = 0;
    hmfree(section->palette->lookup);
    memset(section->palette->entries, 0x00, sizeof(section->palette->entries));

    // now add it again
    for (int i = 0; i < 16 * 16 * 16; i++) {
        section_add_block_state_id(section, section->blocks[i]);
    }
}

void section_create_palette(section_t* section) {
    if (section->palette != NULL) {
        section_compact_palette(section);
    } else {
        // allocate the new palette
        // TODO: check return value of malloc
        section->palette = malloc(sizeof(section_palette_t));
        section->palette->lookup = NULL;
        section->palette->bits = 0;
        memset(section->palette->entries, 0x00, sizeof(section->palette->entries));

        // add all the block ids in the section
        for (int i = 0; i < 16 * 16 * 16; i++) {
            section_add_block_state_id(section, section->blocks[i]);
        }
    }
}

uint8_t* section_write_compact(section_t* section, uint8_t* data) {
    if (section->palette == NULL) {
        // we are using the direct format, this has 4 blocks per word,
        // and each of them is 15 bits
        const size_t bits = 15;
        const size_t total_blocks = 16 * 16 * 16;
        const size_t blocks_per_word = 64 / bits;
        const size_t iterations = total_blocks / blocks_per_word;

        for (int i = 0; i < iterations; i++) {
            // compact into a single word
            uint64_t word = 0;
            for (int j = 0; j < blocks_per_word; j++) {
                word |= section->blocks[i + j] << (i * 15);
            }

            // write it to the array
            *(uint64_t*)arraddnptr(data, 8) = word;
        }
    } else {
        // the min amount of bits per block is 4
        const size_t bits = section->palette->bits < 4 ? 4 : section->palette->bits;
        const size_t total_blocks = 16 * 16 * 16;
        const size_t blocks_per_word = 64 / bits; // check how many blocks fit in a word
        const size_t iterations = total_blocks / blocks_per_word;

        for (int i = 0; i < iterations; i++) {
            // compact into a single word
            uint64_t word = 0;
            for (int j = 0; j < blocks_per_word; j++) {
                block_state_id_t id = section->blocks[i + j];
                uint8_t pid = hmget(section->palette->lookup, id);
                word |= pid << (i * bits);
            }

            // write it to the array
            *(uint64_t*)arraddnptr(data, 8) = word;
        }
    }

    return data;
}

chunk_t* put_chunk(chunk_t* chunk) {
    atomic_fetch_add(&chunk->ref_count, 1);
    return chunk;
}

void release_chunk(chunk_t* chunk) {
    if (atomic_fetch_sub(&chunk->ref_count, 1) == 1) {
        // we can free this chunk
        for (int i = 0; i < 16; i++) {
            free(chunk->light_sections[i]);
            if (chunk->sections[i] != NULL) {
                hmfree(chunk->sections[i]->palette->lookup);
                free(chunk->sections[i]->palette);
                free(chunk->sections[i]);
            }
        }
    }
}

chunk_t* world_get_chunk(world_t* world, chunk_position_t position) {
    pthread_rwlock_rdlock(&world->chunks_lock);
    int idx = hmgeti(world->chunks, position);
    chunk_t* chunk = idx >= 0 ? world->chunks[idx].value : NULL;
    pthread_rwlock_unlock(&world->chunks_lock);
    return chunk;
}

world_t g_overworld = {
    .chunks_lock = PTHREAD_RWLOCK_INITIALIZER,
};

static chunk_t* generate_flat_chunk(void* ctx, chunk_position_t position) {
    // create the chunk level
    chunk_t* chunk = malloc(sizeof(chunk_t));
    memset(chunk, 0, sizeof(*chunk));

    // for flat we only need the lowest most thing
    section_t* section = malloc(sizeof(section_t));
    pthread_rwlock_init(&section->lock, NULL);
    chunk->sections[0] = section;

    // pre-create the palette
    section->palette = malloc(sizeof(section_palette_t));
    memset(section->palette, 0, sizeof(*section->palette));
    section_add_block_state_id(section, g_bedrock.default_state_id);
    section_add_block_state_id(section, g_dirt.default_state_id);
    section_add_block_state_id(section, g_grass_block.default_state_id);

    // set bedrock
    for (int x = 0; x < 16 * 16; x++) {
        section->blocks[x] = g_bedrock.default_state_id;
    }

    // set the dirt
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 16 * 16; x++) {
            section->blocks[x + (y + 1) * 16 * 16] = g_dirt.default_state_id;
        }
    }

    // set the grass
    for (int x = 0; x < 16 * 16; x++) {
        section->blocks[x + 3 * 16 * 16] = g_grass_block.default_state_id;
    }

    return chunk;
}

err_t init_world() {
    err_t err = NO_ERROR;

    // we are just gonna do overworld
    g_overworld.flat = true;
    g_overworld.generate = generate_flat_chunk;

cleanup:
    return err;
}
