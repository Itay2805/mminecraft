#include "player.h"

#include <backend/world.h>

ECS_COMPONENT_DECLARE(player_t);
ECS_COMPONENT_DECLARE(game_mode_t);
ECS_COMPONENT_DECLARE(player_view_distance_t);

ECS_DECLARE(players);

void init_player_ecs() {
    ECS_COMPONENT_DEFINE(g_ecs, player_t);
    ECS_COMPONENT_DEFINE(g_ecs, game_mode_t);
    ECS_COMPONENT_DEFINE(g_ecs, player_view_distance_t);

    ECS_TAG_DEFINE(g_ecs, players);

    ecs_add_pair(g_ecs, ecs_id(player_t), EcsChildOf, components);
    ecs_add_pair(g_ecs, ecs_id(game_mode_t), EcsChildOf, components);
    ecs_enum(g_ecs, {
        .entity = ecs_id(game_mode_t),
        .constants = {
            { .name = "Survival", .value = GAME_MODE_SURVIVAL },
            { .name = "Creative", .value = GAME_MODE_CREATIVE },
            { .name = "Adventure", .value = GAME_MODE_ADVENTURE },
            { .name = "Spectator", .value = GAME_MODE_SPECTATOR },
        }
    });
    ecs_add_pair(g_ecs, ecs_id(player_view_distance_t), EcsChildOf, components);
    ecs_struct(g_ecs, {
        .entity = ecs_id(player_view_distance_t),
        .members = {
            { .name = "view_distance", .type = ecs_id(ecs_u8_t) },
        }
    });
}
