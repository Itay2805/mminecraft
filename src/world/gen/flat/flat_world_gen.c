#include <malloc.h>
#include "flat_world_gen.h"
#include "data/block.h"


chunk_t* flat_world_gen(uint64_t seed, chunk_position_t position) {
    // create the chunk level
    chunk_t* chunk = calloc(1, sizeof(chunk_t));
    memset(chunk, 0, sizeof(*chunk));

    // for flat we only need the lowest most thing
    section_t* section = calloc(1, sizeof(section_t));
    pthread_rwlock_init(&section->lock, NULL);
    chunk->sections[0] = section;

    // pre-create the palette
//    section->palette = calloc(1, sizeof(section_palette_t));
//    section_add_block_state_id(section, g_bedrock.default_state_id);
//    section_add_block_state_id(section, g_dirt.default_state_id);
//    section_add_block_state_id(section, g_grass_block.default_state_id);

    // set bedrock
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            section->blocks[BLOCK_IDX(x, 0, z)] = g_bedrock.default_state_id;
            section->block_count++;
        }
    }

    // set the dirt
    for (int y = 1; y <= 2; y++) {
        for (int z = 0; z < 16; z++) {
            for (int x = 0; x < 16; x++) {
                section->blocks[BLOCK_IDX(x, y, z)] = g_dirt.default_state_id;
                section->block_count++;
            }
        }
    }

    // set the grass
    for (int z = 0; z < 16; z++) {
        for (int x = 0; x < 16; x++) {
            section->blocks[BLOCK_IDX(x, 3, z)] = g_grass_block.default_state_id;
            section->block_count++;
        }
    }

    return chunk;
}
