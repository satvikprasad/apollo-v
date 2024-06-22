// TODO: Polish Pop-ups, refractor repeated code

#include "state.h"

#include <math.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "animation.h"
#include "api.h"
#include "arena.h"
#include "defines.h"
#include "ffmpeg.h"
#include "filesystem.h"
#include "handmademath.h"
#include "io.h"
#include "lmath.h"
#include "loopback.h"
#include "menu.h"
#include "parameter.h"
#include "permanent_storage.h"
#include "procedures.h"
#include "renderer.h"
#include "server.h"
#include "signals.h"
#include "thread.h"
#include "ui.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

static void RenderUI();
static void Render();

static void BeginRecording();
static void EndRecording();
static void UpdateRecording();

static void SetFrequencyCount();
static B8 GetDroppedFiles();
static void CheckForDroppedFiles();

static void FrameCallback(void *buffer_data, U32 n);

static void PrintEllipses(U32 max, F32 min, char buf[max + 1]);

static void CreateFilter(F32 *filter, U32 filter_count);

static void CircleFrequenciesProc(void *user_data);
static void NormalFrequenciesProc(void *user_data);

static void CreateDirectories();

static StateFont LoadStateFont(const char *fp);
static void UnloadStateFont(StateFont font);

static void CreateFilter(F32 *filter, U32 filter_count);

static void FadeInAnimationUpdate(_Animation *anim, void *user_data, F64 dt);

static StateMemory memory;
static State *state;

bool StateLoadFile(const char *fp) {
    if (IsMusicReady(state->music)) {
        DetachAudioStreamProcessor(state->music.stream, FrameCallback);
        StopMusicStream(state->music);
        UnloadMusicStream(state->music);
    }

    state->music = LoadMusicStream(fp);
    strcpy(state->music_fp, fp);

    if (IsMusicReady(state->music)) {
        PlayMusicStream(state->music);
        AttachAudioStreamProcessor(state->music.stream, FrameCallback);

        SetWindowTitle(TextFormat("Apollo - %s", fp));

        return true;
    }

    return false;
}

void StateSetZeroFrequencies(bool v) { state->zero_frequencies = v; }

void StateSetCondition(StateCondition cond) { state->condition = cond; }

void StateInitialise() {
    LoopbackBegin();

    CreateDirectories();

    // Initialising memory
    memory.permanent_storage_size = 1024 * 1024 * 1024;
    memory.permanent_storage =
        PermanentStorageInit(memory.permanent_storage_size);

    state = (State *)memory.permanent_storage;

    ArenaInitialise(&state->arena,
                    memory.permanent_storage_size - sizeof(State),
                    (U8 *)memory.permanent_storage + sizeof(State));

    state->api_data = ArenaPushStruct(&state->arena, ApiData);
    state->renderer_data = ArenaPushStruct(&state->arena, RendererData);
    state->server_data = ArenaPushStruct(&state->arena, ServerData);

    // Initialise default parameters
    {
        state->parameters = ParameterCreate();

        state->def_params.velocity = ParameterSet(
            state->parameters,
            &(Parameter){
                .name = "VELOCITY", .value = 10.f, .min = 1, .max = 100});

        state->def_params.smoothing = ParameterSet(
            state->parameters,
            &(Parameter){
                .name = "SMOOTHING", .value = 5.0f, .min = 1, .max = 10});

        state->def_params.master_volume = ParameterSet(
            state->parameters,
            &(Parameter){
                .name = "MASTER VOL", .value = 100.0f, .min = 0, .max = 100});
    }

    // Initialise animations
    state->animations = AnimationsCreate();

    {
        state->procedures = ProcedureCreate();

        state->def_procs.circle_frequencies =
            _ProcedureAdd(state->procedures, "_1", NULL, CircleFrequenciesProc,
                          &state->arena);

        state->def_procs.normal_frequencies =
            _ProcedureAdd(state->procedures, "_2", NULL, NormalFrequenciesProc,
                          &state->arena);
    }

    char apollo[512];
    FSGetApolloDirectory(apollo);
    ApiInitialise(TextFormat("%s/%s", apollo, "init.lua"), state,
                  state->api_data);

    if (Deserialize(state)) {
        if (!FileExists(state->music_fp) || strlen(state->music_fp) == 0) {
            strcpy(state->music_fp, FSFormatAssetsDirectory("monks.mp3"));
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

    state->font = LoadStateFont("fonts/helvetica.ttf");

    state->filter_count = 5;
    state->filter = ArenaPushArray(&state->arena, state->filter_count, F32);
    CreateFilter(state->filter, state->filter_count);

    GuiLoadStyle(FSFormatAssetsDirectory("styles/apollo.rgs"));

    GuiSetFont(FontClosestToSize(state->font, 20));
    GuiSetStyle(DEFAULT, TEXT_SIZE,
                FontClosestToSize(state->font, 20).baseSize);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xFFFFFFFF);

    SignalsProcessSamples(LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL,
                          &state->frequency_count, 0, 0, state->filter,
                          state->filter_count,
                          _ParameterGetValue(state->def_params.velocity),
                          state->zero_frequencies);

    if (IsMusicReady(state->music)) {
        PlayMusicStream(state->music);
        AttachAudioStreamProcessor(state->music.stream, FrameCallback);
    }

    RendererInitialise(state->renderer_data);
    ServerInitialise(state->server_data, API_URI, &state->arena);

    state->menu_data = MenuCreate(&state->arena);

    state->def_anims.fade_in =
        AnimationsAdd(state->animations, "fade_in", &(F32){0.74f},
                      FadeInAnimationUpdate, &state->arena);
}

void StateDestroy() {
    ServerWait(state->server_data);

    Serialize(state);

    ServerDestroy(state->server_data);

    ApiDestroy(state->api_data);

    RendererDestroy(state->renderer_data);

    ParameterDestroy(state->parameters);

    UnloadStateFont(state->font);
    UnloadMusicStream(state->music);

    PermanentStorageDestroy(memory.permanent_storage);
}

void AddMetricCallback(void *user_data, char *response) {
    *(B8 *)user_data = true;
}

static void FadeAnimationUpdate(_Animation *anim, void *user_data, F64 dt) {
    anim->val = anim->elapsed / *(F32 *)user_data;

    if (anim->val > 1.0) {
        anim->val = 1.0f;
        anim->finished = true;
    }
}

static void *EndRecordingThread(void *data) {
    EndRecording();
    return NULL;
}

void EndRecordingAnimationUpdate(_Animation *anim, void *user_data, F64 dt) {
    anim->val = (0.5f - anim->elapsed) / 0.5f;

    if (anim->elapsed >= 0.5) {
        anim->val = 0.0f;
        state->condition = StateCondition_NORMAL;
        anim->finished = true;
    }
}

void BeginExiting() {
    ServerPostAsync(state->server_data, "add-metric",
                    TextFormat("{\"time\": %f}", GetTime()),
                    &state->should_close, AddMetricCallback, &state->arena);

    if (StateGetLoopback()) {
        state->should_open_on_loopback = true;
    }

    state->condition = StateCondition_EXITING;
    state->def_anims.exiting =
        AnimationsAdd(state->animations, "exiting", &(F32){1.0f},
                      FadeAnimationUpdate, &state->arena);
}

void PopUpEnterAnimationUpdate(_Animation *anim, void *user_data, F64 dt) {
    F32 length = *(F32 *)user_data;

    anim->val = sin((PI * anim->elapsed) / (2 * length));

    if (anim->elapsed >= length) {
        anim->finished = true;
    }
}

void StateAddPopUp(const char *text) {
    memcpy(state->pop_ups + 1, state->pop_ups,
           sizeof(StatePopUp) * (MAX_POP_UPS - 1));

    strcpy(state->pop_ups[0].text, text);

    if (state->pop_up_count < MAX_POP_UPS) {
        state->pop_up_count++;
    }

    state->def_anims.pop_up =
        AnimationsAdd(state->animations, "pop up", &(F32){0.5},
                      PopUpEnterAnimationUpdate, &state->arena);
}

void PopUpExitAnimationUpdate(_Animation *anim, void *user_data, F64 dt) {
    F32 length = *(F32 *)user_data;

    anim->val = sin((PI * (length - anim->elapsed)) / (2 * length));

    if (anim->elapsed >= length) {
        state->pop_up_count--;
        memcpy(state->pop_ups, state->pop_ups + 1,
               sizeof(StatePopUp) * (MAX_POP_UPS - 1));

        anim->finished = true;
    }
}

void StateRemovePopUp() {
    state->def_anims.pop_up_exit =
        AnimationsAdd(state->animations, "pop up exit", &(F32){0.25},
                      PopUpExitAnimationUpdate, &state->arena);
}

bool StateIsPaused() { return !IsMusicStreamPlaying(state->music); }

void StateToggleMenu() { state->render_ui = !state->render_ui; }
bool StateIsShowingMenu() { return state->render_ui; }

void StateTogglePlayPause() {
    if (IsMusicStreamPlaying(state->music)) {
        PauseMusicStream(state->music);
    } else {
        ResumeMusicStream(state->music);
    }
}

void StateUpdate() {
    ApiUpdate(state->api_data, state);
    AnimationsUpdate(state->animations);

    SetFrequencyCount();

    if (IsKeyPressed(KEY_ESCAPE) || WindowShouldClose()) {
        if (state->condition == StateCondition_RECORDING) {
            if (!state->recording_thread) {
                state->recording_thread = ThreadAlloc(&state->arena);
            }

            ThreadCreate(state->recording_thread, EndRecordingThread, state);

            state->def_anims.end_recording =
                AnimationsAdd(state->animations, "end_recording", NULL,
                              EndRecordingAnimationUpdate, &state->arena);

            ResumeMusicStream(state->music);
        } else {
            BeginExiting();
        }
    }

    switch (state->condition) {
    case StateCondition_EXITING: {
    } break;
    case StateCondition_NORMAL: {
        ApiPreUpdate(state->api_data, state);

        if (IsKeyPressed(KEY_R)) {
            SeekMusicStream(state->music, 0);
        }

        if (IsKeyPressed(KEY_SPACE)) {
            StateTogglePlayPause();
        }

        if (IsKeyPressed(KEY_M) && !IsKeyDown(KEY_LEFT_CONTROL) &&
            IsMusicReady(state->music)) {
            if (_ParameterGetValue(state->def_params.master_volume) != 0.f) {
                _ParameterSetValue(state->def_params.master_volume, 0.f);
            } else {
                _ParameterSetValue(state->def_params.master_volume, 100.f);
            }
        }

        if (!IsMusicReady(state->music)) {
            state->condition = StateCondition_LOAD;
            break;
        }

        UpdateMusicStream(state->music);
        if (IsWindowResized()) {
            state->screen_size = HMM_V2(GetRenderWidth(), GetRenderHeight());
        }

        SetMasterVolume(_ParameterGetValue(state->def_params.master_volume) /
                        100.f);

        CheckForDroppedFiles();

        if (IsKeyPressed(KEY_B) && IsMusicReady(state->music)) {
            BeginRecording();
        }

        if (IsKeyPressed(KEY_M) && IsKeyDown(KEY_LEFT_CONTROL)) {
            StateToggleMenu();
        }

        SignalsProcessSamples(
            LOG_MUL, START_FREQ, state->samples, SAMPLE_COUNT,
            state->frequencies, &state->frequency_count, state->dt,
            (U32)_ParameterGetValue(state->def_params.smoothing), state->filter,
            state->filter_count, _ParameterGetValue(state->def_params.velocity),
            state->zero_frequencies);

    } break;

    case StateCondition_LOOPBACK: {
        CheckForDroppedFiles();

        SignalsProcessSamples(
            LOG_MUL, START_FREQ, state->samples, SAMPLE_COUNT,
            state->frequencies, &state->frequency_count, state->dt,
            (U32)_ParameterGetValue(state->def_params.smoothing), state->filter,
            state->filter_count, _ParameterGetValue(state->def_params.velocity),
            state->zero_frequencies);
    } break;

    case StateCondition_ERROR: {
    } break;

    case StateCondition_LOAD: {
        if (IsFileDropped()) {
            if (GetDroppedFiles()) {
                state->condition = StateCondition_NORMAL;
            }
        }
    } break;

    case StateCondition_RECORDING: {
        if (state->record_data.wave_cursor >=
            state->record_data.wave.frameCount) {
            EndRecording();
            state->condition = StateCondition_NORMAL;
        }

        UpdateRecording();
    } break;
    }

    state->dt = GetFrameTime();
    state->window_position =
        HMM_V2(GetWindowPosition().x, GetWindowPosition().y);
    state->master_volume = GetMasterVolume();
}

void StateRender() {
    Color clear = state->api_data->data.opt.bg_color;

    BeginDrawing();
    ClearBackground(clear);

    switch (state->condition) {
    case StateCondition_EXITING: {
        char buf[4];
        PrintEllipses(3, 0.f, buf);

        RendererDrawTextCenter(
            XLargeFont(state->font), TextFormat("Exiting%s", buf),
            HMM_V2(state->screen_size.Width / 2, state->screen_size.Height / 2),
            (Color){255, 255, 255,
                    255 * AnimationsLoad_(state->animations,
                                          state->def_anims.exiting)});
    } break;
    case StateCondition_LOOPBACK:
    case StateCondition_NORMAL: {
        if (!IsMusicReady(state->music)) {
            state->condition = StateCondition_LOAD;
            break;
        }
        RendererSetRenderSize(state->renderer_data, state->screen_size);

        Render();

        if (state->render_ui) {
            RenderUI();

            // Render Pop-Ups -- TODO: Clean this up.
            if (state->pop_up_count > 0 && state->ui) {
                F32 pop_up_padding = 25;
                F32 pop_up_height = 50;

                F32 offset = 0;
                if (AnimationsExists_(state->animations,
                                      state->def_anims.pop_up)) {
                    offset = (1 / 2.f) * (pop_up_height + pop_up_padding) *
                             (AnimationsLoad_(state->animations,
                                              state->def_anims.pop_up) -
                              1);
                } else if (AnimationsExists_(state->animations,
                                             state->def_anims.pop_up_exit)) {
                    offset = (1 / 2.f) * (pop_up_height + pop_up_padding) *
                             (AnimationsLoad_(state->animations,
                                              state->def_anims.pop_up_exit) -
                              1);
                }

                F32 border_size = 2;

                for (U32 i = state->pop_up_count - 1; i > 0; --i) {
                    UIRenderPopUp(border_size, pop_up_height, pop_up_padding,
                                  offset + (50 + pop_up_padding / 2) * i,
                                  255 - i * (255.f / 10), state->screen_size,
                                  state->font, &state->pop_ups[i], false);
                }

                if (UIRenderPopUp(border_size, pop_up_height, pop_up_padding,
                                  offset, 255, state->screen_size, state->font,
                                  &state->pop_ups[0], true) &&
                    !AnimationsExists_(state->animations,
                                       state->def_anims.pop_up_exit)) {
                    StateRemovePopUp();
                }
            }
        }
    } break;

    case StateCondition_LOAD: {
        RendererDrawTextCenter(
            MediumFont(state->font), "Drag & drop music to play",
            HMM_V2(state->screen_size.Width / 2, state->screen_size.Height / 2),
            (Color){255, 255, 255, 255});
    } break;

    case StateCondition_RECORDING: {
        char buf[4];
        PrintEllipses(3, 0.f, buf);

        F64 alpha = 1.0f;

        AnimationsApply(state->animations, state->def_anims.recording->name,
                        &alpha);

        RendererDrawTextCenter(
            XLargeFont(state->font), TextFormat("Now recording%s", buf),
            HMM_V2(state->screen_size.Width / 2, state->screen_size.Height / 2),
            (Color){255, 255, 255, 255 * alpha});

        RendererBeginRecording(state->renderer_data);
        {
            ClearBackground(clear);

            Render();
        }
        RendererEndRecording(state->renderer_data, state->ffmpeg);

    } break;

    default:
        printf("Unhandled case\n");
        break;
    }

    if (AnimationsExists_(state->animations, state->def_anims.fade_in)) {
        DrawRectangle(0, 0, state->screen_size.Width, state->screen_size.Height,
                      (Color){0, 0, 0,
                              255 * AnimationsLoad_(state->animations,
                                                    state->def_anims.fade_in)});
    }

    EndDrawing();
}

static void RenderParameterSlider(const char *name, Rectangle rect,
                                  const char *text_left, const char *text_right,
                                  F32 min, F32 max) {
    F32 val = ParameterGetValue(state->parameters, name);

    GuiSlider(rect, text_left, text_right, &val, min, max);

    ParameterSetValue(state->parameters, name, val);
}

static void RenderUI() {
    F32 padding = 20;
    F32 button_height = 25;
    F32 font_size = 20;
    F32 toggle_width = 25;
    Parameter *parameter;
    U32 _, i = 0;

    UIToggleMenuData data =
        UIMeasureToggleMenu(state->parameters, state->procedures, state->font,
                            font_size, padding, toggle_width);

    F32 left = data.max_loffset + toggle_width + padding;
    F32 param_width = 200;
    F32 max_param_width = (state->screen_size.Width - data.proc_button_width -
                           padding - data.max_roffset) -
                          (left);

    if (toggle_width + data.proc_button_width + padding * 3 >
        state->screen_size.Width) {
        Rectangle button = {padding, padding, toggle_width, button_height};

        if (GuiButton(
                (Rectangle){padding, padding, toggle_width, button_height},
                "#137#")) {
        }

        DrawRectangleRec(button, (Color){0, 0, 0, 100});

        state->ui = false;

        return;
    }

    if (state->ui) {
        if (GuiButton(
                (Rectangle){padding, padding, toggle_width, button_height},
                "#121#")) {
            state->ui = false;
        }

        if (max_param_width > 50) {
            parameter = 0;
            _ = 0;
            i = 0;
            while (ParameterIter(state->parameters, &_, &parameter)) {
                if (!parameter) {
                    continue;
                }

                RenderParameterSlider(
                    parameter->name,
                    (Rectangle){left,
                                i * (button_height + padding / 2) + padding,
                                param_width > max_param_width ? max_param_width
                                                              : param_width,
                                button_height},
                    parameter->name, TextFormat("[%.2f]", parameter->value),
                    parameter->min, parameter->max);
                ++i;
            }
        }

        // Render procedure buttons
        Procedure *proc = 0;
        _ = 0;
        i = 0;
        while (ProcedureIter(state->procedures, &_, &proc)) {
            if (!proc) {
                continue;
            }

            if (GuiToggle(
                    (Rectangle){state->screen_size.Width -
                                    data.proc_button_width - padding,
                                i * (button_height + padding / 2) + padding,
                                data.proc_button_width - padding,
                                button_height},
                    TextFormat("%s", proc->name), &proc->active)) {
                ProcedureToggle(state->procedures, proc->name);
            }
            ++i;
        }

        if (GuiButton(
                (Rectangle){state->screen_size.Width - 35, padding, 25, 25},
                "#11#")) {
            BeginRecording();
        }
    } else {
        if (GuiButton(
                (Rectangle){padding, padding, toggle_width, button_height},
                "#120#")) {
            state->ui = true;
        }
    }
}

static void CircleFrequenciesProc(void *user_data) {
    RendererDrawCircleFrequencies(state->renderer_data, state->frequency_count,
                                  state->frequencies,
                                  state->renderer_data->default_color_func);
}

static void NormalFrequenciesProc(void *user_data) {
    RendererDrawFrequencies(state->renderer_data, state->frequency_count,
                            state->frequencies, true,
                            state->renderer_data->default_color_func);
}

static void Render() {
    ApiPreRender(state->api_data, state);

    _ProcedureCall(state->def_procs.circle_frequencies);
    _ProcedureCall(state->def_procs.normal_frequencies);

    ApiRender(state->api_data, state);
}

static void BeginRecording() {
    state->condition = StateCondition_RECORDING;

    if (!FSCanRunCMD("ffmpeg")) {
        state->condition = StateCondition_NORMAL;
        StateAddPopUp("Please ensure FFMPEG is installed. Can't record :(");
        return;
    }

    HMM_Vec2 render_size = HMM_V2(state->renderer_data->screen.texture.width,
                                  state->renderer_data->screen.texture.height);

    state->ffmpeg = FFMPEGStart(render_size, RENDER_FPS, state->music_fp);

    if (state->ffmpeg < 0) {
        state->condition = StateCondition_NORMAL;
        return;
    }

    memset(state->samples, 0, sizeof(F32) * SAMPLE_COUNT);
    memset(state->frequencies, 0, sizeof(F32) * state->frequency_count);

    state->record_start = GetTime();

    state->record_data.wave = LoadWave(state->music_fp);
    state->record_data.wave_samples = LoadWaveSamples(state->record_data.wave);
    state->record_data.wave_cursor = 0;

    PauseMusicStream(state->music);

    state->def_anims.recording =
        AnimationsAdd(state->animations, "recording", &(F32){0.4f},
                      FadeAnimationUpdate, &state->arena);
}

static void EndRecording() {
    FFMPEGEnd(state->ffmpeg);

    UnloadWave(state->record_data.wave);
    UnloadWaveSamples(state->record_data.wave_samples);
}

static void UpdateRecording() {
    U32 chunk_size = state->record_data.wave.sampleRate / RENDER_FPS;
    F32(*frames)
    [state->record_data.wave.channels] =
        (void *)state->record_data.wave_samples;
    for (U32 i = 0; i < chunk_size; ++i) {
        if (state->record_data.wave_cursor <
            state->record_data.wave.frameCount) {
            StatePushFrame(frames[state->record_data.wave_cursor][0]);
        } else {
            StatePushFrame(0);
        }
        state->record_data.wave_cursor++;
    }

    SignalsProcessSamples(
        LOG_MUL, START_FREQ, state->samples, SAMPLE_COUNT, state->frequencies,
        &state->frequency_count, 1 / (F32)RENDER_FPS,
        (U32)_ParameterGetValue(state->def_params.smoothing), state->filter,
        state->filter_count, _ParameterGetValue(state->def_params.velocity),
        state->zero_frequencies);
}

static void SetFrequencyCount() {
    U32 freq_count;
    SignalsProcessSamples(
        LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL, &freq_count, 0,
        (U32)_ParameterGetValue(state->def_params.smoothing), state->filter,
        state->filter_count, _ParameterGetValue(state->def_params.velocity),
        state->zero_frequencies);

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
        ret = StateLoadFile(dropped_files.paths[0]);
    }

    UnloadDroppedFiles(dropped_files);

    return ret;
}

bool StateGetLoopback() { return state->condition == StateCondition_LOOPBACK; }

void StateToggleProcedure(const char *proc_name) {
    ProcedureToggle(state->procedures, proc_name);
}

B8 StateIterProcedures(U32 *iter, Procedure **procedure) {
    return ProcedureIter(state->procedures, iter, procedure);
}

void StateToggleLoopback() {
    switch (state->condition) {
    case StateCondition_NORMAL:
        // Change condition to loopback mode.
        PauseMusicStream(state->music);
        state->condition = StateCondition_LOOPBACK;
        break;
    case StateCondition_LOOPBACK:
        // Change condition to normal.
        ResumeMusicStream(state->music);
        state->condition = StateCondition_NORMAL;
        break;
    default:
        return;
        break;
    }
}

void StatePushFrame(F32 val) {
    F32 *samples = state->samples;
    U32 sample_count = SAMPLE_COUNT;

    memmove(samples, samples + 1, (sample_count - 1) * sizeof(F32));
    samples[sample_count - 1] = val;
}

B8 StateShouldClose() { return state->should_close; }

static void FrameCallback(void *buffer_data, U32 n) {
    F32(*frame_data)[state->music.stream.channels] = buffer_data;

    if (!StateGetLoopback()) {
        for (U32 i = 0; i < n; ++i) {
            StatePushFrame(frame_data[i][0]);
        }
    }
}

static void PrintEllipses(U32 max, F32 min, char buf[max + 1]) {
    U32 n =
        MaxU32(roundf(0.5f * (max - min) * (sinf(5 * GetTime()) + 1) + min), 0);

    for (U32 i = 0; i < max; ++i) {
        if (i < n) {
            buf[i] = '.';
        } else {
            buf[i] = ' ';
        }
    }

    buf[max] = '\0';
}

static StateFont LoadStateFont(const char *fp) {
    StateFont font;

    U32 incr = MAX_FONT_SIZE / FONT_SIZES_PER_FONT;

    for (U32 i = 0; i < FONT_SIZES_PER_FONT; ++i) {
        font.fonts[i] =
            LoadFontEx(FSFormatAssetsDirectory(fp), (i + 1) * incr, NULL, 0);
    }

    return font;
}

static void UnloadStateFont(StateFont font) {
    for (U32 i = 0; i < FONT_SIZES_PER_FONT; ++i) {
        UnloadFont(font.fonts[i]);
    }
}

static void CreateDirectories() {
    char config[512];
    FSGetConfigDirectory(config);
    if (!DirectoryExists(config)) {
        FSCreateDirectory(config);
    }

    char apollo[512];
    FSGetApolloDirectory(apollo);

    if (!DirectoryExists(apollo)) {
        FSCreateDirectory(apollo);
    }

    if (!DirectoryExists(TextFormat("%s/shaders", apollo))) {
        FSCreateDirectory(TextFormat("%s/shaders", apollo));
    }
}

static void FadeInAnimationUpdate(_Animation *anim, void *user_data, F64 dt) {
    anim->val = sqrtf(1.0f - anim->elapsed / *(F32 *)user_data);

    if (anim->val < 0.0) {
        anim->val = 0.0f;
        anim->finished = true;
    }
}

static void CreateFilter(F32 *filter, U32 filter_count) {
    I32 begin = (U32)floor((F32)filter_count / 2);

    I32 end = (U32)ceil((F32)filter_count / 2);

    for (I32 i = -begin; i < end; ++i) {
        filter[i + begin] = expf(-i * i);
    }
}

static void CheckForDroppedFiles() {
    if (IsFileDropped()) {
        if (!GetDroppedFiles()) {
            state->condition = StateCondition_LOAD;
        } else {
            state->condition = StateCondition_NORMAL;
        }
    }
}
