#pragma once

#include <miniaudio.h>

#include "defines.h"

typedef struct LoopbackData LoopbackData;

void
LoopbackInitialise(LoopbackData *data, void *state);
void
LoopbackDestroy(LoopbackData *data);
U32
LoopbackDataSize();
