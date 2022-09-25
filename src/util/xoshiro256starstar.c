#include <time.h>
#include "xoshiro256starstar.h"


static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}


static uint64_t s[4];

uint64_t xoshiro256starstar_next(void) {
    const uint64_t result = rotl(s[1] * 5, 7) * 9;

    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;

    s[3] = rotl(s[3], 45);

    return result;
}

uint64_t splitmix64_next(uint64_t* x) {
    uint64_t z = (*x += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

void xoshiro256starstar_seed() {
    // use the current time and split it
    // to 256bits using splitmix as long as
    // all the values are zero
    do {
        uint64_t seed = time(NULL);
        s[0] = splitmix64_next(&seed);
        s[1] = splitmix64_next(&seed);
        s[2] = splitmix64_next(&seed);
        s[3] = splitmix64_next(&seed);
    } while (s[0] == 0 && s[1] == 0 && s[2] == 0 && s[3] == 0);
}


