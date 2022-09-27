#pragma once

#include <stdint.h>
#include <uuid/uuid.h>

#include <backend/world.h>

typedef struct {
    double x, y, z;
} EntityPosition;
extern ECS_COMPONENT_DECLARE(EntityPosition);

typedef struct {
    float yaw;
    float pitch;
} EntityRotation;
extern ECS_COMPONENT_DECLARE(EntityRotation);

typedef struct {
    uuid_t uuid;
} EntityUuid;
extern ECS_COMPONENT_DECLARE(EntityUuid);

typedef struct {
    ecs_entity_t chunk;
} EntityChunk;
extern ECS_COMPONENT_DECLARE(EntityChunk);

extern ECS_DECLARE(EntityPositionChanged);
extern ECS_DECLARE(EntityRotationChanged);

void init_entity_ecs();
