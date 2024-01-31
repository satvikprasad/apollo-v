#pragma once

#include <miniaudio.h>

#include "defines.h"

typedef struct LoopbackData {
    ma_device device;
    B8 is_initialised;
} LoopbackData;

void LoopbackDataCallback(ma_device *device, void *output, const void *input,
                          ma_uint32 frame_count);
void LoopbackInitialise(LoopbackData *data, void *state);
void LoopbackDestroy(LoopbackData *data);
