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
