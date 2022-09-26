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

void init_entity_ecs();
