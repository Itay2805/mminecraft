#include "player.h"

#include <backend/world.h>
#include <stdlib.h>

ECS_COMPONENT_DECLARE(player_t);
ECS_COMPONENT_DECLARE(game_mode_t);
ECS_COMPONENT_DECLARE(player_view_distance_t);
ECS_COMPONENT_DECLARE(player_teleport_t);

ECS_DECLARE(players);

ECS_DTOR(player_t, ptr, {
    SAFE_FREE(ptr->name);

    release_connection(ptr->connection);
    ptr->connection = NULL;
})

void init_player_ecs() {
    ECS_COMPONENT_DEFINE(g_ecs, player_t);
    ECS_COMPONENT_DEFINE(g_ecs, game_mode_t);
    ECS_COMPONENT_DEFINE(g_ecs, player_view_distance_t);
    ECS_COMPONENT_DEFINE(g_ecs, player_teleport_t);

    ecs_set_hooks(g_ecs, player_t, {
        .dtor = ecs_dtor(player_t)
    });

    ECS_TAG_DEFINE(g_ecs, players);

    ecs_struct(g_ecs, {
        .entity = ecs_id(player_t),
        .members = {
            { .name = "name", .type = ecs_id(ecs_string_t) },
            { .name = "connection", .type = ecs_id(ecs_uptr_t) },
        }
    });

    ecs_enum(g_ecs, {
        .entity = ecs_id(game_mode_t),
        .constants = {
            { .name = "Survival", .value = GAME_MODE_SURVIVAL },
            { .name = "Creative", .value = GAME_MODE_CREATIVE },
            { .name = "Adventure", .value = GAME_MODE_ADVENTURE },
            { .name = "Spectator", .value = GAME_MODE_SPECTATOR },
        }
    });

    ecs_struct(g_ecs, {
        .entity = ecs_id(player_view_distance_t),
        .members = {
            { .name = "view_distance", .type = ecs_id(ecs_u8_t) },
        }
    });

    ecs_struct(g_ecs, {
        .entity = ecs_id(player_teleport_t),
        .members = {
            { .name = "id", .type = ecs_id(ecs_i32_t) },
        }
    });
}
