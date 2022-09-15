#pragma once

#include <stdint.h>
#include <stdbool.h>

#define VARINT_SEGMENT_BITS 0x7F
#define VARINT_CONTINUE_BIT 0x80

#define DECODE_VARINT(buffer, size) \
    ({ \
        int32_t __value = 0; \
        uint32_t __position = 0; \
        while (true) { \
            CHECK_ERROR(size >= 1, ERROR_PROTOCOL_VIOLATION); \
            uint8_t __byte = *buffer++; size--; \
            __value |= (__byte & VARINT_SEGMENT_BITS) << __position; \
            if ((__byte & VARINT_CONTINUE_BIT) == 0) \
                break; \
            __position += 7; \
            CHECK_ERROR(__position < 32, ERROR_PROTOCOL_VIOLATION); \
        } \
        __value; \
    })

/**
 * Write the varint to the storage, this assumes there is
 * enough storage to write it
 */
int32_t varint_write(int32_t value, uint8_t* buffer);

/**
 * Get the varint size based on the value
 */
int32_t varint_size(int32_t value);
