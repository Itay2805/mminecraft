#pragma once

#include <flecs/flecs.h>
#include "net/protocol.h"

typedef struct player_name {
    // the name of the player
    char name[MAX_STRING_SIZE(16) + 1];
} player_name_t;
extern ECS_COMPONENT_DECLARE(player_name_t);

void init_player_ecs();
