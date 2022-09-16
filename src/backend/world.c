#include "world.h"

#include "backend/components/entity.h"
#include "backend/components/player.h"

ecs_world_t* g_ecs = NULL;

void init_world_ecs() {
    g_ecs = ecs_init();

    init_entity_ecs();
    init_player_ecs();
}
