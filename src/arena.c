#include "arena.h"

#include "defines.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void
ArenaInitialise(MemoryArena *arena, U32 size, U8 *base) {
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

void *
ArenaPushStruct_(MemoryArena *arena, U32 size) {
    assert(arena->used + size <= arena->size);

    void *result = arena->base + arena->used;
    arena->used += size;

    return result;
}

void *
ArenaPushArray_(MemoryArena *arena, U32 count, U32 size) {
    assert(arena->used + count * size <= arena->size);

    void *result = arena->base + arena->used;
    arena->used += count * size;

    return result;
}

char *
ArenaPushString(MemoryArena *arena, const char *string) {
    U32 size = (strlen(string) + 1) * sizeof(char);
    assert(arena->used + size <= arena->size);

    char *result = (char *)(arena->base + arena->used);
    arena->used += size;

    strcpy(result, string);

    return result;
}
