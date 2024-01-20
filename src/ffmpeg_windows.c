#include <assert.h>
#include <stdbool.h>

#include "ffmpeg.h"
#include "handmademath.h"

i32 FFMPEGStart(HMM_Vec2 size, u32 fps, const char *music) { return -1; }

void FFMPEGEnd(i32 pipe) {}

void FFMPEGWrite(i32 pipe, void *data, HMM_Vec2 size) {}
