#pragma once

#define MIN(a, b) \
    ({ \
        __typeof__(a) __a = a; \
        __typeof__(b) __b = b; \
        __a > __b ? __b : __a; \
    })

#define MAX(a, b) \
    ({ \
        __typeof__(a) __a = a; \
        __typeof__(b) __b = b; \
        __a < __b ? __b : __a; \
    })

#define PACKED __attribute__((packed))
#define UNUSED __attribute__((unused))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define SWAP64(value) \
    ({ \
        __typeof__(value) __value = value; \
        _Static_assert(sizeof(value) == 8, "invalid SWAP64 type"); \
        uint64_t __beval = __builtin_bswap64(*(uint64_t*)&__value); \
        *(__typeof__(value)*)&__beval; \
    })

#define SWAP32(value) \
    ({ \
        __typeof__(value) __value = value; \
        _Static_assert(sizeof(value) == 4, "invalid SWAP32 type"); \
        uint32_t __beval = __builtin_bswap32(*(uint32_t*)&__value); \
        *(__typeof__(value)*)&__beval; \
    })

#define SWAP16(value) \
    ({ \
        __typeof__(value) __value = value; \
        _Static_assert(sizeof(value) == 2, "invalid SWAP16 type"); \
        uint16_t __beval = __builtin_bswap16(*(uint16_t*)&__value); \
        *(__typeof__(value)*)&__beval; \
    })
