#include "protocol.h"

#include <stdint.h>

int32_t varint_size(int32_t value) {
    if (value <= ((1 << (7 * 1)) - 1)) {
        return 1;
    } else if (value <= ((1 << (7 * 2)) - 1)) {
        return 2;
    } else if (value <= ((1 << (7 * 3)) - 1)) {
        return 3;
    } else if (value <= ((1 << (7 * 4)) - 1)) {
        return 4;
    } else if (value <= UINT32_MAX) {
        return 5;
    } else {
        return -1;
    }
}

int32_t varint_write(int32_t value, uint8_t* buffer) {
    uint32_t uvalue = (uint32_t)value;

    int32_t size = 0;
    while (true) {
        if ((uvalue & ~VARINT_SEGMENT_BITS) == 0) {
            *buffer = uvalue;
            return ++size;
        }

        *buffer++ = ((uvalue & VARINT_SEGMENT_BITS) | VARINT_CONTINUE_BIT);
        size++;
        uvalue >>= 7;
    }
}
