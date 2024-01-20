#include "arena.h"

#include "defines.h"

#include <stdlib.h>

void *PermanentStorageInit(U32 size) { return malloc(size); }

void PermanentStorageDestroy(void *permanent_storage) {
    free(permanent_storage);
}
