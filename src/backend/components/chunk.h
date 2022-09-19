#pragma once

#include <flecs/flecs.h>

#include <world/world.h>

typedef struct chunk_data {
    // the chunk data
    chunk_t* chunk;
} chunk_data_t;
extern ECS_COMPONENT_DECLARE(chunk_data_t);

extern ECS_COMPONENT_DECLARE(chunk_position_t);

extern ECS_DECLARE(chunks);

void init_chunk_ecs();

ecs_entity_t get_ecs_chunk(chunk_position_t position);
