#pragma once

#include <stddef.h>

typedef struct arena {
    // the allocation base
    void* base;

    // the allocation ptr
    void* ptr;
} arena_t;

void arena_init(arena_t* arena);

void* arena_alloc(arena_t* arena, size_t size);

void arena_reset(arena_t* arena);
