#include "renderer.h"
#include "defines.h"
#include "handmademath.h"
#include "lmath.h"
#include "signals.h"

#include <assert.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>

static inline Color default_color_func(f32 t);

static inline void draw_frequency_polygon(Texture2D tex, HMM_Vec2 *indices, u32 index_count, 
  HMM_Vec2 *vertices, u32 vertex_count,
  Color *colors, u32 color_count, f32 bottom) {
  assert(index_count == vertex_count && vertex_count == color_count && "Index count must be equal to vertex count.");

  rlSetTexture(tex.id);

  rlBegin(RL_QUADS);
  {
    for (i32 i = 0; i < index_count - 1; i++)
    {
      rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      rlTexCoord2f(indices[i].X, 1.f);
      rlVertex2f(vertices[i].X, bottom);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(indices[i + 1].X, 1.f);
      rlVertex2f(vertices[i + 1].X, bottom);

      rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      rlTexCoord2f(indices[i].X, indices[i].Y);
      rlVertex2f(vertices[i].X, vertices[i].Y);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(indices[i + 1].X, indices[i + 1].Y);
      rlVertex2f(vertices[i + 1].X, vertices[i + 1].Y);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(indices[i + 1].X, 1.f);
      rlVertex2f(vertices[i + 1].X, bottom);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(indices[i + 1].X, indices[i + 1].Y);
      rlVertex2f(vertices[i + 1].X, vertices[i + 1].Y);

      rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      rlTexCoord2f(indices[i].X, indices[i].Y);
      rlVertex2f(vertices[i].X, vertices[i].Y);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(indices[i + 1].X, indices[i + 1].Y);
      rlVertex2f(vertices[i + 1].X, vertices[i + 1].Y);
    }
  }
  rlEnd();

  rlSetTexture(0);
}

void renderer_initialise(Renderer *renderer) {
  renderer->shaders[Shaders_CIRCLE_LINES] = LoadShader(0, "assets/shaders/circle_lines.fs");
  renderer->shaders[Shaders_LR_GRADIENT] = LoadShader(0, "assets/shaders/lr_gradient.fs");
  renderer->textures[Textures_DEFAULT] = (Texture2D){rlGetTextureIdDefault(), 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

  renderer->default_color_func = default_color_func;
}

void renderer_destroy(Renderer *renderer) {
  for (u32 i = 0; i < SHADERS_MAX; ++i) {
    UnloadShader(renderer->shaders[i]);
  }

  for (u32 i = 0; i < TEXTURES_MAX; ++i) {
    UnloadTexture(renderer->textures[i]);
  }
}

void renderer_attach(Renderer *renderer) {
  renderer->default_color_func = default_color_func;

  renderer->shaders[Shaders_CIRCLE_LINES] = LoadShader(0, "assets/shaders/circle_lines.fs");
  renderer->shaders[Shaders_LR_GRADIENT] = LoadShader(0, "assets/shaders/lr_gradient.fs");
}

void renderer_detach(Renderer *renderer) {
  UnloadShader(renderer->shaders[Shaders_CIRCLE_LINES]);
  UnloadShader(renderer->shaders[Shaders_LR_GRADIENT]);
}

void renderer_set_render_size(Renderer *renderer, HMM_Vec2 render_size) {
  renderer->render_size = render_size;
}

void renderer_draw_waveform(Renderer *renderer, u32 sample_count, f32 *samples, f32 wave_width_multiplier) {
  u32 count = renderer->render_size.Width;
  f32 cell_width = ceilf((f32)renderer->render_size.Width/count);

  if (cell_width < 1.f) cell_width = 1.f;
  for (u32 i = 0; i < count; ++i) {
    f32 a = ((f32)i)/((f32)count);
    i32 index = (i32)(ceilf(sample_count - a*sample_count/wave_width_multiplier));

    f32 t = samples[index];

    if (t > 0) {
      DrawRectangle(i*cell_width, (renderer->render_size.Height/2)*(1 - t), cell_width, renderer->render_size.Height/2*t, RED);
    } else {
      DrawRectangle(i*cell_width, renderer->render_size.Height/2, cell_width, -renderer->render_size.Height/2*t, RED);
    }
  }
}


void renderer_draw_frequencies(Renderer *renderer, u32 frequency_count, 
    f32 *frequencies, b8 outline, color_func_t *color_func) {
  f32 cell_width = (f32)renderer->render_size.Width/((f32)frequency_count);

  u32 vertex_count = frequency_count;
  HMM_Vec2 vertices[vertex_count];

  Color colors[vertex_count];

  u32 index_count = vertex_count;
  HMM_Vec2 indices[index_count];

  for (u32 i = 0; i < vertex_count; ++i) {
    f32 t = frequencies[i];

    if (t < 0) t = 0;

    Color color = color_func(t);

    colors[i] = color;
    vertices[i] = HMM_V2(i * cell_width, renderer->render_size.Height*(1-0.5*t));
  }

  for (u32 i = 0; i < vertex_count; ++i) {
    HMM_Vec2 vertex = vertices[i];

    indices[i] = HMM_SubV2(vertex, HMM_V2(renderer->render_size.Width/2.f, renderer->render_size.Height*(3.f/4.f)));
    indices[i] = HMM_DivV2(indices[i], HMM_V2(renderer->render_size.Width, renderer->render_size.Height/2.f));
    indices[i] = HMM_AddV2(indices[i], HMM_V2(0.5f, 0.5f));
  }

  draw_frequency_polygon(renderer->textures[Textures_DEFAULT], indices, index_count, vertices, vertex_count,
    colors, vertex_count, renderer->render_size.Height);

  if (outline) {
    rlBegin(RL_LINES);
    {
      rlColor4ub(255, 255, 255, 255);
      for (u32 i = 0; i < index_count - 1; ++i) {
        rlTexCoord2f(indices[i].X, indices[i].Y);
        rlVertex2f(vertices[i].X, vertices[i].Y);
        rlTexCoord2f(indices[i+1].X, indices[i+1].Y);
        rlVertex2f(vertices[i+1].X, vertices[i+1].Y);
      }
    }
    rlEnd();
  }
}

void renderer_draw_circle_frequencies(Renderer *renderer, u32 frequency_count, f32 *frequencies) {
  f32 width = 2;
  assert(IsShaderReady(renderer->shaders[Shaders_CIRCLE_LINES]));

  {
    for (u32 i = 0; i < frequency_count; i+=10) {
      f32 t = frequencies[i];

      Color color = (Color){255-t*200, 255-t*125, 255-t*sin(t*10)*200, 255};

      f32 rad = (renderer->render_size.Height/2)*t*t;

      Rectangle rec = (Rectangle){renderer->render_size.Width/2, renderer->render_size.Height/2,
        2*rad, 2*rad};

      DrawCircleLines(rec.x, rec.y, 2*rad, color);
    }
  }
}

void renderer_draw_text_center(Font font, const char *text, HMM_Vec2 center) {
  u32 font_size = font.baseSize;

  HMM_Vec2 size = ray_to_hmm_v2(MeasureTextEx(font, text, font_size, 1));

  HMM_Vec2 corner = HMM_SubV2(center, HMM_MulV2F(size, 0.5f));

  DrawTextEx(font, text, hmm_to_ray_v2(corner), font_size, 1, WHITE);
}

static inline Color default_color_func(f32 t) {
  return (Color){t*200, t*125, sin(t*GetTime())*50 + 200, 255};
}
