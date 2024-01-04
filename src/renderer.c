#include "renderer.h"
#include "defines.h"
#include "handmademath.h"

#include <raylib.h>
#include <rlgl.h>

void renderer_initialise(Renderer *renderer) {
  renderer->shaders[Shaders_CIRCLE_LINES] = LoadShader(0, "assets/shaders/circle_lines.fs");
  renderer->shaders[Shaders_LR_GRADIENT] = LoadShader(0, "assets/shaders/lr_gradient.fs");
  renderer->textures[Textures_DEFAULT] = (Texture2D){rlGetTextureIdDefault(), 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
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
  renderer->shaders[Shaders_CIRCLE_LINES] = LoadShader(0, "assets/shaders/circle_lines.fs");
  renderer->shaders[Shaders_LR_GRADIENT] = LoadShader(0, "assets/shaders/lr_gradient.fs");
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


void renderer_draw_frequencies(Renderer *renderer, u32 frequency_count, f32 *frequencies) {
  f32 cell_width = ceilf((f32)renderer->render_size.Width/((f32)frequency_count));

  u32 position_count = frequency_count;
  HMM_Vec2 positions[position_count];

  Color colors[position_count];

  u32 vertex_count = position_count;
  HMM_Vec2 vertices[vertex_count];

  for (u32 i = 0; i < position_count; ++i) {
    f32 t = frequencies[i];

    if (t < 0) t = 0;

    Color color = (Color){
      t*200, 
        t*125, 
        sin(t*GetTime())*50 + 200, 
        255
    };

    colors[i] = color;
    positions[i] = HMM_V2(i * cell_width, renderer->render_size.Height*(1-0.5*t));
  }

  for (u32 i = 0; i < position_count; ++i) {
    HMM_Vec2 position = positions[i];


    vertices[i] = HMM_SubV2(position, HMM_V2(renderer->render_size.Width/2.f, renderer->render_size.Height*(3.f/4.f)));
    vertices[i] = HMM_DivV2(vertices[i], HMM_V2(renderer->render_size.Width, renderer->render_size.Height/2.f));
    vertices[i] = HMM_AddV2(vertices[i], HMM_V2(0.5f, 0.5f));
  }
  
  rlSetTexture(renderer->textures[Textures_DEFAULT].id);

  rlBegin(RL_QUADS);
  {
    rlColor4ub(1, 1, 255, 255);
    for (int i = 0; i < vertex_count - 1; i++)
    {
      rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      rlTexCoord2f(vertices[i].X, 1.f);
      rlVertex2f(positions[i].X, renderer->render_size.Height);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(vertices[i + 1].X, 1.f);
      rlVertex2f(positions[i + 1].X, renderer->render_size.Height);

      rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      rlTexCoord2f(vertices[i].X, vertices[i].Y);
      rlVertex2f(positions[i].X, positions[i].Y);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(vertices[i + 1].X, vertices[i + 1].Y);
      rlVertex2f(positions[i + 1].X, positions[i + 1].Y);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(vertices[i + 1].X, 1.f);
      rlVertex2f(positions[i + 1].X, renderer->render_size.Height);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(vertices[i + 1].X, vertices[i + 1].Y);
      rlVertex2f(positions[i + 1].X, positions[i + 1].Y);

      rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      rlTexCoord2f(vertices[i].X, vertices[i].Y);
      rlVertex2f(positions[i].X, positions[i].Y);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(vertices[i + 1].X, vertices[i + 1].Y);
      rlVertex2f(positions[i + 1].X, positions[i + 1].Y);
    }
  }
  rlEnd();

  rlSetTexture(0);
}

void renderer_draw_circle_frequencies(Renderer *renderer, u32 frequency_count, f32 *frequencies) {
  f32 width = 2;
  i32 width_loc = GetShaderLocation(renderer->shaders[Shaders_CIRCLE_LINES], "width");
  SetShaderValue(renderer->shaders[Shaders_CIRCLE_LINES], width_loc, &width, SHADER_UNIFORM_FLOAT);

  i32 size_loc = GetShaderLocation(renderer->shaders[Shaders_CIRCLE_LINES], "size");

  {
    for (u32 i = 0; i < frequency_count; i+=10) {
      f32 t = frequencies[i];

      Color color = (Color){255-t*200, 255-t*125, 255-t*sin(t*10)*200, 255};

      f32 rad = (renderer->render_size.Height/2)*t*t;

      Rectangle rec = (Rectangle){renderer->render_size.Width/2 - rad, renderer->render_size.Height/2 - rad,
        2*rad, 2*rad};

      f32 size = 2*rad;

      SetShaderValue(renderer->shaders[Shaders_CIRCLE_LINES], size_loc, &size, SHADER_UNIFORM_FLOAT);

      BeginShaderMode(renderer->shaders[Shaders_CIRCLE_LINES]); 
      DrawTextureEx(renderer->textures[Textures_DEFAULT], (Vector2){rec.x, rec.y}, 0, 2*rad, color);
      EndShaderMode();
    }
  }
}
