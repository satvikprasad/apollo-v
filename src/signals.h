#pragma once

#include <complex.h>

#include "defines.h"
#include "handmademath.h"

void
SignalsProcessSamples(F32  scale,
                      F32  start_frequency,
                      F32 *samples,
                      U32  sample_count,
                      F32 *out_frequencies,
                      U32 *out_frequency_count,
                      F32  dt,
                      U32  smoothing,
                      F32 *filter,
                      U32  filter_count,
                      U32  velocity,
                      B8   zero_freq);

void
SignalsWindowSamples(F32 *in, F32 *out, U32 length);
void
SignalsSmoothConvolve(F32 *elements,
                      U32  element_count,
                      F32 *filter,
                      U32  filter_count,
                      F32 *out,
                      U32 *out_count);

void
SignalsSmoothConvolveV2Y(HMM_Vec2 *elements,
                         U32       element_count,
                         F32      *filter,
                         U32       filter_count,
                         HMM_Vec2 *out);

F32
SignalsCAmp(float complex z);
void
SignalsFFT(F32 in[], U32 stride, float complex out[], U32 n);
