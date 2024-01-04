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
  char *music_fp;

  b8 recording;

  Font font;

  HMM_Vec2 screen_size;
  HMM_Vec2 window_position;

  f32 master_volume;
  f32 wave_width;

  f32 samples[FREQ_COUNT];
  f32 smooth_freq[1000];
  f32 *window_buffer;
  float complex freq[FREQ_COUNT];

  f32 dt;

  Shader circle_lines_shader;
  Shader lr_gradient_shader;

  Texture2D default_tex;
  RenderTexture2D screen;

  i32 ffmpeg;
  f32 record_start;

  struct {
    Wave wave;
    f32 *wave_samples;
    u32 wave_cursor;
  } record_data;
} State;
