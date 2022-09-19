#pragma once

#include <stdint.h>
#include <uuid/uuid.h>

#include <backend/world.h>

/**
 * Represents a base entity component
 */
typedef struct entity_position {
    double x, y, z;
} entity_position_t;
extern ECS_COMPONENT_DECLARE(entity_position_t);

typedef struct entity_rotation {
    uint8_t pitch;
    uint8_t yaw;
} entity_rotation_t;
extern ECS_COMPONENT_DECLARE(entity_rotation_t);

typedef struct entity_velocity {
    short x, y, z;
} entity_velocity_t;
extern ECS_COMPONENT_DECLARE(entity_velocity_t);

typedef struct entity_id {
    uuid_t uuid;
} entity_id_t;
extern ECS_COMPONENT_DECLARE(entity_id_t);

extern ECS_DECLARE(entity_chunk);

void init_entity_ecs();
