#pragma once

#include "defines.h"

typedef struct MemoryArena {
    U8 *base;
    U32 size;
    U32 used;
} MemoryArena;

#define ArenaPushStruct(arena, type)                                           \
    (type *)ArenaPushStruct_(arena, sizeof(type))

#define ArenaPushArray(arena, count, type)                                     \
    (type *)ArenaPushArray_(arena, count, sizeof(type))

void ArenaInitialise(MemoryArena *arena, U32 size, U8 *base);
void *ArenaPushStruct_(MemoryArena *arena, U32 size);
void *ArenaPushArray_(MemoryArena *arena, U32 count, U32 size);
void *ArenaPushString(MemoryArena *arena, const char *string);
