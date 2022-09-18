#pragma once

#include <flecs/flecs.h>
#include "net/protocol.h"
#include "net/connection.h"

typedef struct player {
    // the name of the player
    char name[MAX_STRING_SIZE(16) + 1];

    // the connection of this player
    connection_t* connection;
} player_t;
extern ECS_COMPONENT_DECLARE(player_t);

typedef enum game_mode {
    GAME_MODE_SURVIVAL,
    GAME_MODE_CREATIVE,
    GAME_MODE_ADVENTURE,
    GAME_MODE_SPECTATOR,
} game_mode_t;
extern ECS_COMPONENT_DECLARE(game_mode_t);

void init_player_ecs();
