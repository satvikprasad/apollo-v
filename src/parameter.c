#include "parameter.h"

#include <string.h>

#include "hashmap.h"

static I32 ParameterCompare(const void *a, const void *b, void *udata) {
    (void)(udata);

    Parameter *pa = (Parameter *)a;
    Parameter *pb = (Parameter *)b;
    return strcmp(pa->name, pb->name);
}

static U64 ParameterHash(const void *item, U64 seed0, U64 seed1) {
    Parameter *pitem = (Parameter *)item;

    return hashmap_sip(pitem->name, strlen(pitem->name), seed0, seed1);
}

HM_Hashmap *ParameterCreate() {
    return hashmap_new(sizeof(Parameter), 0, 0, 0, ParameterHash,
                       ParameterCompare, NULL, NULL);
}

void ParameterDestroy(Parameters *params) { hashmap_free(params); };

B8 ParameterIter(Parameters *params, U32 *iter, Parameter **parameter) {
    return hashmap_iter(params, iter, (void **)parameter);
}

Parameter *ParameterGet(Parameters *params, const char *name) {
    return (Parameter *)hashmap_get(params, &(Parameter){.name = name});
}

F32 ParameterGetValue(Parameters *params, const char *name) {
    Parameter *param =
        ((Parameter *)hashmap_get(params, &(Parameter){.name = name}));

    return param->value;
}

void ParameterSet(Parameters *params, Parameter *param) {
    hashmap_set(params, param);
}

void ParameterSetValue(Parameters *params, const char *name, F32 value) {
    Parameter *prev = ParameterGet(params, name);

    ParameterSet(params, &(Parameter){.name = name,
                                      .value = value,
                                      .min = prev->min,
                                      .max = prev->max});
}

U32 ParameterCount(Parameters *params) { return hashmap_count(params); }
