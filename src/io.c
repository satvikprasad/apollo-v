#include "io.h"

#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "filesystem.h"
#include "handmademath.h"
#include "state.h"

static void WriteString(const char *str, FILE *fptr) {
    U32 len = strlen(str);

    fwrite(&len, sizeof(U32), 1, fptr);
    fwrite(str, sizeof(char), len + 1, fptr);
}

static void ReadString(char *str, FILE *fptr) {
    U32 len;
    fread(&len, sizeof(U32), 1, fptr);
    fread(str, sizeof(char), len + 1, fptr);
}

void Serialize(State *state) {
    FILE *fptr;

    fptr = fopen(FSFormatDataDirectory("data.ly"), "wb");

    if (fptr) {
        fwrite(&state->screen_size, sizeof(HMM_Vec2), 1, fptr);
        fwrite(&state->window_position, sizeof(HMM_Vec2), 1, fptr);
        fwrite(&state->master_volume, sizeof(F32), 1, fptr);

        fwrite(&state->should_open_on_loopback, sizeof(B8), 1, fptr);

        WriteString(state->music_fp, fptr);

        U32 param_count = ParameterCount(state->parameters);
        fwrite(&param_count, sizeof(U32), 1, fptr);

        Parameter *parameter;
        U32 _ = 0;
        while (ParameterIter(state->parameters, &_, &parameter)) {
            WriteString(parameter->name, fptr);
            fwrite(&parameter->value, sizeof(F32), 1, fptr);
            fwrite(&parameter->min, sizeof(F32), 1, fptr);
            fwrite(&parameter->max, sizeof(F32), 1, fptr);
        }

        fclose(fptr);
    } else {
        printf("Failed to open file %s\n", FSFormatDataDirectory("data.ly"));
    }
}

bool Deserialize(State *state) {
    if (FileExists(FSFormatDataDirectory("data.ly"))) {
        FILE *fptr;
        fptr = fopen(FSFormatDataDirectory("data.ly"), "rb");
        {
            fread(&state->screen_size, sizeof(HMM_Vec2), 1, fptr);
            fread(&state->window_position, sizeof(HMM_Vec2), 1, fptr);
            fread(&state->master_volume, sizeof(F32), 1, fptr);

            B8 loopback;
            fread(&loopback, sizeof(B8), 1, fptr);

            if (loopback) {
                state->condition = StateCondition_LOOPBACK;
            }

            ReadString(state->music_fp, fptr);

            U32 param_count;
            fread(&param_count, sizeof(U32), 1, fptr);

            for (U32 i = 0; i < param_count; ++i) {
                char buf[256];
                F32 value, min, max;

                ReadString(buf, fptr);

                char *name = ArenaPushString(&state->arena, buf);

                fread(&value, sizeof(F32), 1, fptr);
                fread(&min, sizeof(F32), 1, fptr);
                fread(&max, sizeof(F32), 1, fptr);

                if (hashmap_get(state->parameters,
                                &(Parameter){.name = name})) {
                    ParameterSet(state->parameters, &(Parameter){.name = name,
                                                                 .value = value,
                                                                 .min = min,
                                                                 .max = max});
                }
            }
        }
        fclose(fptr);

        return true;
    }

    return false;
}
