#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct chunk_view_distance {
    int32_t x;
    int32_t z;
    int32_t size;
} chunk_view_distance_t;

chunk_view_distance_t chunk_view_distance_from_position(double x, double z, int view_distance);

bool chunk_in_view_distance(chunk_view_distance_t distance, int32_t x, int32_t z);

bool chunk_view_distance_same(chunk_view_distance_t a, chunk_view_distance_t b);

#define ITERATE_CHUNKS(target, _x, _z) \
    for (int32_t _x = target.x - target.size; _x <= target.x + target.size; _x++) \
        for (int32_t _z = target.z - target.size; _z <= target.z + target.size; _z++) \
            if (chunk_in_view_distance(target, _x, _z))
