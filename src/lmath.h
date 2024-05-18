#pragma once

#include "defines.h"
#include "handmademath.h"
#include "raylib.h"

U32
MaxU32(U32 a, U32 b);
U32
MinU32(U32 a, U32 b);

F32
MinF32(F32 a, F32 b);
F32
MaxF32(F32 a, F32 b);
F32
ClampF32(F32 v, F32 min, F32 max);
I32
ClampI32(I32 v, I32 min, I32 max);

HMM_Vec2
RayToHMMV2(Vector2 v);
Vector2
HMMToRayV2(HMM_Vec2 v);
