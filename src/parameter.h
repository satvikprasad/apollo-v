#pragma once

#include "defines.h"
#include "hashmap.h"

typedef struct Parameter {
  char *name;
  f32 value;
} Parameter;

HM_Hashmap *parameters_create();
f32 parameter_get(HM_Hashmap *params, char *name);
void parameter_set(HM_Hashmap *params, char *name, f32 value);
