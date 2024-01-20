#pragma once

#include <complex.h>

#include "api.h"
#include "arena.h"
#include "defines.h"
#include "handmademath.h"
#include "parameter.h"
#include "raylib.h"
#include "renderer.h"

typedef struct State State;

typedef enum StateCondition {
    StateCondition_NORMAL = 0,
    StateCondition_ERROR,
    StateCondition_LOAD,
    StateCondition_RECORDING,
} StateCondition;

#define MAX_PARAM_COUNT 100

#define FREQUENCY_COUNT 2048

typedef struct StateMemory {
    void *permanent_storage;
    U32 permanent_storage_size;
} StateMemory;

#define FONT_SIZES_PER_FONT 4
#define MAX_FONT_SIZE 40

#define FontClosestToSize(font, size)                                          \
    (font).fonts[(size) * FONT_SIZES_PER_FONT / MAX_FONT_SIZE - 1]
#define SmallFont(font) FontClosestToSize((font), 10)
#define MediumFont(font) FontClosestToSize((font), 20)
#define LargeFont(font) FontClosestToSize((font), 30)
#define XLargeFont(font) FontClosestToSize((font), 40)

typedef struct StateFont {
    Font fonts[FONT_SIZES_PER_FONT];
} StateFont;

typedef struct State {
    MemoryArena arena;

    Renderer *renderer;
    Api *api;

    Music music;
    char music_fp[256];

    StateFont font;

    HMM_Vec2 screen_size;
    HMM_Vec2 window_position;

    F32 master_volume;

    F32 samples[SAMPLE_COUNT];
    F32 frequencies[FREQUENCY_COUNT];
    U32 frequency_count;

    F32 dt;

    F32 record_start;

    I32 ffmpeg;

    struct {
        Wave wave;
        F32 *wave_samples;
        U32 wave_cursor;
    } record_data;

    F32 filter[1 << 3];
    U32 filter_count;

    StateCondition condition;

    Parameters *parameters;

    B8 ui;
} State;

void StateInitialise();
void StateUpdate();
void StateRender();
void StateDestroy();
