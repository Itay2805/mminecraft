#include "arena.h"

#include <sys/mman.h>

void arena_init(arena_t* arena) {
    arena->ptr = arena->base = mmap(NULL, 1024 * 1024 * 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

void* arena_alloc(arena_t* arena, size_t size) {
    void* ptr = arena->ptr;
    arena->ptr += size;
    return ptr;
}

void arena_reset(arena_t* arena) {
    arena->ptr = arena->base;
}

