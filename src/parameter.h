#pragma once

#include "defines.h"
#include "hashmap.h"

typedef struct Parameter {
    const char *name;
    F32         value, min, max;
} Parameter;

HM_Hashmap *
ParameterCreate();
void
ParameterDestroy(HM_Hashmap *params);
B8
ParameterIter(HM_Hashmap *params, U32 *iter, Parameter **parameter);

Parameter *
ParameterGet(HM_Hashmap *params, const char *name);
F32
ParameterGetValue(HM_Hashmap *params, const char *name);

void
ParameterSet(HM_Hashmap *params, Parameter *param);
void
ParameterSetValue(HM_Hashmap *params, const char *name, F32 value);
U32
ParameterCount(HM_Hashmap *params);
