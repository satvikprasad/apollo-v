#pragma once

#include "defines.h"
#include "handmademath.h"

I32 FFMPEGStart(HMM_Vec2 size, U32 fps, const char *music);
void FFMPEGEnd(I32 pipe);
void FFMPEGWrite(I32 pipe, void *data, HMM_Vec2 size);
