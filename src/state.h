#pragma once

#include <complex.h>

#include "animation.h"
#include "api.h"
#include "arena.h"
#include "defines.h"
#include "handmademath.h"
#include "hashmap.h"
#include "loopback.h"
#include "parameter.h"
#include "procedures.h"
#include "raylib.h"
#include "renderer.h"
#include "server.h"

typedef struct State State;

typedef enum StateCondition {
    StateCondition_NORMAL = 0,
    StateCondition_ERROR,
    StateCondition_LOAD,
    StateCondition_RECORDING,
    StateCondition_EXITING,
} StateCondition;

#define MAX_PARAM_COUNT 100

#define FREQUENCY_COUNT 2048

typedef struct StateMemory {
    void *permanent_storage;
    U32   permanent_storage_size;

    void *transient_storage;
    U32   transient_storage_size;
} StateMemory;

#define FONT_SIZES_PER_FONT 12
#define MAX_FONT_SIZE 100

#define FontClosestToSize(font, size)                                          \
    (font).fonts[ClampI32((size) * FONT_SIZES_PER_FONT / MAX_FONT_SIZE - 1, 0, \
                          FONT_SIZES_PER_FONT - 1)]
#define SmallFont(font) FontClosestToSize((font), 10)
#define MediumFont(font) FontClosestToSize((font), 20)
#define LargeFont(font) FontClosestToSize((font), 30)
#define XLargeFont(font) FontClosestToSize((font), 40)

typedef struct StateFont {
    Font fonts[FONT_SIZES_PER_FONT];
} StateFont;

#define MAX_ANIMATION_COUNT 512
typedef struct State {
    MemoryArena arena;

    RendererData *renderer_data;
    ApiData      *api_data;
    LoopbackData *loopback_data;
    ServerData   *server_data;

    Thread *recording_thread;

    Music music;
    char  music_fp[256];

    StateFont font;

    F32 samples[SAMPLE_COUNT];

    F32 frequencies[FREQUENCY_COUNT];
    U32 frequency_count;

    F32 master_volume;
    F32 dt;
    F32 record_start;

    I32 ffmpeg;

    struct {
        Wave wave;
        F32 *wave_samples;
        U32  wave_cursor;
    } record_data;

    struct {
        _Parameter smoothing;
        _Parameter velocity;
        _Parameter master_volume;
    } def_params;

    struct {
        _Procedure circle_frequencies;
        _Procedure normal_frequencies;
    } def_procs;

    struct {
        _Animation *exiting;
        _Animation *recording;
        _Animation *end_recording;
        _Animation *fade_in;
    } def_anims;

    F32 *filter;
    U32  filter_count;

    StateCondition condition;

    HM_Hashmap *parameters;
    HM_Hashmap *animations;
    HM_Hashmap *procedures;

    B8 render_ui;
    B8 ui;
    B8 loopback;
    B8 should_close;

    B8 zero_frequencies;

    HMM_Vec2 screen_size;
    HMM_Vec2 window_position;
} State;

void
StateInitialise();
void
StateUpdate();
void
StateRender();
void
StateDestroy();
void
StatePushFrame(F32 val, F32 *samples, U32 sample_count);

B8
StateShouldClose();
