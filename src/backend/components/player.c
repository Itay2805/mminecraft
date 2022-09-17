#include "player.h"

#include <backend/world.h>

ECS_COMPONENT_DECLARE(player_t);

void init_player_ecs() {
    ECS_COMPONENT_DEFINE(g_ecs, player_t);
}
