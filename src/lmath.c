#include "lmath.h"

#include "handmademath.h"
#include "raylib.h"

f32 min(f32 a, f32 b) {
  if (a < b) return a;
  return b;
}

f32 max(f32 a, f32 b) {
  if (a > b) return a;
  return b;
}

f32 clamp(f32 v, f32 min, f32 max) {
  if (v < min) {
    v = min;
  }

  if (v > max) {
    v = max;
  }

  return v;
}

HMM_Vec2 ray_to_hmm_v2(Vector2 v) { return HMM_V2(v.x, v.y); }

Vector2 hmm_to_ray_v2(HMM_Vec2 v) { return (Vector2){v.X, v.Y}; }
