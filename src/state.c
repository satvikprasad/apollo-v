#include "state.h"
#include "defines.h"
#include "handmademath.h"
#include "raylib.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

State *state;

static inline void serialize(State *state);
static inline void deserialize(State *state);

static inline void fft(f32 in[], u32 stride, float complex out[], u32 n);
static inline f32 amp(float complex z);
static inline f32 clamp(f32 v, f32 min, f32 max);
static inline void frame_callback(void *buffer_data, u32 n);
static inline f32 min(f32 a, f32 b);
static inline f32 max(f32 a, f32 b);
static inline HMM_Vec2 ray_to_hmmv2(Vector2 v);

void state_initialise(const char *fp) {
  state = malloc(sizeof(State));
  memset(state, 0, sizeof(State));

  deserialize(state);

  state->font = LoadFontEx("assets/fonts/helvetica.ttf", 20, NULL, 0);
  state->music = LoadMusicStream(fp);
  state->wave_width = 10;
  assert(state->music.stream.sampleSize == 32);
  assert(state->music.stream.channels == 2);

  PlayMusicStream(state->music);
  AttachAudioStreamProcessor(state->music.stream, frame_callback);
}

void state_destroy() {
  serialize(state);
}

void *state_detach() {
  if(IsMusicReady(state->music)) {
    DetachAudioStreamProcessor(state->music.stream, frame_callback);
  }

  return state;
}

void state_attach(void *stateptr) {
  state = stateptr;

  if(IsMusicReady(state->music)) {
    AttachAudioStreamProcessor(state->music.stream, frame_callback);
  }
}

void state_update() {
  if (IsMusicReady(state->music)) {
    UpdateMusicStream(state->music);
  }

  if(IsKeyPressed(KEY_SPACE) && IsMusicReady(state->music)) {
    if (IsMusicStreamPlaying(state->music)) {
      PauseMusicStream(state->music);
    } else {
      ResumeMusicStream(state->music);
    }
  }

  if(IsKeyPressed(KEY_M) && IsMusicReady(state->music)) {
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

      if (IsMusicReady(state->music)) {
        PlayMusicStream(state->music);
        AttachAudioStreamProcessor(state->music.stream, frame_callback);
      }
    }

    UnloadDroppedFiles(dropped_files);
  }

  state->wave_width = clamp(state->wave_width + 0.1f*GetMouseWheelMove(), 1.f, 100.f);

  state->screen_size = HMM_V2(GetRenderWidth(), GetRenderHeight());
  state->window_position = HMM_V2(GetWindowPosition().x, GetWindowPosition().y);
  state->master_volume = GetMasterVolume();

  f32 max_amp = 0.0f;
  f32 scale = 1.03;
  u32 bars = 0;

  f32 windowed_samples[FREQ_COUNT];

  memcpy(windowed_samples, state->samples, sizeof(f32)*FREQ_COUNT);
  for (u32 i = 0; i < FREQ_COUNT; ++i) {
    f32 t = (f32)i/FREQ_COUNT;
    f32 window = 0.5 - 0.5*cosf(2*PI*t);
    windowed_samples[i] *= window;
  }
  
  fft(windowed_samples, 1, state->freq, FREQ_COUNT);

  for (u32 i = 0; i < FREQ_COUNT; ++i) {
    f32 c = amp(state->freq[i]);
    if (max_amp < c) {
      max_amp = c;
    }
  }

  for (f32 f = 1.f; f < FREQ_COUNT/2; f = ceilf(scale*f)) {
    ++bars;
  }

  BeginDrawing();
  ClearBackground((Color){10, 10, 10, 255});

  if (IsMusicReady(state->music)) {
    u32 count = 1024;
    f32 cell_width = (f32)state->screen_size.Width/count;

    if (cell_width < 1.f) cell_width = 1.f;
    for (u32 i = 0; i < count; ++i) {
      f32 a = ((f32)i)/((f32)count);
      i32 index = (i32)(FREQ_COUNT - a*FREQ_COUNT/state->wave_width);

      f32 t = state->samples[index];

      if (t > 0) {
        DrawRectangle(i*cell_width, (state->screen_size.Height/2)*(1 - t), cell_width, state->screen_size.Height/2*t, RED);
      } else {
        DrawRectangle(i*cell_width, state->screen_size.Height/2, cell_width, -state->screen_size.Height/2*t, RED);
      }
    }

    cell_width = (f32)GetScreenWidth()/((f32)bars);
    if (cell_width < 1.f) cell_width = 1.f;

    i32 m = 0;
    f32 prev_t = 0.0f;
    for (f32 f = 1.f; f < FREQ_COUNT/2; f = ceilf(scale*f)) {
      u32 i = f;

      f32 a = 0.f;
      for (u32 q = f; q < FREQ_COUNT/2 && q < f*scale; ++q) {
        if (amp(state->freq[q]) > a) a = amp(state->freq[q]);
      }

      f32 t = a/max_amp;

      Color color = (Color){
          t*200, 
          t*125, 
          sin(t*GetTime())*50 + 200, 
          255
          };

//      DrawLine((m)*cell_width, state->screen_size.Height*(1-0.5*prev_t), 
 //         (m+1)*cell_width, state->screen_size.Height*(1-0.5*t), color);

      color.a = 100;

      DrawRectangle(m*cell_width, 
          state->screen_size.Height*(1-0.5*min(t, prev_t)), 
          cell_width, 
          state->screen_size.Height, 
          color);

      if (t > prev_t) {
        DrawTriangle((Vector2){m*cell_width, state->screen_size.Height*(1-0.5*min(t, prev_t))}, 
            (Vector2){(m+1)*cell_width, state->screen_size.Height*(1-0.5*min(t, prev_t))}, 
            (Vector2){(m+1)*cell_width, state->screen_size.Height*(1-0.5*max(t, prev_t))}, color);
      } else {
        DrawTriangle((Vector2){(m)*cell_width, state->screen_size.Height*(1-0.5*min(t, prev_t))}, 
            (Vector2){(m+1)*cell_width, state->screen_size.Height*(1-0.5*min(t, prev_t))}, 
            (Vector2){(m)*cell_width, state->screen_size.Height*(1-0.5*max(t, prev_t))}, color);
      }


      if (i % 10 == 0) {
        DrawCircleLines(state->screen_size.Width/2, 
            state->screen_size.Height/2, 
            (state->screen_size.Height/2)*min(t, prev_t), 
            (Color){t*200, t*125, t*sin(t*10*GetTime())*200, 255});
      }
      prev_t = t;
      ++m;
    }
  } else {
    u32 font_size = state->font.baseSize;

    HMM_Vec2 dim = ray_to_hmmv2(MeasureTextEx(state->font, "Drag & drop music here", font_size, 1));
    DrawTextEx(state->font, "Drag & drop music here", (Vector2){(state->screen_size.Width - dim.Width)/2, (state->screen_size.Height - dim.Height)/2}, font_size, 1, WHITE);
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

static inline void frame_callback(void *buffer_data, u32 n) {
  f32 (*frame_data)[state->music.stream.channels] = buffer_data;

  for (u32 i = 0; i < n; ++i) {
    memmove(state->samples, state->samples + 1, (FREQ_COUNT - 1)*sizeof(state->samples[0]));
    state->samples[FREQ_COUNT - 1] = frame_data[i][0];	
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
