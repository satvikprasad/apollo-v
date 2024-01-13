#include "parameter.h"

#include <string.h>

#include "hashmap.h"

static int parameter_compare(const void *a, const void *b, void *udata) {
  (void)(udata);

  Parameter *pa = (Parameter *)a;
  Parameter *pb = (Parameter *)b;
  return strcmp(pa->name, pb->name);
}

static u64 parameter_hash(const void *item, u64 seed0, u64 seed1) {
  Parameter *pitem = (Parameter *)item;

  return hashmap_sip(pitem->name, strlen(pitem->name), seed0, seed1);
}

HM_Hashmap *parameters_create() {
  return hashmap_new(sizeof(Parameter), 0, 0, 0, parameter_hash,
                     parameter_compare, NULL, NULL);
}

f32 parameter_get(HM_Hashmap *params, char *name) {
  return ((Parameter *)hashmap_get(params, &(Parameter){.name = name}))->value;
}

void parameter_set(HM_Hashmap *params, char *name, f32 value) {
  hashmap_set(params, &(Parameter){.name = name, .value = value});
}
