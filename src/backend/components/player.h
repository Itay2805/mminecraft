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

void init_player_ecs();
