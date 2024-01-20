#pragma once

#include "defines.h"

typedef struct HM_Bucket {
    U64 hash : 48;
    U64 dib : 16;
} HM_Bucket;

typedef struct HM_Hashmap {
    void *(*malloc)(U32);
    void *(*realloc)(void *, U32);
    void (*free)(void *);
    U32 elsize;
    U32 cap;
    U64 seed0;
    U64 seed1;
    U64 (*hash)(const void *item, U64 seed0, U64 seed1);
    int (*compare)(const void *a, const void *b, void *udata);
    void (*elfree)(void *item);
    void *udata;
    U32 bucketsz;
    U32 nbuckets;
    U32 count;
    U32 mask;
    U32 growat;
    U32 shrinkat;
    U8 loadfactor;
    U8 growpower;
    B8 oom;
    void *buckets;
    void *spare;
    void *edata;
} HM_Hashmap;

HM_Hashmap *hashmap_new(U32 elsize, U32 cap, U64 seed0, U64 seed1,
                        U64 (*hash)(const void *item, U64 seed0, U64 seed1),
                        int (*compare)(const void *a, const void *b,
                                       void *udata),
                        void (*elfree)(void *item), void *udata);

HM_Hashmap *hashmap_new_with_allocator(
    void *(*malloc)(U32), void *(*realloc)(void *, U32), void (*free)(void *),
    U32 elsize, U32 cap, U64 seed0, U64 seed1,
    U64 (*hash)(const void *item, U64 seed0, U64 seed1),
    int (*compare)(const void *a, const void *b, void *udata),
    void (*elfree)(void *item), void *udata);

void hashmap_free(HM_Hashmap *map);
void hashmap_clear(HM_Hashmap *map, B8 update_cap);
U32 hashmap_count(HM_Hashmap *map);
B8 hashmap_oom(HM_Hashmap *map);
const void *hashmap_get(HM_Hashmap *map, const void *item);
const void *hashmap_set(HM_Hashmap *map, const void *item);
const void *hashmap_delete(HM_Hashmap *map, const void *item);
const void *hashmap_probe(HM_Hashmap *map, U64 position);
B8 hashmap_scan(HM_Hashmap *map, B8 (*iter)(const void *item, void *udata),
                void *udata);
B8 hashmap_iter(HM_Hashmap *map, U32 *i, void **item);

U64 hashmap_sip(const void *data, U32 len, U64 seed0, U64 seed1);
U64 hashmap_murmur(const void *data, U32 len, U64 seed0, U64 seed1);
U64 hashmap_xxhash3(const void *data, U32 len, U64 seed0, U64 seed1);

const void *hashmap_get_with_hash(HM_Hashmap *map, const void *key, U64 hash);
const void *hashmap_delete_with_hash(HM_Hashmap *map, const void *key,
                                     U64 hash);
const void *hashmap_set_with_hash(HM_Hashmap *map, const void *item, U64 hash);
void hashmap_set_grow_by_power(HM_Hashmap *map, U32 power);
void hashmap_set_load_factor(HM_Hashmap *map, F64 load_factor);
