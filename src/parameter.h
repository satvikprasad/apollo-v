#pragma once

#include "defines.h"
#include "hashmap.h"

typedef HM_Hashmap Parameters;

typedef struct Parameter {
    const char *name;
    F32 value, min, max;
} Parameter;

HM_Hashmap *ParameterCreate();
void ParameterDestroy(Parameters *params);
B8 ParameterIter(Parameters *params, U32 *iter, Parameter **parameter);

Parameter *ParameterGet(Parameters *params, const char *name);
F32 ParameterGetValue(Parameters *params, const char *name);

void ParameterSet(Parameters *params, Parameter *param);
void ParameterSetValue(Parameters *params, const char *name, F32 value);
U32 ParameterCount(Parameters *params);
