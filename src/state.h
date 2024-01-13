#pragma once

#include <complex.h>

#include "defines.h"
#include "handmademath.h"
#include "hashmap.h"
#include "raylib.h"
#include "renderer.h"

typedef struct State State;

#define STATE_METHODS     \
  X(initialise, void)     \
  X(destroy, void)        \
  X(update, void)         \
  X(render, void)         \
  X(attach, void, void *) \
  X(detach, void *)

#define X(name, ret, ...) typedef ret(name##_t)(__VA_ARGS__);
STATE_METHODS
#undef X

typedef enum StateCondition {
  StateCondition_NORMAL = 0,
  StateCondition_ERROR,
  StateCondition_LOAD,
  StateCondition_RECORDING,
} StateCondition;

#define MAX_PARAM_COUNT 100

typedef struct State {
  Renderer renderer;

  Music music;
  char *music_fp;

  Font font;

  HMM_Vec2 screen_size;
  HMM_Vec2 window_position;

  f32 master_volume;

  f32 samples[SAMPLE_COUNT];
  f32 *frequencies;
  u32 frequency_count;

  f32 dt;

  RenderTexture2D screen;

  i32 ffmpeg;
  f32 record_start;

  struct {
    Wave wave;
    f32 *wave_samples;
    u32 wave_cursor;
  } record_data;

  f32 *filter;
  u32 filter_count;

  StateCondition condition;

  HM_Hashmap *parameters;
} State;
