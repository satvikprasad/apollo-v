#include "state.h"
#include "defines.h"
#include "renderer.h"
#include "handmademath.h"
#include "ffmpeg.h"
#include "signals.h"

#include <assert.h>
#include <raylib.h>
#include <rlgl.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

State *state;

static inline void serialize();
static inline void deserialize();
static inline void update();
static inline void render();

static inline void draw_textured_poly(Texture2D texture, HMM_Vec2 center, HMM_Vec2 *points, HMM_Vec2 *texcoords, int pointCount, Color tint);
static inline void push_frame(f32 val);
static inline f32 clamp(f32 v, f32 min, f32 max);
static inline void frame_callback(void *buffer_data, u32 n);
static inline f32 min(f32 a, f32 b);
static inline f32 max(f32 a, f32 b);
static inline HMM_Vec2 ray_to_hmmv2(Vector2 v);

void state_initialise(const char *fp) {
  state = malloc(sizeof(State));
  memset(state, 0, sizeof(State));

  state->music_fp = malloc(sizeof(char) * 128);

  deserialize();

  state->font = LoadFontEx("assets/fonts/helvetica.ttf", 20, NULL, 0);
  state->music = LoadMusicStream(state->music_fp);
  state->wave_width = 10;
  state->screen = LoadRenderTexture(1280, 720);

  signals_process_samples(LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL, &state->frequency_count, 0);
  state->frequencies = calloc(state->frequency_count, sizeof(f32));

  renderer_initialise(&state->renderer);

  if (IsMusicReady(state->music)) {
    PlayMusicStream(state->music);
    AttachAudioStreamProcessor(state->music.stream, frame_callback);
  }
}

void state_destroy() {
  serialize();
  renderer_destroy(&state->renderer);

  UnloadFont(state->font);
  UnloadMusicStream(state->music);
  UnloadRenderTexture(state->screen);

  free(state->music_fp);
  free(state);
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

  renderer_attach(&state->renderer);
}

void state_update() {
  update();

  BeginDrawing();
  ClearBackground((Color){45, 45, 45, 255});

  if (!state->recording) {
    if (IsMusicReady(state->music)) {
      renderer_set_render_size(&state->renderer, state->screen_size);

      render();
    } else {
      u32 font_size = state->font.baseSize;

      HMM_Vec2 dim = ray_to_hmmv2(MeasureTextEx(state->font, "Drag & drop music here", font_size, 1));
      DrawTextEx(state->font, "Drag & drop music here", (Vector2){(state->screen_size.Width - dim.Width)/2, (state->screen_size.Height - dim.Height)/2}, font_size, 1, WHITE);
    }
  } else {
      HMM_Vec2 render_size = HMM_V2(state->screen.texture.width, state->screen.texture.height);

      u32 font_size = state->font.baseSize;

      char *text = "Now recording video...";

      HMM_Vec2 dim = ray_to_hmmv2(MeasureTextEx(state->font, text, font_size, 1));
      DrawTextEx(state->font, text, (Vector2){(state->screen_size.Width - dim.Width)/2, (state->screen_size.Height - dim.Height)/2}, font_size, 1, WHITE);

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
        renderer_set_render_size(&state->renderer, render_size);

        ClearBackground((Color){45, 45, 45, 255});

        render();
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

static inline void serialize() {
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

static inline void deserialize() {
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

static inline void update() {
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
      memset(state->samples, 0, sizeof(f32)*SAMPLE_COUNT);
      memset(state->frequencies, 0, sizeof(f32)*1000);

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

  u32 freq_count;
  signals_process_samples(LOG_MUL, START_FREQ, state->samples, SAMPLE_COUNT, state->frequencies, &freq_count, 1/(f32)RENDER_FPS);
  assert(state->frequency_count == freq_count);
}

static inline void render() {
  renderer_draw_circle_frequencies(&state->renderer, state->frequency_count, state->frequencies);
  renderer_draw_waveform(&state->renderer, SAMPLE_COUNT, state->samples, state->wave_width); 
  renderer_draw_frequencies(&state->renderer, state->frequency_count, state->frequencies);
}

static inline void push_frame(f32 val) {
  memmove(state->samples, state->samples + 1, (SAMPLE_COUNT - 1)*sizeof(state->samples[0]));
  state->samples[SAMPLE_COUNT - 1] = val;	
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

