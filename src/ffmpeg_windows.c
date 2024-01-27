#include <assert.h>
#include <stdbool.h>

#include "ffmpeg.h"
#include "handmademath.h"

I32 FFMPEGStart(HMM_Vec2 size, U32 fps, const char *music) { return -1; }

void FFMPEGEnd(I32 pipe) {}

void FFMPEGWrite(I32 pipe, void *data, HMM_Vec2 size) {}
