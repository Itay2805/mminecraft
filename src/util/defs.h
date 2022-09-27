#pragma once

#define ABS(x) (((x)<0)?(0-(x)):(x))

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
#define DIV_ROUND_DOWN(a, b) ((a) / (b) - ((a) % (b) < 0))

#define SWAP64(value) \
    ({ \
        union {       \
            __typeof__(value) __value; \
            uint64_t __bits_value; \
        } u = { .__value = value };              \
        _Static_assert(sizeof(value) == sizeof(uint64_t), "invalid SWAP64 type"); \
        u.__bits_value = __builtin_bswap64(u.__bits_value); \
        u.__value; \
    })

#define SWAP32(value) \
    ({ \
        union {       \
            __typeof__(value) __value; \
            uint32_t __bits_value; \
        } u = { .__value = value };              \
        _Static_assert(sizeof(value) == sizeof(uint32_t), "invalid SWAP32 type"); \
        u.__bits_value = __builtin_bswap32(u.__bits_value); \
        u.__value; \
    })

#define SWAP16(value) \
    ({ \
        union {       \
            __typeof__(value) __value; \
            uint16_t __bits_value; \
        } u = { .__value = value };              \
        _Static_assert(sizeof(value) == sizeof(uint16_t), "invalid SWAP16 type"); \
        u.__bits_value = __builtin_bswap16(u.__bits_value); \
        u.__value; \
    })
