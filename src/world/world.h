#pragma once

#include <util/except.h>
#include <pthread.h>

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef uint16_t block_state_id_t;

#define INVALID_BLOCK_STATE_ID ((block_state_id_t)-1)

typedef struct section_palette {
    // the entries
    uint64_t entries[4];

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

    // the blocks in this chunk
    block_state_id_t blocks[16 * 16 * 16];

    // used to sync accesses to the palette and blocks
    pthread_rwlock_t lock;
} section_t;

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
 * write the compacted data
 */
uint8_t* section_write_compact(section_t* section, uint8_t* data);

typedef struct section_light {
    // the block light of the section
    uint8_t block_light[16 * 16 * 16];

    // the sky light of the section
    uint16_t sky_light[16 * 16 * 16];
} section_light_t;

typedef struct chunk {
    // how many references exist to the chunk, at zero it gets deleted
    atomic_int ref_count;

    // the block data of sections
    section_t* sections[16];

    // the light sections, used to save space on areas
    // without any blocks but that do have light data
    section_light_t* light_sections[16];
} chunk_t;

chunk_t* put_chunk(chunk_t* chunk);

void release_chunk(chunk_t* chunk);

typedef union chunk_position {
    struct {
        uint64_t x : 28;
        uint64_t y : 28;
        uint64_t z : 8;
    };
    uint64_t packed;
} chunk_position_t;
STATIC_ASSERT(sizeof(chunk_position_t) == sizeof(uint64_t));

typedef chunk_t*(*generate_chunk_t)(void* ctx, chunk_position_t position);

typedef struct world {
    // the chunks, an stb_ds map
    struct {
        chunk_position_t key;
        chunk_t* value;
    }* chunks;

    // is this a flat world?
    bool flat;

    // for generating new chunks
    void* gen_ctx;
    generate_chunk_t generate;

    // used to prevent accesses to the chunk map
    pthread_rwlock_t chunks_lock;
} world_t;

/**
 * Get the chunk at the given position, increasing its ref count
 */
chunk_t* world_get_chunk(world_t* world, chunk_position_t position);

/**
 * The overworld instance of this server
 */
extern world_t g_overworld;

/**
 * Setup the world
 */
err_t init_world();
