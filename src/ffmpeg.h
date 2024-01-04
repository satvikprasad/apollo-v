#pragma once

#include "defines.h"
#include "handmademath.h"

i32 ffmpeg_start(HMM_Vec2 size, u32 fps, const char *music);
void ffmpeg_end(i32 pipe);
void ffmpeg_write(i32 pipe, void *data, HMM_Vec2 size);
