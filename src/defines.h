#pragma once

#include <stdint.h>

typedef int64_t I64;
typedef int32_t I32;
typedef int16_t I16;
typedef int8_t I8;

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

typedef uint32_t B32;
typedef uint8_t B8;

typedef float F32;
typedef double F64;

#define ARRAY_LEN(a) sizeof((a)) / sizeof((a)[0])
#define SAMPLE_COUNT (1 << 15)
#define RENDER_FPS 60
#define LOG_MUL 1.06f
#define START_FREQ 1.0f
