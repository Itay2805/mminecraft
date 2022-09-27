#include "movement.h"

#include <backend/components/player.h>
#include <backend/components/chunk.h>
#include <backend/protocol.h>
#include <backend/world.h>
#include <math.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Player moved
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void player_commit_position(ecs_iter_t* it) {
    PlayerPosition* player_pos = ecs_field(it, PlayerPosition, 1);
    EntityPosition* entity_pos = ecs_field(it, EntityPosition, 2);

    for (int i = 0; i < it->count; i++) {
        // calculate it is not too fast
        double delta_x = fabs(player_pos[i].x - entity_pos[i].x);
        double delta_y = fabs(player_pos[i].y - entity_pos[i].y);
        double delta_z = fabs(player_pos[i].z - entity_pos[i].z);
        double delta = (delta_x * delta_x) + (delta_y * delta_y) + (delta_z * delta_z);

        // TODO: elytra
        if (!isfinite(delta) || delta >= 100) {
            // player moved too fast, disconnect it
            ecs_set(g_ecs, it->entities[i], DisconnectPlayer, {
                .reason = isfinite(delta) ? "You moved too quickly :( (Hacking?)" : "Illegal position"
            });
        } else if (delta != 0) {
            // copy it and mark that the position has changed
            entity_pos[i] = player_pos[i];
            ecs_enable_component(g_ecs, it->entities[i], EntityPositionChanged, true);
            ecs_enable_component(g_ecs, it->entities[i], PlayerPosition, false);
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
               [inout] EntityPosition);
}
