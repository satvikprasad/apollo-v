#pragma once

#include "defines.h"

typedef struct HM_Bucket {
  u64 hash : 48;
  u64 dib : 16;
} HM_Bucket;

typedef struct HM_Hashmap {
  void *(*malloc)(u32);
  void *(*realloc)(void *, u32);
  void (*free)(void *);
  u32 elsize;
  u32 cap;
  u64 seed0;
  u64 seed1;
  u64 (*hash)(const void *item, u64 seed0, u64 seed1);
  int (*compare)(const void *a, const void *b, void *udata);
  void (*elfree)(void *item);
  void *udata;
  u32 bucketsz;
  u32 nbuckets;
  u32 count;
  u32 mask;
  u32 growat;
  u32 shrinkat;
  u8 loadfactor;
  u8 growpower;
  b8 oom;
  void *buckets;
  void *spare;
  void *edata;
} HM_Hashmap;

HM_Hashmap *hashmap_new(u32 elsize, u32 cap, u64 seed0, u64 seed1,
                        u64 (*hash)(const void *item, u64 seed0, u64 seed1),
                        int (*compare)(const void *a, const void *b,
                                       void *udata),
                        void (*elfree)(void *item), void *udata);

HM_Hashmap *hashmap_new_with_allocator(
    void *(*malloc)(u32), void *(*realloc)(void *, u32), void (*free)(void *),
    u32 elsize, u32 cap, u64 seed0, u64 seed1,
    u64 (*hash)(const void *item, u64 seed0, u64 seed1),
    int (*compare)(const void *a, const void *b, void *udata),
    void (*elfree)(void *item), void *udata);

void hashmap_free(HM_Hashmap *map);
void hashmap_clear(HM_Hashmap *map, b8 update_cap);
u32 hashmap_count(HM_Hashmap *map);
b8 hashmap_oom(HM_Hashmap *map);
const void *hashmap_get(HM_Hashmap *map, const void *item);
const void *hashmap_set(HM_Hashmap *map, const void *item);
const void *hashmap_delete(HM_Hashmap *map, const void *item);
const void *hashmap_probe(HM_Hashmap *map, u64 position);
b8 hashmap_scan(HM_Hashmap *map, b8 (*iter)(const void *item, void *udata),
                void *udata);
b8 hashmap_iter(HM_Hashmap *map, u32 *i, void **item);

u64 hashmap_sip(const void *data, u32 len, u64 seed0, u64 seed1);
u64 hashmap_murmur(const void *data, u32 len, u64 seed0, u64 seed1);
u64 hashmap_xxhash3(const void *data, u32 len, u64 seed0, u64 seed1);

const void *hashmap_get_with_hash(HM_Hashmap *map, const void *key, u64 hash);
const void *hashmap_delete_with_hash(HM_Hashmap *map, const void *key,
                                     u64 hash);
const void *hashmap_set_with_hash(HM_Hashmap *map, const void *item, u64 hash);
void hashmap_set_grow_by_power(HM_Hashmap *map, u32 power);
void hashmap_set_load_factor(HM_Hashmap *map, f64 load_factor);
