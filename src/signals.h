#pragma once

#include "defines.h"
#include "handmademath.h"

#include <complex.h>

void signals_process_samples(f32 scale, f32 start_frequency, f32 *samples, u32 sample_count, 
  f32 *out_frequencies, u32 *out_frequency_count, f32 dt, u32 smoothing);
void signals_window_samples(f32 *in, f32 *out, u32 length);
void signals_smooth_convolve(f32 *elements, u32 element_count, f32 *filter, u32 filter_count, f32 *out, u32 *out_count);
void signals_smooth_convolve_v2y(HMM_Vec2 *elements, u32 element_count, f32 *filter, u32 filter_count, HMM_Vec2 *out);
f32 signals_camp(float complex z);
void signals_fft(f32 in[], u32 stride, float complex out[], u32 n);
