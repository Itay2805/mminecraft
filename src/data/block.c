#include "block.h"

block_t g_air = {
    .id = 0,
    .name = "air",
    .min_state_id = 0,
    .max_state_id = 0,
    .default_state_id = 0,
    .solid = false,
    .transparent = true,
    .filter_light = 0,
    .emit_light = 0,
};

block_t g_grass_block = {
    .id = 8,
    .name = "grass_block",
    .min_state_id = 8,
    .max_state_id = 9,
    .default_state_id = 9,
    .solid = true,
    .transparent = false,
    .filter_light = 15,
    .emit_light = 0,
};

block_t g_dirt = {
    .id = 9,
    .name = "dirt",
    .min_state_id = 10,
    .max_state_id = 10,
    .default_state_id = 10,
    .solid = true,
    .transparent = false,
    .filter_light = 15,
    .emit_light = 0,
};

block_t g_bedrock = {
    .id = 25,
    .name = "bedrock",
    .min_state_id = 33,
    .max_state_id = 33,
    .default_state_id = 33,
    .solid = true,
    .transparent = false,
    .filter_light = 15,
    .emit_light = 0,
};
