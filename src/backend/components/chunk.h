#pragma once

#include <flecs/flecs.h>

#include <world/world.h>

typedef struct {
    // the chunk data
    chunk_t* chunk;
} ChunkData;
extern ECS_COMPONENT_DECLARE(ChunkData);

typedef chunk_position_t ChunkPosition;
extern ECS_COMPONENT_DECLARE(ChunkPosition);

void init_chunk_ecs();

ecs_entity_t get_ecs_chunk(ChunkPosition position);
