#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "flecs/flecs.h"

typedef struct join_game_1 {
    int entity_id;
    bool is_hardcore;
    uint8_t gamemode;
    uint8_t previous_gamemode;
    uint8_t world_count; // varint but we only have small values
} __attribute__((packed)) join_game_1_t;

typedef struct join_game_2 {
    uint64_t hashed_seed;
    uint8_t _ignored;
    uint8_t view_distance;
    bool reduced_debug_info;
    bool enable_respawn_screen;
    bool is_debug;
    bool is_flat;
} __attribute__((packed)) join_game_2_t;

void send_join_game(int fd, ecs_entity_t entity_id);
