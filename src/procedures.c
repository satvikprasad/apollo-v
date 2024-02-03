#include "procedures.h"
#include "arena.h"
#include <stdbool.h>
#include <string.h>

static U64
ProcedureHash(const void *item, U64 seed0, U64 seed1) {
    Procedure *proc = (Procedure *)item;

    return hashmap_sip(proc->name, strlen(proc->name), seed0, seed1);
}

static I32
ProcedureCompare(const void *a, const void *b, void *udata) {
    (void)(udata);

    Procedure *pa = (Procedure *)a;
    Procedure *pb = (Procedure *)b;
    return strcmp(pa->name, pb->name);
}

static void
ProcedureFree(void *item) {}

HM_Hashmap *
ProcedureCreate() {
    return hashmap_new(sizeof(Procedure), 0, 0, 0, ProcedureHash,
                       ProcedureCompare, ProcedureFree, NULL);
}

Procedure *
ProcedureAdd_(HM_Hashmap       *procs,
              const char       *name,
              void             *user_data,
              U32               user_data_size,
              ProcedureCallback proc,
              MemoryArena      *arena) {
    Procedure *procedure = ArenaPushStruct(arena, Procedure);
    procedure->callback = proc;
    procedure->name = ArenaPushString(arena, name);
    procedure->active = true;
    procedure->user_data = user_data;

    hashmap_set(procs, procedure);

    return procedure;
}

void
ProcedureCall(HM_Hashmap *procs, const char *name) {
    Procedure *proc =
        (Procedure *)hashmap_get(procs, &(Procedure){.name = name});

    if (proc) {
        if (proc->active) {
            proc->callback(proc->user_data);
        }
    }
}

void
ProcedureCallAll(HM_Hashmap *procs) {
    U32        iter = 0;
    Procedure *proc;
    while (ProcedureIter(procs, &iter, &proc)) {
        if (proc->active) {
            proc->callback(proc->user_data);
        }
    }
}

void
ProcedureToggle(HM_Hashmap *procs, const char *name) {
    Procedure *proc =
        (Procedure *)hashmap_get(procs, &(Procedure){.name = name});

    if (proc) {
        proc->active = !proc->active;
    }
}

B8
ProcedureIter(HM_Hashmap *procs, U32 *iter, Procedure **procedure) {
    return hashmap_iter(procs, iter, (void **)procedure);
}

typedef struct StateWrapperData {
    void (*callback)(void *user_data);
    void *user_data;
} StateWrapperData;

void
ProcedureStateWrapper(void *user_data) {}
