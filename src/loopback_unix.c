#include "loopback.h"

#include "defines.h"
#include "portaudio.h"
#include "state.h"
#include <stdio.h>

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (1)

static void
DumpError(PaError err);

typedef struct LoopbackAudioData {
    F32 left_phase;
    F32 right_phase;
} LoopbackAudioData;

static int
LoopbackCallback(const void                     *input_buffer,
                 void                           *output_buffer,
                 unsigned long                   frames_per_buffer,
                 const PaStreamCallbackTimeInfo *time_info,
                 PaStreamCallbackFlags           status_flags,
                 void                           *user_data) {
    State *state = (State *)user_data;

    if (state->loopback) {
        const F32(*frame_data)[2] = input_buffer;

        state->zero_frequencies = true;

        for (U32 i = 0; i < frames_per_buffer; i++) {
            if (frame_data[i][0] != 0.0f) {
                state->zero_frequencies = false;
            }

            StatePushFrame(frame_data[i][0], state->samples, SAMPLE_COUNT);
        }
    }
    return 0;
}

typedef struct LoopbackData {
    PaStream *stream;
} LoopbackData;

void
LoopbackInitialise(LoopbackData *data, void *state) {
    PaError err = Pa_Initialize();
    DumpError(err);

    err = Pa_OpenDefaultStream(&data->stream, 2, 0, paFloat32, SAMPLE_RATE,
                               FRAMES_PER_BUFFER, LoopbackCallback, state);
    DumpError(err);

    err = Pa_StartStream(data->stream);

    DumpError(err);
}

void
LoopbackDestroy(LoopbackData *data) {
    PaError err = Pa_CloseStream(data->stream);
    DumpError(err);

    err = Pa_Terminate();
    DumpError(err);
}

U32
LoopbackDataSize() {
    return sizeof(LoopbackData);
}

static void
DumpError(PaError err) {
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    }
}
