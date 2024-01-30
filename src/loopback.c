// #define MINIAUDIO_IMPLEMENTATION
#include "loopback.h"
#include "miniaudio.h"
#include "state.h"

#include "defines.h"
#include <stdbool.h>
#include <stdio.h>

void LoopbackDataCallback(ma_device *device, void *output, const void *input,
                          ma_uint32 frame_count) {
    (void)output;

    State *state = (State *)device->pUserData;

    if (state->loopback) {
        const F32(*frame_data)[device->capture.channels] = input;

        for (U32 i = 0; i < frame_count; ++i) {
            StatePushFrame(frame_data[i][0], state->samples, SAMPLE_COUNT);
        }
    }
}

void LoopbackInitialise(LoopbackData *data, void *state) {
    ma_backend backends[] = {ma_backend_wasapi, ma_backend_pulseaudio};

    ma_encoder_config encoder_config =
        ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 44100);

    ma_device_config device_config =
        ma_device_config_init(ma_device_type_loopback);
    device_config.capture.pDeviceID = NULL;
    device_config.capture.format = encoder_config.format;
    device_config.capture.channels = encoder_config.channels;
    device_config.sampleRate = encoder_config.sampleRate;
    device_config.dataCallback = LoopbackDataCallback;
    device_config.pUserData = state;

    ma_result res =
        ma_device_init_ex(backends, sizeof(backends) / sizeof(backends[0]),
                          NULL, &device_config, &data->device);

    if (res != MA_SUCCESS) {
        printf("Failed to open loopback device.\n");
        data->is_initialised = false;
        return;
    }

    res = ma_device_start(&data->device);
    if (res != MA_SUCCESS) {
        printf("Failed to start loopback device.\n");
        data->is_initialised = false;
        return;
    }

    data->is_initialised = true;
}

void LoopbackDestroy(LoopbackData *data) {
    if (data->is_initialised) {
        ma_device_uninit(&data->device);
    }
}
