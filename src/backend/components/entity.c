#include "entity.h"

#include <backend/world.h>

ECS_COMPONENT_DECLARE(entity_position_t);
ECS_COMPONENT_DECLARE(entity_rotation_t);
ECS_COMPONENT_DECLARE(entity_velocity_t);
ECS_COMPONENT_DECLARE(entity_id_t);

void init_entity_ecs() {
    ECS_COMPONENT_DEFINE(g_ecs, entity_position_t);
    ECS_COMPONENT_DEFINE(g_ecs, entity_rotation_t);
    ECS_COMPONENT_DEFINE(g_ecs, entity_velocity_t);
    ECS_COMPONENT_DEFINE(g_ecs, entity_id_t);

    // TODO: debug only
    ecs_struct(g_ecs, {
        .entity = ecs_id(entity_position_t),
        .members = {
                { .name = "x", .type = ecs_id(ecs_f32_t) },
                { .name = "y", .type = ecs_id(ecs_f32_t) },
                { .name = "z", .type = ecs_id(ecs_f32_t) },
        }
    });
    ecs_struct(g_ecs, {
        .entity = ecs_id(entity_rotation_t),
        .members = {
                { .name = "pitch", .type = ecs_id(ecs_i8_t) },
                { .name = "yaw", .type = ecs_id(ecs_i8_t) },
        }
    });
    ecs_struct(g_ecs, {
        .entity = ecs_id(entity_velocity_t),
        .members = {
                { .name = "x", .type = ecs_id(ecs_i16_t) },
                { .name = "y", .type = ecs_id(ecs_i16_t) },
                { .name = "z", .type = ecs_id(ecs_i16_t) },
        }
    });
}
