#pragma once

#include "defines.h"
#include "handmademath.h"
#include "raylib.h"

typedef Color(color_func_t)(F32 t);

typedef enum Textures { Textures_DEFAULT = 0, TEXTURES_MAX } Textures;

typedef enum Shaders {
    Shaders_CIRCLE_LINES = 0,
    Shaders_LR_GRADIENT,
    SHADERS_MAX
} Shaders;

typedef struct RendererData {
    Texture2D textures[TEXTURES_MAX];
    Shader    shaders[SHADERS_MAX];

    RenderTexture2D screen;

    HMM_Vec2 render_size;

    color_func_t *default_color_func;
} RendererData;

void RendererInitialise(RendererData *renderer);
void RendererDestroy(RendererData *renderer);

void RendererSetRenderSize(RendererData *renderer, HMM_Vec2 render_size);

void RendererDrawWaveform(RendererData *renderer,
                          U32           sample_count,
                          F32          *samples,
                          F32           wave_width_multiplier);

void RendererDrawFrequencies(RendererData *renderer,
                             U32           frequency_count,
                             F32          *frequencies,
                             B8            outline,
                             color_func_t *color_func);

void RendererDrawCircleFrequencies(RendererData *renderer,
                                   U32           frequency_count,
                                   F32          *frequencies,
                                   color_func_t  color_func);

void RendererDrawTextCenter(Font        font,
                            const char *text,
                            HMM_Vec2    center,
                            Color       color);

void RendererBeginRecording(RendererData *renderer);
void RendererEndRecording(RendererData *renderer, I32 ffmpeg);
void RendererDrawLinedPoly(RendererData *renderer,
                           HMM_Vec2     *vertices,
                           U32           vertex_count,
                           HMM_Vec2     *indices,
                           U32           index_count,
                           Color         color);
