#pragma once

#include <stdint.h>

/**
 * Seed the random number generator
 */
void xoshiro256starstar_seed();

/**
 * Get a 64bit random number
 */
uint64_t xoshiro256starstar_next();
