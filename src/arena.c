#include "arena.h"

#include "defines.h"

#include <assert.h>
#include <stdlib.h>

void ArenaInitialise(MemoryArena *arena, U32 size, U8 *base) {
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

#define ArenaPushStruct(arena, type)                                           \
    (type *)ArenaPushStruct_(arena, sizeof(type))
void *ArenaPushStruct_(MemoryArena *arena, U32 size) {
    assert(arena->used + size <= arena->size);

    void *result = arena->base + arena->used;
    arena->used += size;

    return result;
}
