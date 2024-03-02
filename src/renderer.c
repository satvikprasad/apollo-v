#include "renderer.h"

#include <assert.h>
#include <raylib.h>
#include <rlgl.h>

#include "defines.h"
#include "ffmpeg.h"
#include "handmademath.h"
#include "lmath.h"

static inline Color
DefaultColorFunc(F32 t);
static inline void
DrawFrequencyPolygon(Texture2D tex,
                     HMM_Vec2 *indices,
                     U32       index_count,
                     HMM_Vec2 *vertices,
                     U32       vertex_count,
                     Color    *colors,
                     U32       color_count,
                     F32       bottom);

void
RendererInitialise(RendererData *renderer) {
    renderer->shaders[Shaders_CIRCLE_LINES] =
        LoadShader(0, "assets/shaders/circle_lines.fs");
    renderer->shaders[Shaders_LR_GRADIENT] =
        LoadShader(0, "assets/shaders/lr_gradient.fs");
    renderer->textures[Textures_DEFAULT] = (Texture2D){
        rlGetTextureIdDefault(), 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 0};

    renderer->default_color_func = DefaultColorFunc;
    renderer->screen = LoadRenderTexture(1280, 720);
}

void
RendererDestroy(RendererData *renderer) {
    for (U32 i = 0; i < SHADERS_MAX; ++i) {
        if (IsShaderReady(renderer->shaders[i])) {
            // UnloadShader(renderer->shaders[i]);
        }
    }

    for (U32 i = 0; i < TEXTURES_MAX; ++i) {
        if (IsTextureReady(renderer->textures[i])) {
            UnloadTexture(renderer->textures[i]);
        }
    }

    UnloadRenderTexture(renderer->screen);
}

void
RendererBeginRecording(RendererData *renderer) {
    assert(IsRenderTextureReady(renderer->screen));

    RendererSetRenderSize(renderer, HMM_V2(renderer->screen.texture.width,
                                           renderer->screen.texture.height));

    BeginTextureMode(renderer->screen);
}

void
RendererEndRecording(RendererData *renderer, I32 ffmpeg) {
    EndTextureMode();

    Image image = LoadImageFromTexture(renderer->screen.texture);
    { FFMPEGWrite(ffmpeg, image.data, renderer->render_size); }
    UnloadImage(image);
}

void
RendererSetRenderSize(RendererData *renderer, HMM_Vec2 render_size) {
    renderer->render_size = render_size;
}

void
RendererDrawWaveform(RendererData *renderer,
                     U32           sample_count,
                     F32          *samples,
                     F32           wave_width_multiplier) {
    U32 count = renderer->render_size.Width;
    F32 cell_width = ceilf((F32)renderer->render_size.Width / count);

    if (cell_width < 1.f)
        cell_width = 1.f;
    for (U32 i = 0; i < count; ++i) {
        F32 a = ((F32)i) / ((F32)count);
        I32 index = (I32)(ceilf(sample_count -
                                a * sample_count / wave_width_multiplier));

        F32 t = samples[index];

        if (t > 0) {
            DrawRectangle(
                i * cell_width, (renderer->render_size.Height / 2) * (1 - t),
                cell_width, renderer->render_size.Height / 2 * t, RED);
        } else {
            DrawRectangle(i * cell_width, renderer->render_size.Height / 2,
                          cell_width, -renderer->render_size.Height / 2 * t,
                          RED);
        }
    }
}

void
RendererDrawFrequencies(RendererData *renderer,
                        U32           frequency_count,
                        F32          *frequencies,
                        B8            outline,
                        color_func_t *color_func) {
    F32 cell_width = (F32)renderer->render_size.Width / ((F32)frequency_count);

    U32      vertex_count = frequency_count;
    HMM_Vec2 vertices[vertex_count];

    Color colors[vertex_count];

    U32      index_count = vertex_count;
    HMM_Vec2 indices[index_count];

    for (U32 i = 0; i < vertex_count; ++i) {
        F32 t = frequencies[i];

        if (t < 0)
            t = 0;

        Color color = color_func(t);

        colors[i] = color;
        vertices[i] = HMM_V2(i * cell_width,
                             renderer->render_size.Height * (1 - 0.5 * t));
    }

    for (U32 i = 0; i < vertex_count; ++i) {
        HMM_Vec2 vertex = vertices[i];

        indices[i] = HMM_SubV2(
            vertex, HMM_V2(renderer->render_size.Width / 2.f,
                           renderer->render_size.Height * (3.f / 4.f)));
        indices[i] =
            HMM_DivV2(indices[i], HMM_V2(renderer->render_size.Width,
                                         renderer->render_size.Height / 2.f));
        indices[i] = HMM_AddV2(indices[i], HMM_V2(0.5f, 0.5f));
    }

    DrawFrequencyPolygon(renderer->textures[Textures_DEFAULT], indices,
                         index_count, vertices, vertex_count, colors,
                         vertex_count, renderer->render_size.Height);

    if (outline) {
        RendererDrawLinedPoly(renderer, vertices, vertex_count, indices,
                              index_count, WHITE);
    }
}

void
RendererDrawLinedPoly(RendererData *renderer,
                      HMM_Vec2     *vertices,
                      U32           vertex_count,
                      HMM_Vec2     *indices,
                      U32           index_count,
                      Color         color) {
    assert(index_count == vertex_count &&
           "Index count must be equal to vertex count.");

    (void)vertex_count;
    (void)renderer;

    rlBegin(RL_LINES);
    {
        rlColor4ub(color.r, color.g, color.b, color.a);
        for (U32 i = 0; i < index_count - 1; ++i) {
            rlTexCoord2f(indices[i].X, indices[i].Y);
            rlVertex2f(vertices[i].X, vertices[i].Y);
            rlTexCoord2f(indices[i + 1].X, indices[i + 1].Y);
            rlVertex2f(vertices[i + 1].X, vertices[i + 1].Y);
        }
    }
    rlEnd();
}

void
RendererDrawCircleFrequencies(RendererData *renderer,
                              U32           frequency_count,
                              F32          *frequencies,
                              color_func_t  color_func) {
    for (U32 i = 0; i < frequency_count; i += 10) {
        F32 t = frequencies[i];

        Color color = color_func(t);

        F32 rad = (renderer->render_size.Height / 2) * t * t;

        Rectangle rec =
            (Rectangle){renderer->render_size.Width / 2,
                        renderer->render_size.Height / 2, 2 * rad, 2 * rad};

        DrawCircleLines(rec.x, rec.y, 2 * rad, color);
    }
}

void
RendererDrawTextCenter(Font        font,
                       const char *text,
                       HMM_Vec2    center,
                       Color       color) {
    U32 font_size = font.baseSize;

    HMM_Vec2 size = RayToHMMV2(MeasureTextEx(font, text, font_size, 1));

    HMM_Vec2 corner = HMM_SubV2(center, HMM_MulV2F(size, 0.5f));

    DrawTextEx(font, text, HMMToRayV2(corner), font_size, 1, color);
}

static inline Color
DefaultColorFunc(F32 t) {
    return (Color){t * 200, t * 125, sin(t * GetTime()) * 50 + 200, 255};
}

static inline void
DrawFrequencyPolygon(Texture2D tex,
                     HMM_Vec2 *indices,
                     U32       index_count,
                     HMM_Vec2 *vertices,
                     U32       vertex_count,
                     Color    *colors,
                     U32       color_count,
                     F32       bottom) {
    assert(index_count == vertex_count && vertex_count == color_count &&
           "Index count must be equal to vertex count.");

    rlSetTexture(tex.id);

    rlBegin(RL_QUADS);
    {
        for (U32 i = 0; i < index_count - 1; i++) {
            rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
            rlTexCoord2f(indices[i].X, 1.f);
            rlVertex2f(vertices[i].X, bottom);

            rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b,
                       colors[i + 1].a);
            rlTexCoord2f(indices[i + 1].X, 1.f);
            rlVertex2f(vertices[i + 1].X, bottom);

            rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
            rlTexCoord2f(indices[i].X, indices[i].Y);
            rlVertex2f(vertices[i].X, vertices[i].Y);

            rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b,
                       colors[i + 1].a);
            rlTexCoord2f(indices[i + 1].X, indices[i + 1].Y);
            rlVertex2f(vertices[i + 1].X, vertices[i + 1].Y);

            rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b,
                       colors[i + 1].a);
            rlTexCoord2f(indices[i + 1].X, 1.f);
            rlVertex2f(vertices[i + 1].X, bottom);

            rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b,
                       colors[i + 1].a);
            rlTexCoord2f(indices[i + 1].X, indices[i + 1].Y);
            rlVertex2f(vertices[i + 1].X, vertices[i + 1].Y);

            rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
            rlTexCoord2f(indices[i].X, indices[i].Y);
            rlVertex2f(vertices[i].X, vertices[i].Y);

            rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b,
                       colors[i + 1].a);
            rlTexCoord2f(indices[i + 1].X, indices[i + 1].Y);
            rlVertex2f(vertices[i + 1].X, vertices[i + 1].Y);
        }
    }
    rlEnd();

    rlSetTexture(0);
}
