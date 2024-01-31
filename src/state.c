#include "state.h"

#include <math.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api.h"
#include "arena.h"
#include "defines.h"
#include "ffmpeg.h"
#include "handmademath.h"
#include "json.h"
#include "lmath.h"
#include "parameter.h"
#include "permanent_storage.h"
#include "renderer.h"
#include "server.h"
#include "signals.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

static void Serialize();
static bool Deserialize();
static void RenderUI();
static void Render();

static void BeginRecording();
static void EndRecording();
static void UpdateRecording();

static void SetFrequencyCount();
static B8   GetDroppedFiles();

static void FrameCallback(void *buffer_data, U32 n);

static StateMemory memory;
static State      *state;

static StateFont LoadStateFont(const char *fp) {
    StateFont font;

    U32 incr = MAX_FONT_SIZE / FONT_SIZES_PER_FONT;

    for (U32 i = 0; i < FONT_SIZES_PER_FONT; ++i) {
        font.fonts[i] = LoadFontEx(fp, (i + 1) * incr, NULL, 0);
    }

    return font;
}

static void UnloadStateFont(StateFont font) {
    for (U32 i = 0; i < FONT_SIZES_PER_FONT; ++i) {
        UnloadFont(font.fonts[i]);
    }
}

static void GetTotalTimeCallback(void *user_data, char *response) {
    State *state = (State *)user_data;

    char buf[256];
    JsonObjectGetKey(response, "time", buf);

    state->total_time = atof(buf);
}

void StateInitialise() {
    memory.permanent_storage_size = 1024 * 1024 * 1024;
    memory.permanent_storage =
        PermanentStorageInit(memory.permanent_storage_size);

    state = (State *)memory.permanent_storage;

    ArenaInitialise(&state->arena,
                    memory.permanent_storage_size - sizeof(State),
                    (U8 *)memory.permanent_storage + sizeof(State));

    state->api_data = ArenaPushStruct(&state->arena, ApiData);
    state->renderer_data = ArenaPushStruct(&state->arena, RendererData);
    state->loopback_data = ArenaPushStruct(&state->arena, LoopbackData);
    state->server_data = ArenaPushStruct(&state->arena, ServerData);

    // Initialise default parameters
    {
        state->parameters = ParameterCreate();

        ParameterSet(state->parameters, &(Parameter){.name = "wave_width",
                                                     .value = 10.f,
                                                     .min = 1,
                                                     .max = 100});
        ParameterSet(state->parameters, &(Parameter){.name = "smoothing",
                                                     .value = 5.0f,
                                                     .min = 1,
                                                     .max = 10});
        ParameterSet(state->parameters, &(Parameter){.name = "velocity",
                                                     .value = 10.f,
                                                     .min = 1,
                                                     .max = 100});
    }

    if (Deserialize()) {
        if (!FileExists(state->music_fp) || strlen(state->music_fp) == 0) {
            strcpy(state->music_fp, "assets/monks.mp3");
        }

        state->music = LoadMusicStream(state->music_fp);
    }

    if (state->screen_size.Width <= 50.0f ||
        state->screen_size.Height <= 50.0f) {
        state->screen_size.Width = 1280;
        state->screen_size.Height = 720;
    }

    SetWindowSize(state->screen_size.Width, state->screen_size.Height);
    SetWindowPosition(state->window_position.X, state->window_position.Y);
    SetMasterVolume(state->master_volume);

    state->font = LoadStateFont("assets/fonts/helvetica.ttf");

    GuiSetFont(FontClosestToSize(state->font, 10));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 10);

    for (I32 i = -2; i < 3; ++i) {
        state->filter[i + 2] = expf(-i * i);
    }

    state->filter_count = 5;

    SignalsProcessSamples(LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL,
                          &state->frequency_count, 0, 0, state->filter,
                          state->filter_count,
                          ParameterGetValue(state->parameters, "velocity"));

    if (IsMusicReady(state->music)) {
        PlayMusicStream(state->music);
        AttachAudioStreamProcessor(state->music.stream, FrameCallback);
    }

    RendererInitialise(state->renderer_data);
    ApiInitialise("lua/init.lua", state, state->api_data);
    LoopbackInitialise(state->loopback_data, state);
    ServerInitialise(state->server_data, API_URI);

    ServerGetAsync(state->server_data, "metrics/total-time", state,
                   GetTotalTimeCallback, &state->arena);
}

void StateDestroy() {
    F64 elapsed = GetTime();

    ServerPostAsync(state->server_data, "add-metric",
                    TextFormat("{\"time\": %f}", elapsed), NULL, NULL,
                    &state->arena);

    ServerWait(state->server_data);

    Serialize();

    ServerDestroy(state->server_data);

    ApiDestroy(state->api_data);

    LoopbackDestroy(state->loopback_data);

    RendererDestroy(state->renderer_data);

    ParameterDestroy(state->parameters);

    UnloadStateFont(state->font);
    UnloadMusicStream(state->music);

    PermanentStorageDestroy(memory.permanent_storage);
}

void StateUpdate() {
    ApiUpdate(state->api_data, state);

    SetFrequencyCount();

    switch (state->condition) {
    case StateCondition_NORMAL:
        ApiPreUpdate(state->api_data, state);

        if (!IsMusicReady(state->music)) {
            state->condition = StateCondition_LOAD;
            break;
        }

        UpdateMusicStream(state->music);

        if (IsKeyPressed(KEY_L)) {
            if (state->loopback) {
                AttachAudioStreamProcessor(state->music.stream, FrameCallback);
                ResumeMusicStream(state->music);
                state->loopback = false;
            } else {
                DetachAudioStreamProcessor(state->music.stream, FrameCallback);
                PauseMusicStream(state->music);
                state->loopback = true;
            }
        }

        // Smoothing controls
        if (IsKeyPressed(KEY_MINUS)) {
            if (ParameterGetValue(state->parameters, "smoothing") != 0) {
                ParameterSetValue(
                    state->parameters, "smoothing",
                    ParameterGetValue(state->parameters, "smoothing") - 1);
            }
        }

        if (IsKeyPressed(KEY_EQUAL) && IsKeyDown(KEY_LEFT_SHIFT)) {
            ParameterSetValue(
                state->parameters, "smoothing",
                ParameterGetValue(state->parameters, "smoothing") + 1);

            if (ParameterGetValue(state->parameters, "smoothing") < 0) {
                ParameterSetValue(state->parameters, "smoothing", 0);
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
            if (!GetDroppedFiles()) {
                state->condition = StateCondition_LOAD;
            }
        }

        if (IsKeyPressed(KEY_B) && IsMusicReady(state->music)) {
            BeginRecording();
            state->condition = StateCondition_RECORDING;
        }

        SignalsProcessSamples(
            LOG_MUL, START_FREQ, state->samples, SAMPLE_COUNT,
            state->frequencies, &state->frequency_count, state->dt,
            (U32)ParameterGetValue(state->parameters, "smoothing"),
            state->filter, state->filter_count,
            ParameterGetValue(state->parameters, "velocity"));

        break;

    case StateCondition_ERROR:
        break;

    case StateCondition_LOAD:
        if (IsFileDropped()) {
            if (GetDroppedFiles()) {
                state->condition = StateCondition_NORMAL;
            }
        }
        break;

    case StateCondition_RECORDING:
        if (state->record_data.wave_cursor >=
            state->record_data.wave.frameCount) {
            EndRecording();
            state->condition = StateCondition_NORMAL;
        }

        UpdateRecording();
        break;
    }

    state->dt = GetFrameTime();
    ParameterSetValue(
        state->parameters, "wave_width",
        ClampF32(ParameterGetValue(state->parameters, "wave_width") +
                     0.1f * GetMouseWheelMove(),
                 1.f, 100.f));
    state->window_position =
        HMM_V2(GetWindowPosition().x, GetWindowPosition().y);
    state->master_volume = GetMasterVolume();
}

void StateRender() {
    Color clear = state->api_data->data.opt.bg_color;

    BeginDrawing();
    ClearBackground(clear);

    switch (state->condition) {
    case StateCondition_NORMAL:
        if (!IsMusicReady(state->music)) {
            state->condition = StateCondition_LOAD;
            break;
        }
        RendererSetRenderSize(state->renderer_data, state->screen_size);

        Render();
        RenderUI();
        break;

    case StateCondition_LOAD:
        RendererDrawTextCenter(MediumFont(state->font),
                               "Drag & drop music to play",
                               HMM_V2(state->screen_size.Width / 2,
                                      state->screen_size.Height / 2));
        break;

    case StateCondition_RECORDING:
        RendererDrawTextCenter(XLargeFont(state->font), "Now recording...",
                               HMM_V2(state->screen_size.Width / 2,
                                      state->screen_size.Height / 2));

        RendererBeginRecording(state->renderer_data);
        {
            ClearBackground(clear);

            Render();
        }
        RendererEndRecording(state->renderer_data, state->ffmpeg);

        break;

    default:
        printf("Unhandled case\n");
        break;
    }

    EndDrawing();
}

static void WriteString(const char *str, FILE *fptr) {
    U32 len = strlen(str);

    fwrite(&len, sizeof(U32), 1, fptr);
    fwrite(str, sizeof(char), len + 1, fptr);
}

static void ReadString(char *str, FILE *fptr) {
    U32 len;
    fread(&len, sizeof(U32), 1, fptr);
    fread(str, sizeof(char), len + 1, fptr);
}

static void Serialize() {
    FILE *fptr;
    fptr = fopen("data.ly", "wb");
    {
        fwrite(&state->screen_size, sizeof(HMM_Vec2), 1, fptr);
        fwrite(&state->window_position, sizeof(HMM_Vec2), 1, fptr);
        fwrite(&state->master_volume, sizeof(F32), 1, fptr);

        WriteString(state->music_fp, fptr);

        U32 param_count = ParameterCount(state->parameters);
        fwrite(&param_count, sizeof(U32), 1, fptr);

        Parameter *parameter;
        U32        _ = 0;
        while (ParameterIter(state->parameters, &_, &parameter)) {
            WriteString(parameter->name, fptr);
            fwrite(&parameter->value, sizeof(F32), 1, fptr);
            fwrite(&parameter->min, sizeof(F32), 1, fptr);
            fwrite(&parameter->max, sizeof(F32), 1, fptr);
        }
    }
    fclose(fptr);
}

static bool Deserialize() {
    if (access("data.ly", F_OK) == 0) {
        FILE *fptr;
        fptr = fopen("data.ly", "rb");
        {
            fread(&state->screen_size, sizeof(HMM_Vec2), 1, fptr);
            fread(&state->window_position, sizeof(HMM_Vec2), 1, fptr);
            fread(&state->master_volume, sizeof(F32), 1, fptr);

            ReadString(state->music_fp, fptr);

            U32 param_count;
            fread(&param_count, sizeof(U32), 1, fptr);

            for (U32 i = 0; i < param_count; ++i) {
                char buf[256];
                F32  value, min, max;

                ReadString(buf, fptr);

                char *name = ArenaPushString(&state->arena, buf);

                fread(&value, sizeof(F32), 1, fptr);
                fread(&min, sizeof(F32), 1, fptr);
                fread(&max, sizeof(F32), 1, fptr);

                ParameterSet(state->parameters, &(Parameter){.name = name,
                                                             .value = value,
                                                             .min = min,
                                                             .max = max});
            }
        }
        fclose(fptr);

        return true;
    }

    return false;
}

static void RenderParameterSlider(const char *name,
                                  Rectangle   rect,
                                  const char *text_left,
                                  const char *text_right,
                                  F32         min,
                                  F32         max) {
    F32 val = ParameterGetValue(state->parameters, name);

    GuiSlider(rect, text_left, text_right, &val, min, max);

    ParameterSetValue(state->parameters, name, val);
}

static void RenderUI() {
    if (state->ui) {
        if (GuiButton((Rectangle){10, 10, 25, 25}, "#120#")) {
            state->ui = false;
        }

        F32 top_padding = 10;

        Parameter *parameter;
        U32        _, i = 0;
        while (ParameterIter(state->parameters, &_, &parameter)) {
            RenderParameterSlider(
                parameter->name,
                (Rectangle){100, i * 30 + top_padding, 200, 20},
                parameter->name, TextFormat("[%.2f]", parameter->value),
                parameter->min, parameter->max);
            ++i;
        }

        if (GuiButton(
                (Rectangle){state->screen_size.Width - 35, top_padding, 25, 25},
                "#11#")) {
            BeginRecording();
            state->condition = StateCondition_RECORDING;
        }

        GuiLabel((Rectangle){100, 200, 100, 100},
                 TextFormat("Total time: %3.f", state->total_time + GetTime()));
    } else {
        if (GuiButton((Rectangle){10, 10, 25, 25}, "#121#")) {
            state->ui = true;
        }
    }
}

static void Render() {
    ApiPreRender(state->api_data, state);

    RendererDrawCircleFrequencies(state->renderer_data, state->frequency_count,
                                  state->frequencies,
                                  state->renderer_data->default_color_func);
    // RendererDrawWaveform(state->renderer_data, SAMPLE_COUNT, state->samples,
    //                      ParameterGetValue(state->parameters, "wave_width"));
    RendererDrawFrequencies(state->renderer_data, state->frequency_count,
                            state->frequencies, true,
                            state->renderer_data->default_color_func);

    ApiRender(state->api_data, state);
}

static void BeginRecording() {
    memset(state->samples, 0, sizeof(F32) * SAMPLE_COUNT);
    memset(state->frequencies, 0, sizeof(F32) * state->frequency_count);

    HMM_Vec2 render_size = HMM_V2(state->renderer_data->screen.texture.width,
                                  state->renderer_data->screen.texture.height);

    state->ffmpeg = FFMPEGStart(render_size, RENDER_FPS, state->music_fp);
    state->record_start = GetTime();

    state->record_data.wave = LoadWave(state->music_fp);
    state->record_data.wave_samples = LoadWaveSamples(state->record_data.wave);
    state->record_data.wave_cursor = 0;

    PauseMusicStream(state->music);
}

static void EndRecording() {
    FFMPEGEnd(state->ffmpeg);

    UnloadWave(state->record_data.wave);
    UnloadWaveSamples(state->record_data.wave_samples);

    ResumeMusicStream(state->music);
}

static void UpdateRecording() {
    U32 chunk_size = state->record_data.wave.sampleRate / RENDER_FPS;
    F32(*frames)
    [state->record_data.wave.channels] =
        (void *)state->record_data.wave_samples;
    for (U32 i = 0; i < chunk_size; ++i) {
        if (state->record_data.wave_cursor <
            state->record_data.wave.frameCount) {
            StatePushFrame(frames[state->record_data.wave_cursor][0],
                           state->samples, SAMPLE_COUNT);
        } else {
            StatePushFrame(0, state->samples, SAMPLE_COUNT);
        }
        state->record_data.wave_cursor++;
    }

    SignalsProcessSamples(
        LOG_MUL, START_FREQ, state->samples, SAMPLE_COUNT, state->frequencies,
        &state->frequency_count, 1 / (F32)RENDER_FPS,
        (U32)ParameterGetValue(state->parameters, "smoothing"), state->filter,
        state->filter_count, ParameterGetValue(state->parameters, "velocity"));
}

static void SetFrequencyCount() {
    U32 freq_count;
    SignalsProcessSamples(
        LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL, &freq_count, 0,
        (U32)ParameterGetValue(state->parameters, "smoothing"), state->filter,
        state->filter_count, ParameterGetValue(state->parameters, "velocity"));

    if (freq_count != state->frequency_count) {
        if (freq_count > state->frequency_count) {
            memset(state->frequencies + state->frequency_count, 0,
                   (freq_count - state->frequency_count) * sizeof(F32));
        }
    }
}

static B8 GetDroppedFiles() {
    B8 ret = false;

    FilePathList dropped_files = LoadDroppedFiles();

    if (dropped_files.count > 0) {
        if (IsMusicReady(state->music)) {
            DetachAudioStreamProcessor(state->music.stream, FrameCallback);
            StopMusicStream(state->music);
            UnloadMusicStream(state->music);
        }

        state->music = LoadMusicStream(dropped_files.paths[0]);
        strcpy(state->music_fp, dropped_files.paths[0]);

        if (IsMusicReady(state->music)) {
            PlayMusicStream(state->music);
            AttachAudioStreamProcessor(state->music.stream, FrameCallback);
            ret = true;

            SetWindowTitle(TextFormat("Lynx - %s", dropped_files.paths[0]));
        }
    }

    UnloadDroppedFiles(dropped_files);

    return ret;
}

void StatePushFrame(F32 val, F32 *samples, U32 sample_count) {
    memmove(samples, samples + 1, (sample_count - 1) * sizeof(F32));
    samples[sample_count - 1] = val;
}

static void FrameCallback(void *buffer_data, U32 n) {
    F32(*frame_data)[state->music.stream.channels] = buffer_data;

    for (U32 i = 0; i < n; ++i) {
        StatePushFrame(frame_data[i][0], state->samples, SAMPLE_COUNT);
    }
}
