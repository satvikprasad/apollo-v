#pragma once

#include "defines.h"

void *PermanentStorageInit(U32 size);

void PermanentStorageDestroy(void *permanent_storage);
