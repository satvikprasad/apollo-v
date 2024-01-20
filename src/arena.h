#pragma once

#include "defines.h"

typedef struct MemoryArena {
    U8 *base;
    U32 size;
    U32 used;
} MemoryArena;

#define ArenaPushStruct(arena, type)                                           \
    (type *)ArenaPushStruct_(arena, sizeof(type))

void ArenaInitialise(MemoryArena *arena, U32 size, U8 *base);
void *ArenaPushStruct_(MemoryArena *arena, U32 size);
