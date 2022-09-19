#include "world.h"

#include "backend/components/entity.h"
#include "backend/components/player.h"
#include "backend/components/chunk.h"

ecs_world_t* g_ecs = NULL;

ECS_DECLARE(components);

void init_world_ecs() {
    g_ecs = ecs_init();

    // we want the id range to be only 32bit so it is easier for us to
    // use the raw ecs id as the id we send
    ecs_set_entity_range(g_ecs, 0, UINT32_MAX);

    ECS_TAG_DEFINE(g_ecs, components);

    init_entity_ecs();
    init_player_ecs();
    init_chunk_ecs();
}
