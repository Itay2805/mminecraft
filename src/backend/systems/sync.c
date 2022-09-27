#include "sync.h"
#include "util/defs.h"

#include <backend/components/player.h>
#include <backend/components/chunk.h>
#include <backend/protocol.h>
#include <backend/world.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Chunk synchronization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void sync_chunks(ecs_iter_t* it) {
    PlayerConnection* connection = ecs_field(it, PlayerConnection, 1);
    ecs_entity_t chunk = ecs_pair_second(g_ecs, ecs_field_id(it, 2));

    // get the chunk data, if does not have it yet then exit this query
    const ChunkData* chunkData = ecs_get(g_ecs, chunk, ChunkData);
    if (chunkData == NULL) {
        return;
    }

    for (int i = 0; i < it->count; i++) {
        // send the chunk data
        send_chunk_data(connection[i].connection->fd, chunkData->chunk);

        // send the light data
        send_full_update_light(connection[i].connection->fd, chunkData->chunk);

        // add the has chunk and remove the needs chunk
        ecs_add_pair(g_ecs, it->entities[i], PlayerHasChunk, chunk);
        ecs_remove_pair(g_ecs, it->entities[i], PlayerNeedsChunk, chunk);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entity synchronization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static ecs_rule_t* m_players_in_chunk = NULL;

static int m_players_in_chunk_var = 0;

static void init_sync_entities() {
    m_players_in_chunk = ecs_rule(g_ecs, {
        .expr = "[none] (PlayerHasChunk, $Chunk), [in] PlayerConnection",
    });
    m_players_in_chunk_var = ecs_rule_find_var(m_players_in_chunk, "Chunk");
}

static void sync_entities(ecs_iter_t* it) {
    UNUSED EntityPosition* positions = ecs_field(it, EntityPosition, 1);
    UNUSED EntityRotation* rotations = ecs_field(it, EntityRotation, 2);
    EntityChunk* chunk = ecs_field(it, EntityChunk, 3);

    for (int i = 0; i < it->count; i++) {
        ecs_entity_t current = it->entities[i];

        bool need_pos = ecs_has(g_ecs, current, EntityPositionChanged);
        bool need_rot = ecs_has(g_ecs, current, EntityRotationChanged);

        // go over all the players with the same chunk as the chunk this entity is in
        ecs_iter_t sit = ecs_rule_iter(g_ecs, m_players_in_chunk);
        ecs_iter_set_var(&sit, m_players_in_chunk_var, chunk->chunk);
        while (ecs_rule_next(&sit)) {
            UNUSED PlayerConnection* connection = ecs_field(&sit, PlayerConnection, 2);
            for (int j = 0; j < sit.count; j++) {
                // don't send to self
                if (it->entities[j] == sit.entities[j]) {
                    continue;
                }

                if (need_pos && need_rot) {
                    // TODO: send Entity Position and Rotation
                } else if (need_pos) {
                    // TODO: send Entity position
                } else if (need_rot) {
                    // TODO: Send entity rotation
                }
            }
        }


        // no longer have these enabled
        ecs_enable_component(g_ecs, it->entities[i], EntityPositionChanged, false);
        ecs_enable_component(g_ecs, it->entities[i], EntityRotationChanged, false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void init_sync_systems() {
    // pushes the chunks to all the players that need chunks
    ECS_SYSTEM(g_ecs, sync_chunks, EcsOnStore,
               [in] PlayerConnection, [in] (PlayerNeedsChunk, *));

    // push the entity positions that have changed to all the
    // players near them
    init_sync_entities();
    ECS_SYSTEM(g_ecs, sync_entities, EcsOnStore,
               [in] EntityPosition, [in] EntityRotation, [in] EntityChunk,
               EntityPositionChanged || EntityRotationChanged);
}
