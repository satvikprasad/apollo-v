#include "state.h"
#include "defines.h"
#include "raylib.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static inline void FFT(f32 in[], u32 stride, float complex out[], u32 n);

f32 freq_in[FREQ_COUNT];
float complex freq[FREQ_COUNT];
Frame frames[1024];
u32 frame_count;

f32 Amp(float complex z) {
	f32 a = fabsf(crealf(z));
	f32 b = fabsf(cimagf(z));

	if (a < b) return b;

	return a;
}

void Callback(void *buffer_data, u32 n) {
	u32 capacity = ARRAY_LEN(frames);

	if (capacity - frame_count >= n) {
		memcpy(frames + frame_count, buffer_data, sizeof(Frame)*n);
		frame_count += n;
	} else if (n <= capacity) {
		memmove(frames, frames + n, sizeof(Frame)*(capacity - n));
		memcpy(frames + (capacity - n), buffer_data, sizeof(Frame)*n);
	} else {
		memcpy(frames, buffer_data, sizeof(Frame)*capacity);
		frame_count = capacity;
	}

	if (n > FREQ_COUNT) n = FREQ_COUNT;

	Frame *frame_data = buffer_data;

	for (u32 i = 0; i < n; ++i) {
		freq_in[i] = frame_data[i].left;	
	}

}

void StateInitialise(State *state, const char *fp) {
	state->music = LoadMusicStream(fp);
	assert(state->music.stream.sampleSize == 32);
	assert(state->music.stream.channels == 2);

	SetMusicVolume(state->music, 0.0f);
	PlayMusicStream(state->music);
	AttachAudioStreamProcessor(state->music.stream, Callback);
}

void StateDetach(State *state) {
	DetachAudioStreamProcessor(state->music.stream, Callback);
}

void StateAttach(State *state) {
	AttachAudioStreamProcessor(state->music.stream, Callback);
}

void StateUpdate(State *state) {
	UpdateMusicStream(state->music);

	if(IsKeyPressed(KEY_SPACE)) {
		if (IsMusicStreamPlaying(state->music)) {
			PauseMusicStream(state->music);
		} else {
			ResumeMusicStream(state->music);
		}
	}

	i32 w = GetRenderWidth();
	i32 h = GetRenderHeight();

	FFT(freq_in, 1, freq, FREQ_COUNT);
	
	f32 max_amp = 0.0f;
	for (u32 i = 0; i < FREQ_COUNT; ++i) {
		float c = Amp(freq[i]);
		if (max_amp < c) max_amp = c;
	}

	BeginDrawing();
	ClearBackground(RAYWHITE);

	f32 cell_width = ((f32)GetRenderWidth())/((f32)FREQ_COUNT);
	if (cell_width < 1.f) cell_width = 1.f;
	for (u32 i = 0; i < FREQ_COUNT; ++i) {
		f32 t = Amp(freq[i])/max_amp;
		DrawRectangle(i*cell_width, h-(h/2)*t, cell_width, h, (Color){t*200, t*125, sin(t*GetTime())*50 + 200, 255});
//		DrawCircleLines(w/2, h/2, (h/2)*t, (Color){t*200, t*125, t*sin(10*GetTime())*200, 255});
	}

	u32 count = 1024;
	cell_width = (f32)w/count;

	if (cell_width < 1.f) cell_width = 1.f;
	for (u32 i = 0; i < count; ++i) {
		f32 a = ((f32)i)/((f32)count);
		i32 index = (i32)(a*frame_count);

		f32 t = frames[index].left;

		if (t > 0) {
			DrawRectangle(i*cell_width, h/2 - h/2*t, cell_width, h/2*t, RED);
		} else {
			DrawRectangle(i*cell_width, h/2, cell_width, h/2*-t, RED);
		}
	}

	EndDrawing();	
}

static inline void FFT(f32 in[], u32 stride, float complex out[], u32 n) {
	assert(n > 0);

	if (n == 1) {
		out[0] = in[0];
		return;
	}

	FFT(in, stride*2, out, n/2);
	FFT(in + stride, stride*2, out + n/2, n/2);

	for (u32 k = 0; k < n/2; ++k) {
		f32 t = (f32)k / n;

		float complex v = cexp(-2*I*PI*t)*out[k + n/2];
		float complex e = out[k];

		out[k] = e + v;
		out[k + n/2] = e - v;
	}
}
