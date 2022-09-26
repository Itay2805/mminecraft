#include "player.h"

#include <backend/world.h>
#include <stdlib.h>

ECS_COMPONENT_DECLARE(PlayerConnection);
ECS_COMPONENT_DECLARE(PlayerGameMode);
ECS_COMPONENT_DECLARE(PlayerViewDistance);
ECS_COMPONENT_DECLARE(PlayerTeleportRequest);

ECS_DECLARE(PlayerHasChunk);
ECS_DECLARE(PlayerNeedsChunk);

ECS_DTOR(PlayerConnection, ptr, {
    SAFE_FREE(ptr->name);

    release_connection(ptr->connection);
    ptr->connection = NULL;
})

void init_player_ecs() {
    ECS_COMPONENT_DEFINE(g_ecs, PlayerConnection);
    ECS_COMPONENT_DEFINE(g_ecs, PlayerGameMode);
    ECS_COMPONENT_DEFINE(g_ecs, PlayerViewDistance);
    ECS_COMPONENT_DEFINE(g_ecs, PlayerTeleportRequest);

    ECS_TAG_DEFINE(g_ecs, PlayerHasChunk);
    ECS_TAG_DEFINE(g_ecs, PlayerNeedsChunk);

    ecs_set_hooks(g_ecs, PlayerConnection, {
        .dtor = ecs_dtor(PlayerConnection)
    });

    ecs_struct(g_ecs, {
        .entity = ecs_id(PlayerConnection),
        .members = {
            { .name = "name", .type = ecs_id(ecs_string_t) },
            { .name = "connection", .type = ecs_id(ecs_uptr_t) },
        }
    });

    ecs_enum(g_ecs, {
        .entity = ecs_id(PlayerGameMode),
        .constants = {
            { .name = "Survival", .value = GAME_MODE_SURVIVAL },
            { .name = "Creative", .value = GAME_MODE_CREATIVE },
            { .name = "Adventure", .value = GAME_MODE_ADVENTURE },
            { .name = "Spectator", .value = GAME_MODE_SPECTATOR },
        }
    });

    ecs_struct(g_ecs, {
        .entity = ecs_id(PlayerViewDistance),
        .members = {
            { .name = "view_distance", .type = ecs_id(ecs_u8_t) },
        }
    });

    ecs_struct(g_ecs, {
        .entity = ecs_id(PlayerTeleportRequest),
        .members = {
            { .name = "id", .type = ecs_id(ecs_i32_t) },
        }
    });
}
