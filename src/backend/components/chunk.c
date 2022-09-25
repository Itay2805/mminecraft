#include "chunk.h"
#include "backend/world.h"
#include "util/stb_ds.h"
#include "world/gen/generator.h"

ECS_COMPONENT_DECLARE(chunk_data_t);
ECS_COMPONENT_DECLARE(chunk_position_t);

ECS_DECLARE(chunks);

static ECS_DTOR(chunk_data_t, ptr, {
    WARN("TODO: release chunk");
});

void init_chunk_ecs() {
    ECS_TAG_DEFINE(g_ecs, chunks);

    ECS_COMPONENT_DEFINE(g_ecs, chunk_data_t);
    ECS_COMPONENT_DEFINE(g_ecs, chunk_position_t);

    ecs_struct(g_ecs, {
        .entity = ecs_id(chunk_position_t),
        .members = {
            { .name = "x", .type = ecs_id(ecs_i32_t) },
            { .name = "z", .type = ecs_id(ecs_i32_t) },
        }
    });

    ecs_set_hooks(g_ecs, chunk_data_t, {
        .dtor = ecs_dtor(chunk_data_t)
    });
}

static struct {
    chunk_position_t key;
    ecs_entity_t value;
}* m_chunk_entities = NULL;

ecs_entity_t get_ecs_chunk(chunk_position_t position) {
    // TODO: support dimensions

    // does not exists, get it from the world instance
    int idx = hmgeti(m_chunk_entities, position);
    if (idx < 0) {
        ecs_entity_t entity = ecs_new_id(g_ecs);
        ecs_add_pair(g_ecs, entity, EcsChildOf, chunks);
        ecs_set(g_ecs, entity, chunk_position_t, { .x = position.x, .z = position.z });

        // give a name for debugging
        char buffer[256] = { 0 };
        snprintf(buffer, sizeof(buffer), "overworld_%d_%d", position.x, position.z);
        ecs_set_name(g_ecs, entity, buffer);

        // store it
        hmput(m_chunk_entities, position, entity);

        // store the chunk along side if needed
        chunk_t* chunk = world_get_chunk(&g_overworld, position);
        if (chunk == NULL) {
            // queue the world gen of the chunk, the next person will just get
            // the entity itself
            generator_queue(&g_overworld, position, entity);
        } else {
            // add its component right away, giving away our reference
            ecs_set(g_ecs, entity, chunk_data_t, { .chunk = chunk });
        }

        return entity;
    } else {
        return m_chunk_entities[idx].value;
    }
}
