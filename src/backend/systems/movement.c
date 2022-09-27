#include "movement.h"
#include "backend/util/view_distance.h"

#include <backend/components/player.h>
#include <backend/components/chunk.h>
#include <backend/protocol.h>
#include <backend/world.h>
#include <util/defs.h>

#include <math.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Player moved
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void player_commit_position(ecs_iter_t* it) {
    const PlayerPosition* player_pos = ecs_field(it, PlayerPosition, 1);
    EntityPosition* entity_pos = ecs_field(it, EntityPosition, 2);
    const PlayerViewDistance* player_vd = ecs_field(it, PlayerViewDistance, 3);
    const PlayerConnection* connection = ecs_field(it, PlayerConnection, 4);

    for (int i = 0; i < it->count; i++) {
        ecs_entity_t current = it->entities[i];

        chunk_view_distance_t old = chunk_view_distance_from_position(entity_pos->x, entity_pos->z, player_vd->view_distance);
        chunk_view_distance_t new = chunk_view_distance_from_position(player_pos->x, player_pos->z, player_vd->view_distance);

        // figure if we need any new chunks, if we need mark that the player needs them
        if (!chunk_view_distance_same(new, old)) {

            // the position has updated, tell the client we have a new view position
            send_update_view_position(connection->connection->fd, (chunk_position_t){
                .x = (int32_t)(entity_pos[i].x / 16),
                .z = (int32_t)(entity_pos[i].z / 16)
            });

            // TODO: more correct way to have both, we only move little amount of chunks
            //       so this will work for now

            int32_t old_cx = DIV_ROUND_DOWN((int32_t)entity_pos->x, 16);
            int32_t old_cz = DIV_ROUND_DOWN((int32_t)entity_pos->z, 16);

            int32_t new_cx = DIV_ROUND_DOWN((int32_t)player_pos->x, 16);
            int32_t new_cz = DIV_ROUND_DOWN((int32_t)player_pos->z, 16);

            // add new ones
            int32_t start_x = MIN(old.x - old.size, new.x - old.size);
            int32_t start_z = MIN(old.z - old.size, new.z - old.size);
            int32_t end_x = MAX(old.x + old.size, new.x + old.size);
            int32_t end_z = MAX(old.z + old.size, new.z + old.size);
            TRACE("map:");
            for (int32_t x = start_x; x <= end_x; x++) {
                for (int32_t z = start_z; z <= end_z; z++) {
                    bool in_old = chunk_in_view_distance(old, x, z);
                    bool in_new = chunk_in_view_distance(new, x, z);

                    if (old_cx == x && old_cz == z) {
                        printf(" O");
                    } else if (new_cx == x && new_cz == z) {
                        printf(" N");
                    } else if (in_old && in_new) {
                        printf(" I");
                    } else if (in_old) {
                        printf(" o");
                    } else if (in_new) {
                        printf(" n");
                    } else {
                        printf(" +");
                    }

                    if (!in_old && !in_new) {
                        continue;
                    }

                    chunk_position_t pos = (chunk_position_t){ .x = x, .z = z };
                    ecs_entity_t chunk = get_ecs_chunk(pos);

                    if (in_new && in_old) {
                        // overlapping, the client can keep it
                    } else if (in_new) {
                        // new region, need to send it
                        ecs_add_pair(g_ecs, current, PlayerNeedsChunk, chunk);
                    } else if (in_old) {
                        // old region, need to unload it
                        send_unload_chunk(connection->connection->fd, pos);
                        ecs_remove_pair(g_ecs, current, PlayerHasChunk, chunk);
                    }
                }

                printf("\n");
            }
        }

        // calculate it is not too fast
        double delta_x = player_pos[i].x - entity_pos[i].x;
        double delta_y = player_pos[i].y - entity_pos[i].y;
        double delta_z = player_pos[i].z - entity_pos[i].z;
        double delta = sqrt((delta_x * delta_x) + (delta_y * delta_y) + (delta_z * delta_z));

        // TODO: elytra
        if (!isfinite(delta) || delta >= 100) {
            // player moved too fast, disconnect it
            ecs_set(g_ecs, current, DisconnectPlayer, {
                .reason = isfinite(delta) ? "You moved too quickly :( (Hacking?)" : "Illegal position"
            });
        } else if (delta != 0) {
            // copy it and mark that the position has changed
            entity_pos[i] = player_pos[i];
            ecs_enable_component(g_ecs, current, EntityPositionChanged, true);
            ecs_enable_component(g_ecs, current, PlayerPosition, false);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void init_movement_systems() {
    // the player has moved, update it
    ECS_SYSTEM(g_ecs, player_commit_position, EcsPreStore,
               [in] PlayerPosition,
               [inout] EntityPosition,
               [in] PlayerViewDistance,
               [in] PlayerConnection);
}
