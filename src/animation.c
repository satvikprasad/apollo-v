#include "animation.h"
#include "arena.h"
#include "hashmap.h"
#include "lmath.h"
#include "raylib.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static U64
AnimationsHash(const void *item, U64 seed0, U64 seed1) {
    _Animation *anim = (_Animation *)item;

    return hashmap_sip(anim->name, strlen(anim->name), seed0, seed1);
}

static I32
AnimationsCompare(const void *a, const void *b, void *udata) {
    _Animation *anim_a = (_Animation *)a;
    _Animation *anim_b = (_Animation *)b;

    return strcmp(anim_a->name, anim_b->name);
}

static void
AnimationsFree(void *item) {
    _Animation *anim = (_Animation *)item;

    if (anim->user_data) {
        free(anim->user_data);
    }

    free(anim->name);
    free(anim);
}

HM_Hashmap *
AnimationsCreate() {
    return hashmap_new(sizeof(_Animation), 0, 0, 0, AnimationsHash,
                       AnimationsCompare, AnimationsFree, NULL);
}

/**
 * @brief Adds an animation to the animations hashmap. Allocates user_data for
 * you.
 *
 * @param anims The animations hashmap.
 * @param name The name of the animation.
 * @param user_data The user data to pass to the animation update function.
 * @param user_data_size The size of the user data.
 * @param update The animation update function.
 * @param arena The arena to allocate the animation from.
 * @return Animation* The animation.
 */
_Animation *
AnimationsAdd_(HM_Hashmap *anims,
               const char *name,
               void       *user_data,
               U32         user_data_size,
               void (*update)(_Animation *anim, void *user_data, F64 dt),
               MemoryArena *arena) {
    if (AnimationsExists(anims, name)) {
        _Animation *anim = (_Animation *)hashmap_get(
            anims, &(_Animation){.name = (char *)name});

        anim->val = 0.0f;

        hashmap_set(anims, anim);
    }

    _Animation *anim = malloc(sizeof(_Animation));
    memset(anim, 0, sizeof(_Animation));
    anim->update = update;

    if (user_data) {
        anim->user_data = malloc(user_data_size);
        memcpy(anim->user_data, user_data, user_data_size);
    }

    anim->name = calloc(strlen(name) + 1, sizeof(char));
    strcpy(anim->name, name);

    hashmap_set(anims, anim);

    return anim;
}

void
AnimationsUpdate(HM_Hashmap *anims) {
    U32         i;
    _Animation *anim;

    while (hashmap_iter(anims, &i, (void **)&anim)) {
        if (anim->finished) {
            AnimationsDelete(anims, anim->name);
            continue;
        }

        anim->elapsed += GetFrameTime();
        anim->update(anim, anim->user_data, GetFrameTime());
        anim->val = ClampF32(anim->val, 0.0f, 1.0f);
    }
}

B8
AnimationsExists(HM_Hashmap *anims, const char *name) {
    _Animation *anim =
        (_Animation *)hashmap_get(anims, &(_Animation){.name = (char *)name});

    return anim != NULL;
}

B8
AnimationsExists_(HM_Hashmap *anims, _Animation *anim) {
    if (anim) {
        return AnimationsExists(anims, anim->name);
    }

    return NULL;
}

F32
AnimationsLoad(HM_Hashmap *anims, const char *name) {
    if (!name) {
        return 1.0f;
    }

    _Animation *anim =
        (_Animation *)hashmap_get(anims, &(_Animation){.name = (char *)name});

    if (anim) {
        return anim->val;
    }

    return 1.0f;
}

F32
AnimationsLoad_(HM_Hashmap *anims, _Animation *anim) {
    if (anim) {
        return AnimationsLoad(anims, anim->name);
    }

    return 1.0f;
}

F32
AnimationsLoadEx(HM_Hashmap *anims, _Animation *anim_in) {
    _Animation *anim = (_Animation *)hashmap_get(
        anims, &(_Animation){.name = (char *)anim_in->name});

    if (anim) {
        return anim->val;
    }

    return 1.0f;
}

void
AnimationsDelete(HM_Hashmap *anims, char *name) {
    hashmap_delete(anims, &(_Animation){.name = name});
}

void
AnimationsApply(HM_Hashmap *anims, char *name, F64 *val) {
    if (AnimationsExists(anims, name)) {
        *val = AnimationsLoad(anims, name);
    }
}

void
AnimationsApply_(HM_Hashmap *anims, _Animation anim) {
    if (anim.name) {
        AnimationsApply(anims, anim.name, &anim.val);
    } else {
        assert(0 && "Received NULL Animation");
    }
}
