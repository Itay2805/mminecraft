#pragma once

#include <util/except.h>
#include <pthread.h>

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef uint16_t block_state_id_t;

#define INVALID_BLOCK_STATE_ID ((block_state_id_t)-1)

typedef struct chunk_position {
    int32_t x;
    int32_t z;
} chunk_position_t;

typedef struct section_palette {
    // the entries
    uint64_t entries[256];

    // reverse lookup
    struct {
        block_state_id_t key;
        uint8_t value;
    }* lookup;

    // the amount of bits taken, max of 8
    uint8_t bits;
} section_palette_t;

typedef struct section {
    // the palette for this chunk, once filled
    // it is going to be deleted and the section
    // will use the direct format, we can later on
    // try and process it again and see if a palette
    // is not needed
    section_palette_t* palette;

    // the amount of non-air blocks in this chunk
    uint16_t block_count;

    // the blocks in this chunk
    block_state_id_t blocks[16 * 16 * 16];

    // used to sync accesses to the palette and blocks
    pthread_rwlock_t lock;
} section_t;

/**
 * Used to index the blocks array
 */
#define BLOCK_IDX(x, y, z) ((x) + ((z) * 16) + ((y) * 16 * 16))

/**
 * Makes sure the block exists in the palette
 */
void section_add_block_state_id(section_t* section, block_state_id_t block);

/**
 * Tries to compact a section's palette, nop if there isn't one
 */
void section_compact_palette(section_t* section);

/**
 * Tries to create a palette for the chunk from the block data in it,
 * tries to compact it if one already exists
 */
void section_create_palette(section_t* section);

/**
 * Unlike the palette this has to always be kept and
 * updated properly, if you don't create a palette at
 * least try and use this to calculate the block count
 */
void section_calc_block_count(section_t* section);

/**
 * write the compacted data
 */
uint8_t* section_write_compact(section_t* section, uint8_t* data);

typedef struct section_light {
    // the block light of the section, nibble array
    uint8_t light[(16 * 16 * 16) / 2];
} section_light_t;

typedef struct chunk {
    // position of this chunk
    chunk_position_t position;

    // the block data of sections
    section_t* sections[16];

    // the light sections, used to save space on areas
    // without any blocks but that do have light data
    section_light_t* block_light_sections[18];
    section_light_t* sky_light_sections[18];
} chunk_t;

chunk_t* put_chunk(chunk_t* chunk);

void release_chunk(chunk_t* chunk);

typedef chunk_t*(*generate_chunk_t)(uint64_t seed, chunk_position_t position);

typedef struct world {
    // the chunks, an stb_ds map
    struct {
        chunk_position_t key;
        chunk_t* value;
    }* chunks;

    // is this a flat world?
    bool flat;

    // for generating new chunks
    uint64_t seed;
    generate_chunk_t generate;

    // used to prevent accesses to the chunk map
    pthread_rwlock_t chunks_lock;
} world_t;

/**
 * Get the chunk at the given position, increasing its ref count
 */
chunk_t* world_get_chunk(world_t* world, chunk_position_t position);

/**
 * Set a new chunk in the world, hope that it is not generated already
 */
void world_set_chunk(world_t* world, chunk_t* chunk);

/**
 * The overworld instance of this server
 */
extern world_t g_overworld;

/**
 * Setup the world
 */
err_t init_world();
