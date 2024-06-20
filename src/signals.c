#include "signals.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "handmademath.h"
#include "raylib.h"

// To convert from regular sample count to logarithmic frequency count, pass in
// NULL for *out_frequencies e.g SignalsProcessSamples(LOG_MUL, START_FREQ, 0,
// SAMPLE_COUNT, NULL, &freq_count_ptr, 0, SMOOTHING)
void SignalsProcessSamples(F32 scale, F32 start_frequency, F32 *samples,
                           U32 sample_count, F32 *out_frequencies,
                           U32 *out_frequency_count, F32 dt, U32 smoothing,
                           F32 *filter, U32 filter_count, U32 velocity,
                           B8 zero_freq) {
    *out_frequency_count =
        logf((0.5f * (F32)sample_count) / start_frequency) / logf(scale);

    if (out_frequencies == NULL) {
        for (U32 i = 0; i < smoothing; ++i) {
            SignalsSmoothConvolve(NULL, *out_frequency_count, NULL,
                                  filter_count, NULL, out_frequency_count);
        }
        return;
    }

    F32 max_amp = 0.0f;

    F32 window_buffer[sample_count];
    float complex frequencies[sample_count];

    SignalsWindowSamples(samples, window_buffer, sample_count);
    SignalsFFT(window_buffer, 1, frequencies, sample_count);

    for (U32 i = 0; i < sample_count / 2; ++i) {
        F32 c = SignalsCAmp(frequencies[i]);
        if (max_amp < c) {
            max_amp = c;
        }
    }

    F32 log_freq[*out_frequency_count];

    for (U32 i = 0; i < *out_frequency_count; ++i) {
        U32 f = powf(scale, i) * start_frequency;

        F32 a = SignalsCAmp(frequencies[f]);
        for (U32 q = f; q < sample_count / 2 && q < f * scale; ++q) {
            if (SignalsCAmp(frequencies[q]) > a) {
                a = SignalsCAmp(frequencies[q]);
            }
        }

        log_freq[i] = a / max_amp;
    }

    for (U32 i = 0; i < smoothing; ++i) {
        SignalsSmoothConvolve(log_freq, *out_frequency_count, filter,
                              filter_count, log_freq, out_frequency_count);
    }

    if (zero_freq) {
        memset(log_freq, 0, *out_frequency_count * sizeof(F32));
    }

    F32 *smooth_frequencies = out_frequencies;

    for (U32 i = 0; i < *out_frequency_count; ++i) {
        if (log_freq[i] > -FLT_MAX) {
            if (velocity < 100) {
                smooth_frequencies[i] +=
                    velocity * (log_freq[i] - smooth_frequencies[i]) * dt;
            } else {
                smooth_frequencies[i] = log_freq[i];
            }
        }
    }
}

void SignalsWindowSamples(F32 *in, F32 *out, U32 length) {
    memcpy(out, in, sizeof(F32) * length);
    for (U32 i = 0; i < length; ++i) {
        F32 t = (F32)i / length;
        F32 window = 0.5 - 0.5 * cosf(2 * PI * t);
        out[i] *= window;
    }
}

F32 SignalsCAmp(float complex z) {
    return logf(creal(z) * creal(z) + cimag(z) * cimag(z));
}

void SignalsFFT(F32 in[], U32 stride, float complex out[], U32 n) {
    assert(n > 0);

    if (n == 1) {
        out[0] = in[0];
        return;
    }

    SignalsFFT(in, stride * 2, out, n / 2);
    SignalsFFT(in + stride, stride * 2, out + n / 2, n / 2);

    for (U32 k = 0; k < n / 2; ++k) {
        F32 t = (F32)k / n;

        float complex v = cexp(-2 * I * PI * t) * out[k + n / 2];
        float complex e = out[k];

        out[k] = e + v;
        out[k + n / 2] = e - v;
    }
}

void SignalsSmoothConvolve(F32 *elements, U32 element_count, F32 *filter,
                           U32 filter_count, F32 *out, U32 *out_count) {
    *out_count = filter_count + element_count - 1;

    if (elements == NULL || filter == NULL || out == NULL)
        return;

    F32 y[*out_count];

    for (U32 i = 0; i < *out_count; ++i) {
        F32 dot = 0.f;
        F32 sum = 0.f;
        for (U32 j = 0; j < filter_count; ++j) {
            if (j + i - filter_count + 1 >= element_count) {
                continue;
            }

            if (j + i - filter_count + 1 < 0) {
                continue;
            }

            dot += filter[j] * elements[j + i - filter_count + 1];
            sum += filter[j];
        }

        y[i] = dot / sum;
    }

    memcpy(out, y, sizeof(F32) * (*out_count));
}

void SignalsSmoothConvolveV2Y(HMM_Vec2 *elements, U32 element_count,
                              F32 *filter, U32 filter_count, HMM_Vec2 *out) {
    HMM_Vec2 y[element_count];

    for (U32 i = 0; i < element_count; ++i) {
        F32 dot = 0.f;
        F32 sum = 0.f;
        for (U32 j = 0; j < filter_count; ++j) {
            if (j + i >= element_count) {
                continue;
            }

            dot += filter[j] * elements[j + i].Y;
            sum += filter[j];
        }

        y[i].Y = dot / sum;
        y[i].X = elements[i].X;
    }

    memcpy(out, y, sizeof(HMM_Vec2) * element_count);
}
