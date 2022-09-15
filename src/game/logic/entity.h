#include <stdint.h>

#include <math/vec3.h>
#include <util/uuid.h>

typedef enum entity_kind {
    ENTITY_KIND_MAX,
} entity_kind_t;

typedef struct entity {
    entity_kind_t kind;

    uuid_t uuid;
    uint32_t id;

    uint8_t pitch, yaw;
    vec3_t position;
    vec3_t velocity;
} entity_t;
