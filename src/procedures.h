#pragma once

#include "arena.h"
#include "defines.h"
#include "hashmap.h"

typedef void (*ProcedureCallback)(void *data);

typedef struct Procedure {
    const char       *name;
    ProcedureCallback callback;
    B8                active;
    void             *user_data;
} Procedure;

HM_Hashmap *
ProcedureCreate();

#define ProcedureAdd(procedures, name, user_data, callback, arena)             \
    ProcedureAdd_((procedures), (name), (void *)(user_data),                   \
                  sizeof(*(user_data)), (callback), (arena))

Procedure *
ProcedureAdd_(HM_Hashmap       *procedures,
              const char       *name,
              void             *user_data,
              U32               user_data_size,
              ProcedureCallback callback,
              MemoryArena      *arena);

void
ProcedureCall(HM_Hashmap *procedures, const char *name);

void
ProcedureCallAll(HM_Hashmap *procedures);

B8
ProcedureIter(HM_Hashmap *procedures, U32 *iter, Procedure **procedure);

void
ProcedureToggle(HM_Hashmap *procs, const char *name);
