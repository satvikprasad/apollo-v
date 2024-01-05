#include "defines.h"
#include "handmademath.h"
#include "raylib.h"
#include "signals.h"

#include <assert.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// To convert from regular sample count to logarithmic frequency count, pass in NULL for *out_frequencies
// e.g signals_process_samples(LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL, &freq_count_ptr, 0, SMOOTHING)
void signals_process_samples(f32 scale, f32 start_frequency, f32 *samples, u32 sample_count, 
  f32 *out_frequencies, u32 *out_frequency_count, f32 dt, u32 smoothing) {
  *out_frequency_count = logf((0.5f*(f32)sample_count)/start_frequency)/logf(scale); 

  if (out_frequencies == NULL) {
    for (u32 i = 0; i < smoothing; ++i) {
    signals_smooth_convolve(NULL, *out_frequency_count, NULL, 3, NULL, out_frequency_count);
    }
    return;
  }

  f32 max_amp = 0.0f;
   
  f32 window_buffer[sample_count];
  float complex frequencies[sample_count];

  signals_window_samples(samples, window_buffer, sample_count);
  signals_fft(window_buffer, 1, frequencies, sample_count);

  for (u32 i = 0; i < sample_count/2; ++i) {
    f32 c = signals_camp(frequencies[i]);
    if (max_amp < c) {
      max_amp = c;
    }
  }

  f32 log_freq[*out_frequency_count];

  for (u32 i = 0; i < *out_frequency_count; ++i) {
    u32 f = powf(scale, i)*start_frequency;
    u32 f1 = powf(scale, i + 1)*start_frequency;

    f32 a = signals_camp(frequencies[f]);
    u32 m = 0;
    for (u32 q = f; q < sample_count/2 && q < f*scale; ++q) {
      if (signals_camp(frequencies[q]) > a) a = signals_camp(frequencies[q]);
    }

    log_freq[i] = a/max_amp;
  }

  for (u32 i = 0; i < smoothing; ++i) {
    signals_smooth_convolve(log_freq, *out_frequency_count, (f32[3]){1, 1, 1}, 3, log_freq, out_frequency_count);
  }

  f32 smooth_frequencies[*out_frequency_count];
  memcpy(smooth_frequencies, out_frequencies, *out_frequency_count * sizeof(f32));

  for (u32 i = 0; i < *out_frequency_count; ++i) {
    if (log_freq[i] > -FLT_MAX) {
      smooth_frequencies[i] += 10*(log_freq[i] - smooth_frequencies[i])*dt;
    }
  }

  memcpy(out_frequencies, smooth_frequencies, sizeof(f32) * *out_frequency_count);
}

void signals_window_samples(f32 *in, f32 *out, u32 length) {
  memcpy(out, in, sizeof(f32)*length);
  for (u32 i = 0; i < length; ++i) {
    f32 t = (f32)i/length;
    f32 window = 0.5 - 0.5*cosf(2*PI*t);
    out[i] *= window;
  }
}

f32 signals_camp(float complex z) {
  return logf(creal(z)*creal(z) + cimag(z)*cimag(z));
}

void signals_fft(f32 in[], u32 stride, float complex out[], u32 n) {
  assert(n > 0);

  if (n == 1) {
    out[0] = in[0];
    return;
  }

  signals_fft(in, stride*2, out, n/2);
  signals_fft(in + stride, stride*2, out + n/2, n/2);

  for (u32 k = 0; k < n/2; ++k) {
    f32 t = (f32)k / n;

    float complex v = cexp(-2*I*PI*t)*out[k + n/2];
    float complex e = out[k];

    out[k] = e + v;
    out[k + n/2] = e - v;
  }
}

void signals_smooth_convolve(f32 *elements, u32 element_count, f32 *filter, u32 filter_count, f32 *out, u32 *out_count) {
  *out_count = filter_count + element_count - 1;

  if (elements == NULL || filter == NULL || out == NULL) return;

  f32 y[element_count];

  for (u32 i = 0; i < *out_count; ++i) {
    f32 dot = 0.f;
    f32 sum = 0.f;
    for (u32 j = 0; j < filter_count; ++j) {
      if (j+i - filter_count + 1 >= element_count) {
        continue;
      }

      if (j+i - filter_count + 1 < 0) {
        continue;
      }

      dot += filter[j]*elements[j+i - filter_count + 1];
      sum += filter[j];
    }

    y[i] = dot/sum;
  }

  memcpy(out, y, sizeof(f32)*(*out_count));
}

void signals_smooth_convolve_v2y(HMM_Vec2 *elements, u32 element_count, f32 *filter, u32 filter_count, HMM_Vec2 *out) {
  HMM_Vec2 y[element_count];

  for (u32 i = 0; i < element_count; ++i) {
    f32 dot = 0.f;
    f32 sum = 0.f;
    for (u32 j = 0; j < filter_count; ++j) {
      if (j + i >= element_count) {
        continue;
      }

      dot += filter[j]*elements[j+i].Y;
      sum += filter[j];
    }

    y[i].Y = dot/sum;
    y[i].X = elements[i].X;
  }

  memcpy(out, y, sizeof(HMM_Vec2)*element_count);
}
