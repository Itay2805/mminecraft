#pragma once

#include "util/except.h"
#include "world/world.h"
#include "flecs/flecs.h"

/**
 * Initialize the chunk generation
 */
err_t init_generator();

/**
 * Queue the chunk generation for the given position
 */
void generator_queue(world_t* world, chunk_position_t position, ecs_entity_t entity);
