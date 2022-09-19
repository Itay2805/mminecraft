#include "chunk.h"
#include "backend/world.h"

ECS_COMPONENT_DECLARE(chunk_data_t);
ECS_COMPONENT_DECLARE(chunk_position_t);

ECS_DECLARE(chunks);

void init_chunk_ecs() {
    ECS_TAG_DEFINE(g_ecs, chunks);

    ECS_COMPONENT_DEFINE(g_ecs, chunk_data_t);
    ECS_COMPONENT_DEFINE(g_ecs, chunk_position_t);

    ecs_add_pair(g_ecs, ecs_id(chunk_position_t), EcsChildOf, components);
    ecs_struct(g_ecs, {
        .entity = ecs_id(chunk_position_t),
        .members = {
            { .name = "x", .type = ecs_id(ecs_i32_t) },
            { .name = "z", .type = ecs_id(ecs_i32_t) },
        }
    });

    ecs_add_pair(g_ecs, ecs_id(chunk_data_t), EcsChildOf, components);

}

ecs_entity_t get_ecs_chunk(chunk_position_t position) {
    // TODO: support dimensions

    char buffer[256] = { 0 };
    snprintf(buffer, sizeof(buffer), "chunks.%d_%d", position.x, position.z);
    ecs_entity_t entity = ecs_lookup_fullpath(g_ecs, buffer);

    // does not exists, get it from the world instance
    if (entity == 0) {
        entity = ecs_new_id(g_ecs);
        ecs_add_pair(g_ecs, entity, EcsChildOf, chunks);
        ecs_set(g_ecs, entity, chunk_position_t, { .x = position.x, .z = position.z });
        ecs_set_name(g_ecs, entity, buffer + sizeof("chunks.") - 1);

        chunk_t* chunk = world_get_chunk(&g_overworld, position);
        if (chunk == NULL) {
            WARN("TODO: queue chunk generation for %d.%d", position.x, position.z);
        } else {
            // add its component right away, giving away our reference
            ecs_set(g_ecs, entity, chunk_data_t, { .chunk = chunk });
        }
    }

    return entity;
}
