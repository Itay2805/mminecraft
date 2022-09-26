#include "entity.h"

#include <backend/world.h>

ECS_COMPONENT_DECLARE(EntityPosition);
ECS_COMPONENT_DECLARE(EntityRotation);
ECS_COMPONENT_DECLARE(EntityUuid);

void init_entity_ecs() {
    ECS_COMPONENT_DEFINE(g_ecs, EntityPosition);
    ECS_COMPONENT_DEFINE(g_ecs, EntityRotation);
    ECS_COMPONENT_DEFINE(g_ecs, EntityUuid);

    // TODO: debug only
    ecs_struct(g_ecs, {
        .entity = ecs_id(EntityPosition),
        .members = {
                { .name = "x", .type = ecs_id(ecs_f32_t) },
                { .name = "y", .type = ecs_id(ecs_f32_t) },
                { .name = "z", .type = ecs_id(ecs_f32_t) },
        }
    });
    ecs_struct(g_ecs, {
        .entity = ecs_id(EntityRotation),
        .members = {
                { .name = "pitch", .type = ecs_id(ecs_f32_t) },
                { .name = "yaw", .type = ecs_id(ecs_f32_t) },
        }
    });
}
