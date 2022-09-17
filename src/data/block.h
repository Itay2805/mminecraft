#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct block {
    // The identification of this block
    uint32_t id;
    const char* name;

    // TODO: item

    // state id
    uint16_t min_state_id;
    uint16_t max_state_id;
    uint16_t default_state_id;

    // is this a solid block
    bool solid;

    // light relatedvalues
    bool transparent;
    uint8_t filter_light;
    uint8_t emit_light;
} block_t;

extern block_t g_air;
extern block_t g_grass_block;
extern block_t g_dirt;
extern block_t g_bedrock;
