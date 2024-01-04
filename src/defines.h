#pragma once

#include <stdint.h>

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef uint32_t b32;
typedef uint8_t b8;

typedef float f32;
typedef double f64;

#define ARRAY_LEN(a) sizeof((a))/sizeof((a)[0])
#define SAMPLE_COUNT (1 << 15)
#define RENDER_FPS 60
#define LOG_MUL 1.06f
#define START_FREQ 1.0f
