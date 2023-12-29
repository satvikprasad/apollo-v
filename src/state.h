#pragma once

#include "defines.h"
#include "frame.h"
#include "raylib.h"
#include "handmademath.h"

#include <complex.h>

typedef struct State State;

#define STATE_METHODS \
  X(initialise, void, const char *fp) \
  X(destroy, void) \
  X(update, void) \
  X(attach, void, void *) \
  X(detach, void *)

#define X(name, ret, ...) typedef ret (name##_t)(__VA_ARGS__);
STATE_METHODS
#undef X

typedef struct State {
	Music music;
  Font font;

  HMM_Vec2 screen_size;
  HMM_Vec2 window_position;

  f32 master_volume;
  f32 wave_width;

  f32 samples[FREQ_COUNT];
  float complex freq[FREQ_COUNT];
} State;
