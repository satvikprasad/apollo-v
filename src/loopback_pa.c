#include "defines.h"

#include "loopback_pa.h"
#include "portaudio.h"
#include <stdio.h>

void
LoopbackInitialisePa(LoopbackDataPa *loopback_data) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    }
}

void
LoopbackDestroyPa(LoopbackDataPa *loopback_data) {
    PaError err = Pa_Terminate();
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    }
}
