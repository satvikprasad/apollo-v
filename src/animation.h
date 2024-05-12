#pragma once

#include "arena.h"
#include "defines.h"
#include "hashmap.h"

typedef struct _Animation {
    F64   elapsed;
    F64   val;
    char *name;

    void *user_data;

    void (*update)(struct _Animation *anim, void *user_data, F64 dt);

    B8 finished;
} _Animation;

HM_Hashmap *
AnimationsCreate();

void
AnimationsUpdate(HM_Hashmap *anims);

#define AnimationsAdd(anims, name, user_data, update, arena)                   \
    AnimationsAdd_((anims), (name), (void *)(user_data), sizeof(*(user_data)), \
                   (update), (arena))

_Animation *
AnimationsAdd_(HM_Hashmap *anims,
               const char *name,
               void       *user_data,
               U32         user_data_size,
               void (*update)(_Animation *anim, void *user_data, F64 dt),
               MemoryArena *arena);

void
AnimationsDelete(HM_Hashmap *anims, char *name);

F32
AnimationsLoad(HM_Hashmap *anims, const char *name);

F32
AnimationsLoad_(HM_Hashmap *anims, _Animation *anim);

B8
AnimationsExists(HM_Hashmap *anims, const char *name);

B8
AnimationsExists_(HM_Hashmap *anims, _Animation *anim);

void
AnimationsApply(HM_Hashmap *anims, char *name, F64 *val);
