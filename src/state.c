#include "state.h"
#include "defines.h"
#include "handmademath.h"
#include "raylib.h"
#include "rlgl.h"
#include "ffmpeg.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

State *state;

static inline void serialize(State *state);
static inline void deserialize(State *state);
static inline void draw_frequencies(u32 sample_count, HMM_Vec2 render_size);
static inline void draw_circle_frequencies(u32 sample_count, HMM_Vec2 render_size);
static inline void draw_waveform(HMM_Vec2 render_size);

static inline void draw_textured_poly(Texture2D texture, HMM_Vec2 center, HMM_Vec2 *points, HMM_Vec2 *texcoords, int pointCount, Color tint);
static inline void push_frame(f32 val);
static inline u32 process_samples(f32 scale, f32 start_freq, f32 dt);
static inline void window_samples(f32 *in, f32 *out, u32 length);
static inline void fft(f32 in[], u32 stride, float complex out[], u32 n);
static inline f32 amp(float complex z);
static inline f32 clamp(f32 v, f32 min, f32 max);
static inline void push_frame(f32 val);
static inline void frame_callback(void *buffer_data, u32 n);
static inline f32 min(f32 a, f32 b);
static inline f32 max(f32 a, f32 b);
static inline HMM_Vec2 ray_to_hmmv2(Vector2 v);

void state_initialise(const char *fp) {
  state = malloc(sizeof(State));
  memset(state, 0, sizeof(State));

  state->music_fp = malloc(sizeof(char) * 128);

  deserialize(state);

  state->font = LoadFontEx("assets/fonts/helvetica.ttf", 20, NULL, 0);

  state->circle_lines_shader = LoadShader(0, "assets/shaders/circle_lines.fs");
  state->lr_gradient_shader = LoadShader(0, "assets/shaders/lr_gradient.fs");

  state->music = LoadMusicStream(state->music_fp);

  state->wave_width = 10;

  state->screen = LoadRenderTexture(1280, 720);
  state->window_buffer = calloc(FREQ_COUNT, sizeof(f32));
  state->default_tex = (Texture2D){rlGetTextureIdDefault(), 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

  if (IsMusicReady(state->music)) {
    PlayMusicStream(state->music);
    AttachAudioStreamProcessor(state->music.stream, frame_callback);
  }
}

void state_destroy() {
  serialize(state);

  free(state->music_fp);
  free(state->window_buffer);
  free(state);
}

void *state_detach() {
  if(IsMusicReady(state->music)) {
    DetachAudioStreamProcessor(state->music.stream, frame_callback);
  }

  UnloadShader(state->circle_lines_shader);
  UnloadShader(state->lr_gradient_shader);

  return state;
}

void state_attach(void *stateptr) {
  state = stateptr;

  if(IsMusicReady(state->music)) {
    AttachAudioStreamProcessor(state->music.stream, frame_callback);
  }

  state->circle_lines_shader = LoadShader(0, "assets/shaders/circle_lines.fs");
  state->lr_gradient_shader = LoadShader(0, "assets/shaders/lr_gradient.fs");
}

void state_update() {
  if (IsMusicReady(state->music)) {
    UpdateMusicStream(state->music);
  }

  if (state->record_data.wave_cursor >= state->record_data.wave.frameCount && state->recording) {
    ffmpeg_end(state->ffmpeg);

    UnloadWave(state->record_data.wave);
    UnloadWaveSamples(state->record_data.wave_samples);

    ResumeMusicStream(state->music);

    state->recording = false;
  }


  if (!state->recording) {
    if(IsKeyPressed(KEY_SPACE) && IsMusicReady(state->music)) {
      if (IsMusicStreamPlaying(state->music)) {
        PauseMusicStream(state->music);
      } else {
        ResumeMusicStream(state->music);
      }
    }

    if (IsWindowResized()) {
      state->screen_size = HMM_V2(GetRenderWidth(), GetRenderHeight());
    }

    if (IsKeyPressed(KEY_B) && IsMusicReady(state->music)) {
      memset(state->samples, 0, sizeof(f32)*FREQ_COUNT);
      memset(state->smooth_freq, 0, sizeof(f32)*1000);

      HMM_Vec2 render_size = HMM_V2(state->screen.texture.width, state->screen.texture.height);
      
      state->ffmpeg = ffmpeg_start(render_size, RENDER_FPS, state->music_fp);
      state->record_start = GetTime();

      state->record_data.wave = LoadWave(state->music_fp);
      state->record_data.wave_samples = LoadWaveSamples(state->record_data.wave);
      state->record_data.wave_cursor = 0;

      PauseMusicStream(state->music);

      state->recording = true;
    }

    if (IsKeyPressed(KEY_M) && IsMusicReady(state->music)) {
      if (GetMasterVolume() != 0.f) {
        SetMasterVolume(0.f);
      } else {
        SetMasterVolume(1.f);
      }
    }

    if (IsFileDropped()) {
      FilePathList dropped_files = LoadDroppedFiles();

      if (dropped_files.count > 0) {
        if(IsMusicReady(state->music)) {
          DetachAudioStreamProcessor(state->music.stream, frame_callback);
          StopMusicStream(state->music);
          UnloadMusicStream(state->music);
        }

        state->music = LoadMusicStream(dropped_files.paths[0]);
        strcpy(state->music_fp, dropped_files.paths[0]);

        if (IsMusicReady(state->music)) {
          PlayMusicStream(state->music);
          AttachAudioStreamProcessor(state->music.stream, frame_callback);
        }
      }

      UnloadDroppedFiles(dropped_files);
    }
  }

  state->dt = GetFrameTime();
  state->wave_width = clamp(state->wave_width + 0.1f*GetMouseWheelMove(), 1.f, 100.f);

  state->window_position = HMM_V2(GetWindowPosition().x, GetWindowPosition().y);
  state->master_volume = GetMasterVolume();

  f32 max_amp = 0.0f;
  f32 scale = 1.06;
  f32 start_freq = 1.0f;

  u32 sample_count = process_samples(scale, start_freq, 1/(f32)RENDER_FPS);

  BeginDrawing();
  ClearBackground((Color){45, 45, 45, 255});

  if (!state->recording) {
    if (IsMusicReady(state->music)) {
      draw_circle_frequencies(sample_count, state->screen_size);
      draw_waveform(state->screen_size); 
      draw_frequencies(sample_count, state->screen_size);
    } else {
      u32 font_size = state->font.baseSize;

      HMM_Vec2 dim = ray_to_hmmv2(MeasureTextEx(state->font, "Drag & drop music here", font_size, 1));
      DrawTextEx(state->font, "Drag & drop music here", (Vector2){(state->screen_size.Width - dim.Width)/2, (state->screen_size.Height - dim.Height)/2}, font_size, 1, WHITE);
    }
  } else {
      u32 font_size = state->font.baseSize;

      char *text = "Now recording video...";

      HMM_Vec2 dim = ray_to_hmmv2(MeasureTextEx(state->font, text, font_size, 1));
      DrawTextEx(state->font, text, (Vector2){(state->screen_size.Width - dim.Width)/2, (state->screen_size.Height - dim.Height)/2}, font_size, 1, WHITE);

      HMM_Vec2 render_size = HMM_V2(state->screen.texture.width, state->screen.texture.height);

      u32 chunk_size = state->record_data.wave.sampleRate/RENDER_FPS;
      f32 (*frames)[state->record_data.wave.channels] = (void *)state->record_data.wave_samples;
      for (u32 i = 0; i < chunk_size; ++i ) {
        if (state->record_data.wave_cursor < state->record_data.wave.frameCount) {
          push_frame(frames[state->record_data.wave_cursor][0]);
        } else {
          push_frame(0);
        }
        state->record_data.wave_cursor++;
      }

      BeginTextureMode(state->screen); 
      {
        ClearBackground((Color){45, 45, 45, 255});
        draw_circle_frequencies(sample_count, render_size);
        draw_waveform(render_size); 
        draw_frequencies(sample_count, render_size);
      }
      EndTextureMode();

      Image image = LoadImageFromTexture(state->screen.texture);
      {
        ffmpeg_write(state->ffmpeg, image.data, HMM_V2(image.width, image.height));
      }
      UnloadImage(image);
  }

  EndDrawing();	
}

static inline void serialize(State *state) {
  FILE *fptr;
  fptr = fopen("data.ly", "wb");
  {
    fwrite(&state->screen_size, sizeof(HMM_Vec2), 1, fptr);
    fwrite(&state->window_position, sizeof(HMM_Vec2), 1, fptr);
    fwrite(&state->master_volume, sizeof(f32), 1, fptr);
    fputs(state->music_fp, fptr);
  }
  fclose(fptr);
}

static inline void deserialize(State *state) {
  if (access("data.ly", F_OK) == 0) {
    FILE *fptr;
    fptr = fopen("data.ly", "rb");
    {
      fread(&state->screen_size, sizeof(HMM_Vec2), 1, fptr);
      fread(&state->window_position, sizeof(HMM_Vec2), 1, fptr);
      fread(&state->master_volume, sizeof(f32), 1, fptr);
      state->music_fp = fgets(state->music_fp, 128, fptr);
    }
    fclose(fptr);

    SetWindowSize(state->screen_size.Width, state->screen_size.Height);
    SetWindowPosition(state->window_position.X, state->window_position.Y);
    SetMasterVolume(state->master_volume);
  }
}

static inline void fft(f32 in[], u32 stride, float complex out[], u32 n) {
  assert(n > 0);

  if (n == 1) {
    out[0] = in[0];
    return;
  }

  fft(in, stride*2, out, n/2);
  fft(in + stride, stride*2, out + n/2, n/2);

  for (u32 k = 0; k < n/2; ++k) {
    f32 t = (f32)k / n;

    float complex v = cexp(-2*I*PI*t)*out[k + n/2];
    float complex e = out[k];

    out[k] = e + v;
    out[k + n/2] = e - v;
  }
}

static inline f32 amp(float complex z) {
  return logf(creal(z)*creal(z) + cimag(z)*cimag(z));
}

static inline void push_frame(f32 val) {
  memmove(state->samples, state->samples + 1, (FREQ_COUNT - 1)*sizeof(state->samples[0]));
  state->samples[FREQ_COUNT - 1] = val;	
}

static inline void frame_callback(void *buffer_data, u32 n) {
  f32 (*frame_data)[state->music.stream.channels] = buffer_data;

  for (u32 i = 0; i < n; ++i) {
    push_frame(frame_data[i][0]);
  }
}

static inline f32 clamp(f32 v, f32 min, f32 max) {
  if (v < min) {
    v = min;
  } 

  if (v > max) {
    v = max;
  }

  return v;
}

static inline HMM_Vec2 ray_to_hmmv2(Vector2 v) {
  return HMM_V2(v.x, v.y);
}

static inline f32 min(f32 a, f32 b) {
  if (a < b) return a;
  return b;
}

static inline f32 max(f32 a, f32 b) {
  if (a > b) return a;
  return b;
}

static inline void window_samples(f32 *in, f32 *out, u32 length) {
  memcpy(out, in, sizeof(f32)*length);
  for (u32 i = 0; i < length; ++i) {
    f32 t = (f32)i/length;
    f32 window = 0.5 - 0.5*cosf(2*PI*t);
    out[i] *= window;
  }
}

static inline void draw_circle_frequencies(u32 sample_count, HMM_Vec2 render_size) {
  f32 width = 2;
  i32 width_loc = GetShaderLocation(state->circle_lines_shader, "width");
  SetShaderValue(state->circle_lines_shader, width_loc, &width, SHADER_UNIFORM_FLOAT);

  i32 size_loc = GetShaderLocation(state->circle_lines_shader, "size");

  {
    for (u32 i = 0; i < sample_count; i+=10) {
      f32 t = state->smooth_freq[i];

      Color color = (Color){255-t*200, 255-t*125, 255-t*sin(t*10)*200, 255};

      f32 rad = (render_size.Height/2)*t*t;

      Rectangle rec = (Rectangle){render_size.Width/2 - rad, render_size.Height/2 - rad,
        2*rad, 2*rad};

      f32 size = 2*rad;

      SetShaderValue(state->circle_lines_shader, size_loc, &size, SHADER_UNIFORM_FLOAT);

      BeginShaderMode(state->circle_lines_shader); 
      DrawTextureEx(state->default_tex, (Vector2){rec.x, rec.y}, 0, 2*rad, color);
      EndShaderMode();
    }
  }
}

static inline void draw_waveform(HMM_Vec2 render_size) {
  u32 count = render_size.Width;
  f32 cell_width = ceilf((f32)render_size.Width/count);

  if (cell_width < 1.f) cell_width = 1.f;
  for (u32 i = 0; i < count; ++i) {
    f32 a = ((f32)i)/((f32)count);
    i32 index = (i32)(ceilf(FREQ_COUNT - a*FREQ_COUNT/state->wave_width));

    f32 t = state->samples[index];

    if (t > 0) {
      DrawRectangle(i*cell_width, (render_size.Height/2)*(1 - t), cell_width, render_size.Height/2*t, RED);
    } else {
      DrawRectangle(i*cell_width, render_size.Height/2, cell_width, -render_size.Height/2*t, RED);
    }
  }
}

static inline void draw_frequencies(u32 sample_count, HMM_Vec2 render_size) {
  f32 cell_width = ceilf((f32)render_size.Width/((f32)sample_count));

  u32 position_count = sample_count;
  HMM_Vec2 positions[position_count];

  Color colors[position_count];

  u32 vertex_count = position_count;
  HMM_Vec2 vertices[vertex_count];

  for (u32 i = 0; i < position_count; ++i) {
    f32 t = state->smooth_freq[i];

    if (t < 0) t = 0;

    Color color = (Color){
      t*200, 
        t*125, 
        sin(t*GetTime())*50 + 200, 
        255
    };

    colors[i] = color;
    positions[i] = HMM_V2(i * cell_width, render_size.Height*(1-0.5*t));
  }

  for (u32 i = 0; i < position_count; ++i) {
    HMM_Vec2 position = positions[i];


    vertices[i] = HMM_SubV2(position, HMM_V2(render_size.Width/2.f, render_size.Height*(3.f/4.f)));
    vertices[i] = HMM_DivV2(vertices[i], HMM_V2(render_size.Width, render_size.Height/2.f));
    vertices[i] = HMM_AddV2(vertices[i], HMM_V2(0.5f, 0.5f));
  }
  
  rlSetTexture(state->default_tex.id);

  // Texturing is only supported on RL_QUADS
  rlBegin(RL_QUADS);
  {
    rlColor4ub(1, 1, 255, 255);
    for (int i = 0; i < vertex_count - 1; i++)
    {
      rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      rlTexCoord2f(vertices[i].X, 1.f);
      rlVertex2f(positions[i].X, render_size.Height);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(vertices[i + 1].X, 1.f);
      rlVertex2f(positions[i + 1].X, render_size.Height);

      rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      rlTexCoord2f(vertices[i].X, vertices[i].Y);
      rlVertex2f(positions[i].X, positions[i].Y);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(vertices[i + 1].X, vertices[i + 1].Y);
      rlVertex2f(positions[i + 1].X, positions[i + 1].Y);

      rlColor4ub(colors[i + 1].r, colors[i + 1].g, colors[i + 1].b, colors[i + 1].a);
      rlTexCoord2f(vertices[i + 1].X, 1.f);
      rlVertex2f(positions[i + 1].X, render_size.Height);

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

static inline u32 process_samples(f32 scale, f32 start_freq, f32 dt) {
  f32 max_amp = 0.0f;
    
  window_samples(state->samples, state->window_buffer, FREQ_COUNT);
  fft(state->window_buffer, 1, state->freq, FREQ_COUNT);

  for (u32 i = 0; i < FREQ_COUNT/2; ++i) {
    f32 c = amp(state->freq[i]);
    if (max_amp < c) {
      max_amp = c;
    }
  }

  u32 sample_count = logf((0.5f*(f32)FREQ_COUNT)/start_freq)/logf(scale);
  f32 log_freq[sample_count];

  for (u32 i = 0; i < sample_count; ++i) {
    u32 f = powf(scale, i)*start_freq;
    u32 f1 = powf(scale, i + 1)*start_freq;

    f32 a = amp(state->freq[f]);
    u32 m = 0;
    for (u32 q = f; q < FREQ_COUNT/2 && q < f*scale; ++q) {
      if (amp(state->freq[q]) > a) a = amp(state->freq[q]);
    }

    log_freq[i] = a/max_amp;
  }


  for (u32 i = 0; i < sample_count; ++i) {
    if (log_freq[i] > -FLT_MAX) {
      state->smooth_freq[i] += 10*(log_freq[i] - state->smooth_freq[i])*dt;
    }
  }

  return sample_count;
}
