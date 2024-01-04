#include "defines.h"
#include "raylib.h"
#include "signals.h"

#include <assert.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <string.h>

// To convert from regular sample count to logarithmic frequency count, pass in NULL for *out_frequencies
// e.g signals_process_samples(LOG_MUL, START_FREQ, 0, SAMPLE_COUNT, NULL, &freq_count_ptr, 0)
void signals_process_samples(f32 scale, f32 start_frequency, f32 *samples, u32 sample_count, f32 *out_frequencies, u32 *out_frequency_count, f32 dt) {
  *out_frequency_count = logf((0.5f*(f32)sample_count)/start_frequency)/logf(scale); 

  if (out_frequencies == NULL) return;

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
