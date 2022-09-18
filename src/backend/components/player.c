#include "player.h"

#include <backend/world.h>

ECS_COMPONENT_DECLARE(player_t);
ECS_COMPONENT_DECLARE(game_mode_t);

void init_player_ecs() {
    ECS_COMPONENT_DEFINE(g_ecs, player_t);
    ECS_COMPONENT_DEFINE(g_ecs, game_mode_t);

    ecs_enum(g_ecs, {
        .entity = ecs_id(game_mode_t),
        .constants = {
            { .name = "Survival", .value = GAME_MODE_SURVIVAL },
            { .name = "Creative", .value = GAME_MODE_CREATIVE },
            { .name = "Adventure", .value = GAME_MODE_ADVENTURE },
            { .name = "Spectator", .value = GAME_MODE_SPECTATOR },
        }
    });
}
