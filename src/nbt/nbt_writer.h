#pragma once

#include <stdint.h>

#define NBT_END(arr) \
    do { \
        arrpush(arr, 0); \
    } while(0)

#define NBT_RAW_STRING(arr, name) \
    do { \
        uint16_t __len = strlen(name); \
        arrpush(arr, __len >> 8); \
        arrpush(arr, __len & 0xFF); \
        void* ptr = arraddnptr(arr, __len); \
        memcpy(ptr, name, __len); \
    } while(0)

#define NBT_BYTE(arr, name, value) \
    do { \
        arrpush(arr, 1); \
        NBT_RAW_STRING(arr, name); \
        arrpush(arr, value); \
    } while(0)

#define NBT_SHORT(arr, name, value) \
    do { \
        uint16_t __value = value; \
        arrpush(arr, 2); \
        NBT_RAW_STRING(arr, name); \
        arrpush(arr, __value >> 8); \
        arrpush(arr, __value & 0xFF); \
    } while(0)

#define NBT_INT(arr, name, value) \
    do { \
        uint32_t __value = value; \
        arrpush(arr, 3); \
        NBT_RAW_STRING(arr, name); \
        arrpush(arr, (__value >> 24) & 0xFF); \
        arrpush(arr, (__value >> 16) & 0xFF); \
        arrpush(arr, (__value >> 8) & 0xFF); \
        arrpush(arr, __value & 0xFF); \
    } while(0)

#define NBT_LONG(arr, name, value) \
    do { \
        uint32_t __value = value; \
        arrpush(arr, 4); \
        NBT_RAW_STRING(arr, name); \
        arrpush(arr, (__value >> 52) & 0xFF); \
        arrpush(arr, (__value >> 48) & 0xFF); \
        arrpush(arr, (__value >> 40) & 0xFF); \
        arrpush(arr, (__value >> 32) & 0xFF); \
        arrpush(arr, (__value >> 24) & 0xFF); \
        arrpush(arr, (__value >> 16) & 0xFF); \
        arrpush(arr, (__value >> 8) & 0xFF); \
        arrpush(arr, __value & 0xFF); \
    } while(0)

#define NBT_FLOAT(arr, name, value) \
    do { \
        float __value2 = value; \
        uint32_t __value = *(uint32_t*)&__value2; \
        arrpush(arr, 5); \
        NBT_RAW_STRING(arr, name); \
        arrpush(arr, (__value >> 24) & 0xFF); \
        arrpush(arr, (__value >> 16) & 0xFF); \
        arrpush(arr, (__value >> 8) & 0xFF); \
        arrpush(arr, __value & 0xFF); \
    } while(0)


#define NBT_STRING(arr, name, value) \
    do { \
        arrpush(arr, 8); \
        NBT_RAW_STRING(arr, name); \
        NBT_RAW_STRING(arr, value); \
    } while(0)

#define NBT_LIST(arr, name, size) \
    do { \
        uint32_t __value = size; \
        arrpush(arr, 9); \
        NBT_RAW_STRING(arr, name); \
        arrpush(arr, (__value >> 24) & 0xFF); \
        arrpush(arr, (__value >> 16) & 0xFF); \
        arrpush(arr, (__value >> 8) & 0xFF); \
        arrpush(arr, __value & 0xFF); \
    } while(0)

#define NBT_COMPOUND(arr, name) \
    do { \
        arrpush(arr, 10); \
        NBT_RAW_STRING(arr, name); \
    } while(0)
