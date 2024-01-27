#include "lmath.h"

#include "handmademath.h"
#include "raylib.h"

F32 MinF32(F32 a, F32 b) {
    if (a < b) {
        return a;
    }

    return b;
}

F32 MaxF32(F32 a, F32 b) {
    if (a > b) {
        return a;
    }

    return b;
}

F32 ClampF32(F32 v, F32 min, F32 max) {
    if (v < min) {
        v = min;
    }

    if (v > max) {
        v = max;
    }

    return v;
}

I32 ClampI32(I32 v, I32 min, I32 max) {
    if (v < min) {
        v = min;
    }

    if (v > max) {
        v = max;
    }

    return v;
}

HMM_Vec2 RayToHMMV2(Vector2 v) { return HMM_V2(v.x, v.y); }

Vector2 HMMToRayV2(HMM_Vec2 v) { return (Vector2){v.X, v.Y}; }
