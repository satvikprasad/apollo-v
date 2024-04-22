#include "parameter.h"

#include <string.h>

#include "hashmap.h"

static I32
ParameterCompare(const void *a, const void *b, void *udata) {
    (void)(udata);

    Parameter *pa = (Parameter *)a;
    Parameter *pb = (Parameter *)b;
    return strcmp(pa->name, pb->name);
}

static U64
ParameterHash(const void *item, U64 seed0, U64 seed1) {
    Parameter *pitem = (Parameter *)item;

    return hashmap_sip(pitem->name, strlen(pitem->name), seed0, seed1);
}

HM_Hashmap *
ParameterCreate() {
    return hashmap_new(sizeof(Parameter), 0, 0, 0, ParameterHash,
                       ParameterCompare, NULL, NULL);
}

void
ParameterDestroy(HM_Hashmap *params) {
    hashmap_free(params);
};

B8
ParameterIter(HM_Hashmap *params, U32 *iter, Parameter **parameter) {
    return hashmap_iter(params, iter, (void **)parameter);
}

Parameter *
ParameterGet(HM_Hashmap *params, const char *name) {
    return (Parameter *)hashmap_get(params, &(Parameter){.name = name});
}

F32
ParameterGetValue(HM_Hashmap *params, const char *name) {
    Parameter *param =
        ((Parameter *)hashmap_get(params, &(Parameter){.name = name}));

    if (param) {
        return param->value;
    }

    return 0.0f;
}

_Parameter
ParameterSet(HM_Hashmap *params, Parameter *param) {
    hashmap_set(params, param);

    return ParameterGeneratePtr(params, param->name);
}

void
ParameterSetValue(HM_Hashmap *params, const char *name, F32 value) {
    Parameter *prev = ParameterGet(params, name);

    if (prev) {
        ParameterSet(params, &(Parameter){.name = name,
                                          .value = value,
                                          .min = prev->min,
                                          .max = prev->max});
    }
}

U32
ParameterCount(HM_Hashmap *params) {
    return hashmap_count(params);
}

void
_ParameterSetValue(_Parameter param, F32 value) {
    ParameterSetValue(param.parameters, param.name, value);
}

F32
_ParameterGetValue(_Parameter param) {
    return ParameterGetValue(param.parameters, param.name);
}

_Parameter
ParameterGeneratePtr(HM_Hashmap *params, const char *name) {
    _Parameter param = (_Parameter){.parameters = params};

    memcpy(param.name, name, strlen(name) + 1);

    return param;
}
