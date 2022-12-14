#include "chunk.h"
#include "backend/world.h"
#include "util/stb_ds.h"
#include "world/gen/generator.h"

ECS_COMPONENT_DECLARE(ChunkData);
ECS_COMPONENT_DECLARE(ChunkPosition);

ECS_DECLARE(chunks);

static ECS_DTOR(ChunkData, ptr, {
    WARN("TODO: release chunk");
});

void init_chunk_ecs() {
    ECS_TAG_DEFINE(g_ecs, chunks);

    ECS_COMPONENT_DEFINE(g_ecs, ChunkData);
    ECS_COMPONENT_DEFINE(g_ecs, ChunkPosition);

    ecs_struct(g_ecs, {
        .entity = ecs_id(ChunkPosition),
        .members = {
            { .name = "x", .type = ecs_id(ecs_i32_t) },
            { .name = "z", .type = ecs_id(ecs_i32_t) },
        }
    });

    ecs_set_hooks(g_ecs, ChunkData, {
        .dtor = ecs_dtor(ChunkData)
    });
}

static struct {
    chunk_position_t key;
    ecs_entity_t value;
}* m_chunk_entities = NULL;

static pthread_rwlock_t m_chunk_entities_lock = PTHREAD_RWLOCK_INITIALIZER;

ecs_entity_t get_ecs_chunk(chunk_position_t position) {
    // TODO: support dimensions
    ptrdiff_t temp = 0;

    // does not exists, get it from the world instance
    pthread_rwlock_rdlock(&m_chunk_entities_lock);
    int idx = hmgeti_ts(m_chunk_entities, position, temp);
    if (idx < 0) {
        pthread_rwlock_unlock(&m_chunk_entities_lock);

        ecs_entity_t entity = ecs_new_id(g_ecs);
        ecs_add_pair(g_ecs, entity, EcsChildOf, chunks);
        ecs_set(g_ecs, entity, ChunkPosition, { .x = position.x, .z = position.z });

        // give a name for debugging
        char buffer[256] = { 0 };
        snprintf(buffer, sizeof(buffer), "overworld@x=%d,z=%d", position.x, position.z);
        ecs_doc_set_name(g_ecs, entity, buffer);

        // store it
        pthread_rwlock_wrlock(&m_chunk_entities_lock);
        hmput(m_chunk_entities, position, entity);
        pthread_rwlock_unlock(&m_chunk_entities_lock);

        // store the chunk along side if needed
        chunk_t* chunk = world_get_chunk(&g_overworld, position);
        if (chunk == NULL) {
            // queue the world gen of the chunk, the next person will just get
            // the entity itself
            generator_queue(&g_overworld, position, entity);
        } else {
            // add its component right away, giving away our reference
            ecs_set(g_ecs, entity, ChunkData, { .chunk = chunk });
        }

        return entity;
    } else {
        ecs_entity_t entity = m_chunk_entities[idx].value;
        pthread_rwlock_unlock(&m_chunk_entities_lock);

        return entity;
    }
}
