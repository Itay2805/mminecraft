#include <stdlib.h>
#include <assert.h>
#include <netinet/in.h>
#include "world.h"
#include "util/stb_ds.h"
#include "data/block.h"
#include "world/gen/generator.h"
#include "world/gen/flat/flat_world_gen.h"
#include "util/defs.h"
#include "frontend/protocol.h"

void section_add_block_state_id(section_t* section, block_state_id_t block) {
    // this is not using a palette
    if (section->palette == NULL) {
        return;
    }

    section_palette_t* palette = section->palette;

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
        free(section->palette);
        section->palette = NULL;
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
    section->block_count = 0;
    for (int i = 0; i < 16 * 16 * 16; i++) {
        section_add_block_state_id(section, section->blocks[i]);

        // air is hardcoded to be zero for ease of use
        if (section->blocks[i] != 0) {
            section->block_count++;
        }
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
        section->block_count = 0;
        for (int i = 0; i < 16 * 16 * 16; i++) {
            section_add_block_state_id(section, section->blocks[i]);

            if (section->blocks[i] != g_air.default_state_id) {
                section->block_count++;
            }
        }
    }
}

void section_calc_block_count(section_t* section) {
    section->block_count = 0;
    for (int i = 0; i < 16 * 16 * 16; i++) {
        if (section->blocks[i] != g_air.default_state_id) {
            section->block_count++;
        }
    }
}


static uint8_t* write_varint(uint8_t* data, int32_t value) {
    uint32_t uvalue = (uint32_t)value;

    while (true) {
        if ((uvalue & ~VARINT_SEGMENT_BITS) == 0) {
            *arraddnptr(data, 1) = uvalue;
            return data;
        }

        *arraddnptr(data, 1) = ((uvalue & VARINT_SEGMENT_BITS) | VARINT_CONTINUE_BIT);
        uvalue >>= 7;
    }
}

uint8_t* section_write_compact(section_t* section, uint8_t* data) {
    // calculate the bits per block
    int64_t bits = 15;
    if (section->palette != NULL) {
        bits = section->palette->bits < 4 ? 4 : section->palette->bits;
        assert(bits <= 8);
    }

    // calculate the rest
    const int64_t total_blocks = 16 * 16 * 16;
    const int64_t blocks_per_long = 64 / bits; // check how many blocks fit in a word
    const int64_t longs = DIV_ROUND_UP(total_blocks, blocks_per_long);

    // write the number of longs we need for this
    data = write_varint(data, (int32_t)longs);

    // write the compact data
    for (int64_t long_num = 0; long_num < longs; long_num++) {
        // compact into a single word
        int64_t word = 0;
        for (int64_t block_num = 0; block_num < blocks_per_long; block_num++) {
            int64_t block_idx = long_num * blocks_per_long + block_num;
            if (block_idx >= 16 * 16 * 16)
                break;

            // get the id, doing a lookup if needed
            int64_t id = section->blocks[block_idx];
            if (section->palette != NULL) {
                id = hmget(section->palette->lookup, id);
            }
            word |= id << (block_num * bits);
        }

        // write it to the array
        *(int64_t*)arraddnptr(data, sizeof(int64_t)) = SWAP64(word);
    }

    return data;
}

chunk_t* put_chunk(chunk_t* chunk) {
//    atomic_fetch_add(&chunk->ref_count, 1);
    return chunk;
}

void release_chunk(chunk_t* chunk) {
//    if (atomic_fetch_sub(&chunk->ref_count, 1) == 1) {
        // we can free this chunk
        for (int i = 0; i < 16; i++) {
            free(chunk->light_sections[i]);
            if (chunk->sections[i] != NULL) {
                hmfree(chunk->sections[i]->palette->lookup);
                free(chunk->sections[i]->palette);
                free(chunk->sections[i]);
            }
        }
//    }
}

chunk_t* world_get_chunk(world_t* world, chunk_position_t position) {
    pthread_rwlock_rdlock(&world->chunks_lock);
    int idx = hmgeti(world->chunks, position);
    chunk_t* chunk = idx >= 0 ? world->chunks[idx].value : NULL;
    pthread_rwlock_unlock(&world->chunks_lock);
    return chunk;
}

void world_set_chunk(world_t* world, chunk_t* chunk) {
    pthread_rwlock_wrlock(&world->chunks_lock);
    hmput(world->chunks, chunk->position, chunk);
    pthread_rwlock_unlock(&world->chunks_lock);
}

world_t g_overworld = {
    .chunks_lock = PTHREAD_RWLOCK_INITIALIZER,
};


err_t init_world() {
    err_t err = NO_ERROR;

    // we are just gonna do overworld
    g_overworld.flat = true;
    g_overworld.generate = flat_world_gen;

    // setup the world gen
    CHECK_AND_RETHROW(init_generator());

cleanup:
    return err;
}
