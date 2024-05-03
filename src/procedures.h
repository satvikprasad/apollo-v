#pragma once

#include "arena.h"
#include "defines.h"
#include "hashmap.h"

typedef void (*ProcedureCallback)(void *data);

typedef struct Procedure {
    char             *name;
    ProcedureCallback callback;
    B8                active;
    void             *user_data;
} Procedure;

typedef struct _Procedure {
    char        name[512];
    HM_Hashmap *procedures;
} _Procedure;

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

_Procedure
_ProcedureAdd(HM_Hashmap       *procs,
              const char       *name,
              void             *user_data,
              ProcedureCallback proc,
              MemoryArena      *arena);

void
ProcedureCall(HM_Hashmap *procedures, const char *name);

void
_ProcedureCall(_Procedure proc);

void
ProcedureCallAll(HM_Hashmap *procedures);

B8
ProcedureIter(HM_Hashmap *procedures, U32 *iter, Procedure **procedure);

void
ProcedureToggle(HM_Hashmap *procs, const char *name);

_Procedure
ProcedureGeneratePtr(HM_Hashmap *procs, const char *name);
