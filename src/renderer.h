#pragma once

#include "defines.h"
#include "handmademath.h"
#include "raylib.h"

typedef enum Textures {
  Textures_DEFAULT = 0,
  TEXTURES_MAX
} Textures;

typedef enum Shaders {
  Shaders_CIRCLE_LINES = 0,
  Shaders_LR_GRADIENT,
  SHADERS_MAX
} Shaders;

typedef struct Renderer {
  Texture2D textures[TEXTURES_MAX];
  Shader shaders[SHADERS_MAX];

  HMM_Vec2 render_size;
} Renderer; 

void renderer_initialise(Renderer *renderer);
void renderer_destroy(Renderer *renderer);
void renderer_attach(Renderer *renderer);
void renderer_set_render_size(Renderer *renderer, HMM_Vec2 render_size);
void renderer_draw_waveform(Renderer *renderer, u32 sample_count, f32 *samples, f32 wave_width_multiplier);
void renderer_draw_frequencies(Renderer *renderer, u32 frequency_count, f32 *frequencies);
void renderer_draw_circle_frequencies(Renderer *renderer, u32 frequency_count, f32 *frequencies);
