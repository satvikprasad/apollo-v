#include "state.h"

#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"
#include "ffmpeg.h"
#include "handmademath.h"
#include "hashmap.h"
#include "lmath.h"
#include "parameter.h"
#include "renderer.h"
#include "signals.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

static void serialize();
static void deserialize();
static void render_ui();
static void render();

static void begin_recording();
static void end_recording();
static void update_recording();

static void update_frequencies();
static b8 load_dropped_files();

static void push_frame(f32 val, f32 *samples, u32 sample_count);
static void frame_callback(void *buffer_data, u32 n);

State *state;

void state_initialise() {
  state = malloc(sizeof(State));
  memset(state, 0, sizeof(State));

  state->music_fp = malloc(sizeof(char) * 128);

  // Initialise parameters
  {
    state->parameters = parameters_create();

    parameter_set(state->parameters, "wave_width", 10.f);
    parameter_set(state->parameters, "smoothing", 5.0f);
  }

  deserialize();

  state->font = LoadFontEx("assets/fonts/helvetica.ttf", 20, NULL, 0);
  state->music = LoadMusicStream(state->music_fp);
  state->screen = LoadRenderTexture(1280, 720);

  GuiSetFont(state->font);

  state->filter = calloc(5, sizeof(f32));

  for (i32 i = -2; i < 3; ++i) {
    state->filter[i + 2] = expf(-i * i);
  }

  state->filter_count = 5;

  signals_process_samples(LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL,
                          &state->frequency_count, 0, 0, state->filter,
                          state->filter_count);
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

  hashmap_free(state->parameters);

  UnloadFont(state->font);
  UnloadMusicStream(state->music);
  UnloadRenderTexture(state->screen);

  free(state->music_fp);
  free(state->frequencies);
  free(state);
}

void *state_detach() {
  if (IsMusicReady(state->music)) {
    DetachAudioStreamProcessor(state->music.stream, frame_callback);
  }

  renderer_detach(&state->renderer);

  return state;
}

void state_attach(void *stateptr) {
  state = stateptr;

  if (IsMusicReady(state->music)) {
    AttachAudioStreamProcessor(state->music.stream, frame_callback);
  }

  renderer_attach(&state->renderer);
}

void state_update() {
  switch (state->condition) {
    case StateCondition_NORMAL:
      if (!IsMusicReady(state->music)) {
        state->condition = StateCondition_LOAD;
        break;
      }

      UpdateMusicStream(state->music);

      // Smoothing controls
      if (IsKeyPressed(KEY_MINUS)) {
        if (parameter_get(state->parameters, "smoothing") != 0) {
          parameter_set(state->parameters, "smoothing",
                        parameter_get(state->parameters, "smoothing") - 1);
        }
      }

      if (IsKeyPressed(KEY_EQUAL) && IsKeyDown(KEY_LEFT_SHIFT)) {
        parameter_set(state->parameters, "smoothing",
                      parameter_get(state->parameters, "smoothing") + 1);

        if (parameter_get(state->parameters, "smoothing") < 0) {
          parameter_set(state->parameters, "smoothing", 0);
        }
      }

      if (IsKeyPressed(KEY_SPACE)) {
        if (IsMusicStreamPlaying(state->music)) {
          PauseMusicStream(state->music);
        } else {
          ResumeMusicStream(state->music);
        }
      }

      if (IsWindowResized()) {
        state->screen_size = HMM_V2(GetRenderWidth(), GetRenderHeight());
      }

      if (IsKeyPressed(KEY_M) && IsMusicReady(state->music)) {
        if (GetMasterVolume() != 0.f) {
          SetMasterVolume(0.f);
        } else {
          SetMasterVolume(1.f);
        }
      }

      if (IsFileDropped()) {
        if (!load_dropped_files()) {
          state->condition = StateCondition_LOAD;
        }
      }

      if (IsKeyPressed(KEY_B) && IsMusicReady(state->music)) {
        begin_recording();
        state->condition = StateCondition_RECORDING;
      }

      signals_process_samples(
          LOG_MUL, START_FREQ, state->samples, SAMPLE_COUNT, state->frequencies,
          &state->frequency_count, state->dt,
          (u32)parameter_get(state->parameters, "smoothing"), state->filter,
          state->filter_count);

      break;

    case StateCondition_ERROR:
      break;

    case StateCondition_LOAD:
      if (IsFileDropped()) {
        if (load_dropped_files()) {
          state->condition = StateCondition_NORMAL;
        }
      }
      break;

    case StateCondition_RECORDING:
      if (state->record_data.wave_cursor >=
          state->record_data.wave.frameCount) {
        end_recording();
        state->condition = StateCondition_NORMAL;
      }

      update_recording();
      break;
  }

  state->dt = GetFrameTime();
  parameter_set(state->parameters, "wave_width",
                clamp(parameter_get(state->parameters, "wave_width") +
                          0.1f * GetMouseWheelMove(),
                      1.f, 100.f));
  state->window_position = HMM_V2(GetWindowPosition().x, GetWindowPosition().y);
  state->master_volume = GetMasterVolume();

  update_frequencies();
}

void state_render() {
  Color clear = (Color){15, 15, 15, 255};

  BeginDrawing();
  ClearBackground(clear);

  switch (state->condition) {
    case StateCondition_NORMAL:
      if (!IsMusicReady(state->music)) {
        state->condition = StateCondition_LOAD;
        break;
      }
      renderer_set_render_size(&state->renderer, state->screen_size);

      render();
      render_ui();
      break;

    case StateCondition_LOAD:
      renderer_draw_text_center(
          state->font, "Drag & drop music to play",
          HMM_V2(state->screen_size.Width / 2, state->screen_size.Height / 2));
      break;

    case StateCondition_RECORDING:
      assert(IsRenderTextureReady(state->screen));

      HMM_Vec2 render_size =
          HMM_V2(state->screen.texture.width, state->screen.texture.height);

      renderer_draw_text_center(
          state->font, "Now recording...",
          HMM_V2(state->screen_size.Width / 2, state->screen_size.Height / 2));

      BeginTextureMode(state->screen);
      {
        renderer_set_render_size(&state->renderer, render_size);

        ClearBackground(clear);

        render();
      }
      EndTextureMode();

      Image image = LoadImageFromTexture(state->screen.texture);
      { ffmpeg_write(state->ffmpeg, image.data, render_size); }
      UnloadImage(image);
      break;

    default:
      printf("Unhandled case\n");
      break;
  }

  EndDrawing();
}

static void serialize() {
  FILE *fptr;
  fptr = fopen("data.ly", "wb");
  {
    fwrite(&state->screen_size, sizeof(HMM_Vec2), 1, fptr);
    fwrite(&state->window_position, sizeof(HMM_Vec2), 1, fptr);
    fwrite(&state->master_volume, sizeof(f32), 1, fptr);

    u32 smoothing = (u32)parameter_get(state->parameters, "smoothing");
    fwrite(&smoothing, sizeof(u32), 1, fptr);

    fputs(state->music_fp, fptr);
  }
  fclose(fptr);
}

static void deserialize() {
  if (access("data.ly", F_OK) == 0) {
    FILE *fptr;
    fptr = fopen("data.ly", "rb");
    {
      fread(&state->screen_size, sizeof(HMM_Vec2), 1, fptr);
      fread(&state->window_position, sizeof(HMM_Vec2), 1, fptr);
      fread(&state->master_volume, sizeof(f32), 1, fptr);

      u32 smoothing = 0;
      fread(&smoothing, sizeof(u32), 1, fptr);
      parameter_set(state->parameters, "smoothing", smoothing);

      state->music_fp = fgets(state->music_fp, 128, fptr);
    }
    fclose(fptr);

    SetWindowSize(state->screen_size.Width, state->screen_size.Height);
    SetWindowPosition(state->window_position.X, state->window_position.Y);
    SetMasterVolume(state->master_volume);
  }
}

static void render_parameter_slider(char *name, Rectangle rect, char *text_left,
                                    char *text_right, f32 min, f32 max) {
  f32 val = parameter_get(state->parameters, name);

  GuiSlider(rect, text_left, text_right, &val, min, max);

  parameter_set(state->parameters, name, val);
}

static void render_ui() {
  render_parameter_slider("wave_width", (Rectangle){100, 10, 200, 20},
                          "Wave width", "", 1, 100);

  render_parameter_slider("smoothing", (Rectangle){100, 40, 200, 20},
                          "Smoothing", "", 0, 10);

  if (GuiButton((Rectangle){310, 20, 25, 25}, "#11#")) {
    begin_recording();
    state->condition = StateCondition_RECORDING;
  }
}

static void render() {
  renderer_draw_circle_frequencies(&state->renderer, state->frequency_count,
                                   state->frequencies,
                                   state->renderer.default_color_func);
  renderer_draw_waveform(&state->renderer, SAMPLE_COUNT, state->samples,
                         parameter_get(state->parameters, "wave_width"));
  renderer_draw_frequencies(&state->renderer, state->frequency_count,
                            state->frequencies, true,
                            state->renderer.default_color_func);
}

static void begin_recording() {
  memset(state->samples, 0, sizeof(f32) * SAMPLE_COUNT);
  memset(state->frequencies, 0, sizeof(f32) * state->frequency_count);

  HMM_Vec2 render_size =
      HMM_V2(state->screen.texture.width, state->screen.texture.height);

  state->ffmpeg = ffmpeg_start(render_size, RENDER_FPS, state->music_fp);
  state->record_start = GetTime();

  state->record_data.wave = LoadWave(state->music_fp);
  state->record_data.wave_samples = LoadWaveSamples(state->record_data.wave);
  state->record_data.wave_cursor = 0;

  PauseMusicStream(state->music);
}

static void end_recording() {
  ffmpeg_end(state->ffmpeg);

  UnloadWave(state->record_data.wave);
  UnloadWaveSamples(state->record_data.wave_samples);

  ResumeMusicStream(state->music);
}

static void update_recording() {
  u32 chunk_size = state->record_data.wave.sampleRate / RENDER_FPS;
  f32(*frames)[state->record_data.wave.channels] =
      (void *)state->record_data.wave_samples;
  for (u32 i = 0; i < chunk_size; ++i) {
    if (state->record_data.wave_cursor < state->record_data.wave.frameCount) {
      push_frame(frames[state->record_data.wave_cursor][0], state->samples,
                 SAMPLE_COUNT);
    } else {
      push_frame(0, state->samples, SAMPLE_COUNT);
    }
    state->record_data.wave_cursor++;
  }

  signals_process_samples(LOG_MUL, START_FREQ, state->samples, SAMPLE_COUNT,
                          state->frequencies, &state->frequency_count,
                          1 / (f32)RENDER_FPS,
                          (u32)parameter_get(state->parameters, "smoothing"),
                          state->filter, state->filter_count);
}

static void update_frequencies() {
  u32 freq_count;
  signals_process_samples(LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL,
                          &freq_count, 0,
                          (u32)parameter_get(state->parameters, "smoothing"),
                          state->filter, state->filter_count);

  if (freq_count != state->frequency_count) {
    state->frequencies = realloc(state->frequencies, freq_count * sizeof(f32));

    if (freq_count > state->frequency_count) {
      memset(state->frequencies + state->frequency_count, 0,
             (freq_count - state->frequency_count) * sizeof(f32));
    }
  }
}

static b8 load_dropped_files() {
  b8 ret = false;

  FilePathList dropped_files = LoadDroppedFiles();

  if (dropped_files.count > 0) {
    if (IsMusicReady(state->music)) {
      DetachAudioStreamProcessor(state->music.stream, frame_callback);
      StopMusicStream(state->music);
      UnloadMusicStream(state->music);
    }

    state->music = LoadMusicStream(dropped_files.paths[0]);
    strcpy(state->music_fp, dropped_files.paths[0]);

    if (IsMusicReady(state->music)) {
      PlayMusicStream(state->music);
      AttachAudioStreamProcessor(state->music.stream, frame_callback);
      ret = true;
    }
  }

  UnloadDroppedFiles(dropped_files);

  return ret;
}

static void push_frame(f32 val, f32 *samples, u32 sample_count) {
  memmove(samples, samples + 1, (sample_count - 1) * sizeof(f32));
  samples[sample_count - 1] = val;
}

static void frame_callback(void *buffer_data, u32 n) {
  f32(*frame_data)[state->music.stream.channels] = buffer_data;

  for (u32 i = 0; i < n; ++i) {
    push_frame(frame_data[i][0], state->samples, SAMPLE_COUNT);
  }
}
