#include "view_distance.h"
#include "util/defs.h"

chunk_view_distance_t chunk_view_distance_from_position(double x, double z, int view_distance) {
    return (chunk_view_distance_t){
        .x = DIV_ROUND_DOWN((int32_t)x, 16),
        .z = DIV_ROUND_DOWN((int32_t)z, 16),
        .size = view_distance
    };
}

bool chunk_in_view_distance(chunk_view_distance_t distance, int32_t x, int32_t z) {
    return ((x - distance.x) * (x - distance.x) +
            (z - distance.z) * (z - distance.z) <= distance.size * distance.size);
}

bool chunk_view_distance_same(chunk_view_distance_t a, chunk_view_distance_t b) {
    return a.x == b.x && a.z == b.z && a.size == b.size;
}
