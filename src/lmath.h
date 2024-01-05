#pragma once

#include "defines.h"
#include "handmademath.h"
#include "raylib.h"

f32 min(f32 a, f32 b);
f32 max(f32 a, f32 b);
f32 clamp(f32 v, f32 min, f32 max);

HMM_Vec2 ray_to_hmm_v2(Vector2 v);
Vector2 hmm_to_ray_v2(HMM_Vec2 v);
