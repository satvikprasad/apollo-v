// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "hashmap.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GROW_AT 0.60   /* 60% */
#define SHRINK_AT 0.10 /* 10% */

#ifndef HASHMAP_LOAD_FACTOR
#define HASHMAP_LOAD_FACTOR GROW_AT
#endif

static void *(*__malloc)(U32) = NULL;
static void *(*__realloc)(void *, U32) = NULL;
static void (*__free)(void *) = NULL;

// hashmap_set_allocator allows for configuring a custom allocator for
// all hashmap library operations. This function, if needed, should be called
// only once at startup and a prior to calling hashmap_new().
void hashmap_set_allocator(void *(*malloc)(U32), void (*free)(void *)) {
    __malloc = malloc;
    __free = free;
}

void hashmap_set_grow_by_power(HM_Hashmap *map, U32 power) {
    map->growpower = power < 1 ? 1 : power > 16 ? 16 : power;
}

static F64 clamp_load_factor(F64 factor, F64 default_factor) {
    // Check for NaN and clamp between 50% and 90%
    return factor != factor ? default_factor
           : factor < 0.50  ? 0.50
           : factor > 0.95  ? 0.95
                            : factor;
}

void hashmap_set_load_factor(HM_Hashmap *map, F64 factor) {
    factor = clamp_load_factor(factor, map->loadfactor / 100.0);
    map->loadfactor = factor * 100;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
}

static HM_Bucket *bucket_at0(void *buckets, U32 bucketsz, U32 i) {
    return (HM_Bucket *)(((char *)buckets) + (bucketsz * i));
}

static HM_Bucket *bucket_at(HM_Hashmap *map, U32 index) {
    return bucket_at0(map->buckets, map->bucketsz, index);
}

static void *bucket_item(HM_Bucket *entry) {
    return ((char *)entry) + sizeof(HM_Bucket);
}

static U64 clip_hash(U64 hash) { return hash & 0xFFFFFFFFFFFF; }

static U64 get_hash(HM_Hashmap *map, const void *key) {
    return clip_hash(map->hash(key, map->seed0, map->seed1));
}

// hashmap_new_with_allocator returns a new hash map using a custom allocator.
// See hashmap_new for more information information
HM_Hashmap *hashmap_new_with_allocator(
    void *(*_malloc)(U32), void *(*_realloc)(void *, U32),
    void (*_free)(void *), U32 elsize, U32 cap, U64 seed0, U64 seed1,
    U64 (*hash)(const void *item, U64 seed0, U64 seed1),
    I32 (*compare)(const void *a, const void *b, void *udata),
    void (*elfree)(void *item), void *udata) {
    _malloc = _malloc ? _malloc : __malloc ? __malloc : malloc;
    _realloc = _realloc ? _realloc : __realloc ? __realloc : realloc;
    _free = _free ? _free : __free ? __free : free;
    U32 ncap = 16;
    if (cap < ncap) {
        cap = ncap;
    } else {
        while (ncap < cap) {
            ncap *= 2;
        }
        cap = ncap;
    }
    U32 bucketsz = sizeof(HM_Bucket) + elsize;
    while (bucketsz & (sizeof(uintptr_t) - 1)) {
        bucketsz++;
    }
    // hashmap + spare + edata
    U32 size = sizeof(HM_Hashmap) + bucketsz * 2;
    HM_Hashmap *map = _malloc(size);
    if (!map) {
        return NULL;
    }
    memset(map, 0, sizeof(HM_Hashmap));
    map->elsize = elsize;
    map->bucketsz = bucketsz;
    map->seed0 = seed0;
    map->seed1 = seed1;
    map->hash = hash;
    map->compare = compare;
    map->elfree = elfree;
    map->udata = udata;
    map->spare = ((char *)map) + sizeof(HM_Hashmap);
    map->edata = (char *)map->spare + bucketsz;
    map->cap = cap;
    map->nbuckets = cap;
    map->mask = map->nbuckets - 1;
    map->buckets = _malloc(map->bucketsz * map->nbuckets);
    if (!map->buckets) {
        _free(map);
        return NULL;
    }
    memset(map->buckets, 0, map->bucketsz * map->nbuckets);
    map->growpower = 1;
    map->loadfactor = clamp_load_factor(HASHMAP_LOAD_FACTOR, GROW_AT) * 100;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;
    map->malloc = _malloc;
    map->realloc = _realloc;
    map->free = _free;
    return map;
}

// hashmap_new returns a new hash map.
// Param `elsize` is the size of each element in the tree. Every element that
// is inserted, deleted, or retrieved will be this size.
// Param `cap` is the default lower capacity of the hashmap. Setting this to
// zero will default to 16.
// Params `seed0` and `seed1` are optional seed values that are passed to the
// following `hash` function. These can be any value you wish but it's often
// best to use randomly generated values.
// Param `hash` is a function that generates a hash value for an item. It's
// important that you provide a good hash function, otherwise it will perform
// poorly or be vulnerable to Denial-of-service attacks. This implementation
// comes with two helper functions `hashmap_sip()` and `hashmap_murmur()`.
// Param `compare` is a function that compares items in the tree. See the
// qsort stdlib function for an example of how this function works.
// The hashmap must be freed with hashmap_free().
// Param `elfree` is a function that frees a specific item. This should be NULL
// unless you're storing some kind of reference data in the hash.
HM_Hashmap *hashmap_new(U32 elsize, U32 cap, U64 seed0, U64 seed1,
                        U64 (*hash)(const void *item, U64 seed0, U64 seed1),
                        I32 (*compare)(const void *a, const void *b,
                                       void *udata),
                        void (*elfree)(void *item), void *udata) {
    return hashmap_new_with_allocator(NULL, NULL, NULL, elsize, cap, seed0,
                                      seed1, hash, compare, elfree, udata);
}

static void free_elements(HM_Hashmap *map) {
    if (map->elfree) {
        for (U32 i = 0; i < map->nbuckets; i++) {
            HM_Bucket *bucket = bucket_at(map, i);
            if (bucket->dib)
                map->elfree(bucket_item(bucket));
        }
    }
}

// hashmap_clear quickly clears the map.
// Every item is called with the element-freeing function given in hashmap_new,
// if present, to free any data referenced in the elements of the hashmap.
// When the update_cap is provided, the map's capacity will be updated to match
// the currently number of allocated buckets. This is an optimization to ensure
// that this operation does not perform any allocations.
void hashmap_clear(HM_Hashmap *map, B8 update_cap) {
    map->count = 0;
    free_elements(map);
    if (update_cap) {
        map->cap = map->nbuckets;
    } else if (map->nbuckets != map->cap) {
        void *new_buckets = map->malloc(map->bucketsz * map->cap);
        if (new_buckets) {
            map->free(map->buckets);
            map->buckets = new_buckets;
        }
        map->nbuckets = map->cap;
    }
    memset(map->buckets, 0, map->bucketsz * map->nbuckets);
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;
}

static B8 resize0(HM_Hashmap *map, U32 new_cap) {
    HM_Hashmap *map2 = hashmap_new_with_allocator(
        map->malloc, map->realloc, map->free, map->elsize, new_cap, map->seed0,
        map->seed1, map->hash, map->compare, map->elfree, map->udata);
    if (!map2)
        return false;
    for (U32 i = 0; i < map->nbuckets; i++) {
        HM_Bucket *entry = bucket_at(map, i);
        if (!entry->dib) {
            continue;
        }
        entry->dib = 1;
        U32 j = entry->hash & map2->mask;
        while (1) {
            HM_Bucket *bucket = bucket_at(map2, j);
            if (bucket->dib == 0) {
                memcpy(bucket, entry, map->bucketsz);
                break;
            }
            if (bucket->dib < entry->dib) {
                memcpy(map2->spare, bucket, map->bucketsz);
                memcpy(bucket, entry, map->bucketsz);
                memcpy(entry, map2->spare, map->bucketsz);
            }
            j = (j + 1) & map2->mask;
            entry->dib += 1;
        }
    }
    map->free(map->buckets);
    map->buckets = map2->buckets;
    map->nbuckets = map2->nbuckets;
    map->mask = map2->mask;
    map->growat = map2->growat;
    map->shrinkat = map2->shrinkat;
    map->free(map2);
    return true;
}

static B8 resize(HM_Hashmap *map, U32 new_cap) { return resize0(map, new_cap); }

// hashmap_set_with_hash works like hashmap_set but you provide your
// own hash. The 'hash' callback provided to the hashmap_new function
// will not be called
const void *hashmap_set_with_hash(HM_Hashmap *map, const void *item, U64 hash) {
    hash = clip_hash(hash);
    map->oom = false;
    if (map->count >= map->growat) {
        if (!resize(map, map->nbuckets * (1 << map->growpower))) {
            map->oom = true;
            return NULL;
        }
    }

    HM_Bucket *entry = map->edata;
    entry->hash = hash;
    entry->dib = 1;
    void *eitem = bucket_item(entry);
    memcpy(eitem, item, map->elsize);

    void *bitem;
    U32 i = entry->hash & map->mask;
    while (1) {
        HM_Bucket *bucket = bucket_at(map, i);
        if (bucket->dib == 0) {
            memcpy(bucket, entry, map->bucketsz);
            map->count++;
            return NULL;
        }
        bitem = bucket_item(bucket);
        if (entry->hash == bucket->hash &&
            (!map->compare || map->compare(eitem, bitem, map->udata) == 0)) {
            memcpy(map->spare, bitem, map->elsize);
            memcpy(bitem, eitem, map->elsize);
            return map->spare;
        }
        if (bucket->dib < entry->dib) {
            memcpy(map->spare, bucket, map->bucketsz);
            memcpy(bucket, entry, map->bucketsz);
            memcpy(entry, map->spare, map->bucketsz);
            eitem = bucket_item(entry);
        }
        i = (i + 1) & map->mask;
        entry->dib += 1;
    }
}

// hashmap_set inserts or replaces an item in the hash map. If an item is
// replaced then it is returned otherwise NULL is returned. This operation
// may allocate memory. If the system is unable to allocate additional
// memory then NULL is returned and hashmap_oom() returns true.
const void *hashmap_set(HM_Hashmap *map, const void *item) {
    return hashmap_set_with_hash(map, item, get_hash(map, item));
}

// hashmap_get_with_hash works like hashmap_get but you provide your
// own hash. The 'hash' callback provided to the hashmap_new function
// will not be called
const void *hashmap_get_with_hash(HM_Hashmap *map, const void *key, U64 hash) {
    hash = clip_hash(hash);
    U32 i = hash & map->mask;
    while (1) {
        HM_Bucket *bucket = bucket_at(map, i);
        if (!bucket->dib)
            return NULL;
        if (bucket->hash == hash) {
            void *bitem = bucket_item(bucket);
            if (!map->compare || map->compare(key, bitem, map->udata) == 0) {
                return bitem;
            }
        }
        i = (i + 1) & map->mask;
    }
}

// hashmap_get returns the item based on the provided key. If the item is not
// found then NULL is returned.
const void *hashmap_get(HM_Hashmap *map, const void *key) {
    return hashmap_get_with_hash(map, key, get_hash(map, key));
}

// hashmap_probe returns the item in the bucket at position or NULL if an item
// is not set for that bucket. The position is 'moduloed' by the number of
// buckets in the hashmap.
const void *hashmap_probe(HM_Hashmap *map, U64 position) {
    U32 i = position & map->mask;
    HM_Bucket *bucket = bucket_at(map, i);
    if (!bucket->dib) {
        return NULL;
    }
    return bucket_item(bucket);
}

// hashmap_delete_with_hash works like hashmap_delete but you provide your
// own hash. The 'hash' callback provided to the hashmap_new function
// will not be called
const void *hashmap_delete_with_hash(HM_Hashmap *map, const void *key,
                                     U64 hash) {
    hash = clip_hash(hash);
    map->oom = false;
    U32 i = hash & map->mask;
    while (1) {
        HM_Bucket *bucket = bucket_at(map, i);
        if (!bucket->dib) {
            return NULL;
        }
        void *bitem = bucket_item(bucket);
        if (bucket->hash == hash &&
            (!map->compare || map->compare(key, bitem, map->udata) == 0)) {
            memcpy(map->spare, bitem, map->elsize);
            bucket->dib = 0;
            while (1) {
                HM_Bucket *prev = bucket;
                i = (i + 1) & map->mask;
                bucket = bucket_at(map, i);
                if (bucket->dib <= 1) {
                    prev->dib = 0;
                    break;
                }
                memcpy(prev, bucket, map->bucketsz);
                prev->dib--;
            }
            map->count--;
            if (map->nbuckets > map->cap && map->count <= map->shrinkat) {
                // Ignore the return value. It's ok for the resize operation to
                // fail to allocate enough memory because a shrink operation
                // does not change the integrity of the data.
                resize(map, map->nbuckets / 2);
            }
            return map->spare;
        }
        i = (i + 1) & map->mask;
    }
}

// hashmap_delete removes an item from the hash map and returns it. If the
// item is not found then NULL is returned.
const void *hashmap_delete(HM_Hashmap *map, const void *key) {
    return hashmap_delete_with_hash(map, key, get_hash(map, key));
}

// hashmap_count returns the number of items in the hash map.
U32 hashmap_count(HM_Hashmap *map) { return map->count; }

// hashmap_free frees the hash map
// Every item is called with the element-freeing function given in hashmap_new,
// if present, to free any data referenced in the elements of the hashmap.
void hashmap_free(HM_Hashmap *map) {
    if (!map)
        return;
    free_elements(map);
    map->free(map->buckets);
    map->free(map);
}

// hashmap_oom returns true if the last hashmap_set() call failed due to the
// system being out of memory.
B8 hashmap_oom(HM_Hashmap *map) { return map->oom; }

// hashmap_scan iterates over all items in the hash map
// Param `iter` can return false to stop iteration early.
// Returns false if the iteration has been stopped early.
B8 hashmap_scan(HM_Hashmap *map, B8 (*iter)(const void *item, void *udata),
                void *udata) {
    for (U32 i = 0; i < map->nbuckets; i++) {
        HM_Bucket *bucket = bucket_at(map, i);
        if (bucket->dib && !iter(bucket_item(bucket), udata)) {
            return false;
        }
    }
    return true;
}

// hashmap_iter iterates one key at a time yielding a reference to an
// entry at each iteration. Useful to write simple loops and avoid writing
// dedicated callbacks and udata structures, as in hashmap_scan.
//
// map is a hash map handle. i is a pointer to a U32 cursor that
// should be initialized to 0 at the beginning of the loop. item is a void
// pointer pointer that is populated with the retrieved item. Note that this
// is NOT a copy of the item stored in the hash map and can be directly
// modified.
//
// Note that if hashmap_delete() is called on the hashmap being iterated,
// the buckets are rearranged and the iterator must be reset to 0, otherwise
// unexpected results may be returned after deletion.
//
// This function has not been tested for thread safety.
//
// The function returns true if an item was retrieved; false if the end of the
// iteration has been reached.
B8 hashmap_iter(HM_Hashmap *map, U32 *i, void **item) {
    HM_Bucket *bucket;
    do {
        if (*i >= map->nbuckets)
            return false;
        bucket = bucket_at(map, *i);
        (*i)++;
    } while (!bucket->dib);
    *item = bucket_item(bucket);
    return true;
}

//-----------------------------------------------------------------------------
// SipHash reference C implementation
//
// Copyright (c) 2012-2016 Jean-Philippe Aumasson
// <jeanphilippe.aumasson@gmail.com>
// Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
//
// To the extent possible under law, the author(s) have dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along
// with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
//
// default: SipHash-2-4
//-----------------------------------------------------------------------------
static U64 SIP64(const uint8_t *in, const U32 inlen, U64 seed0, U64 seed1) {
#define U8TO64_LE(p)                                                           \
    {(((U64)((p)[0])) | ((U64)((p)[1]) << 8) | ((U64)((p)[2]) << 16) |         \
      ((U64)((p)[3]) << 24) | ((U64)((p)[4]) << 32) | ((U64)((p)[5]) << 40) |  \
      ((U64)((p)[6]) << 48) | ((U64)((p)[7]) << 56))}
#define U64TO8_LE(p, v)                                                        \
    {                                                                          \
        U32TO8_LE((p), (uint32_t)((v)));                                       \
        U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));                             \
    }
#define U32TO8_LE(p, v)                                                        \
    {                                                                          \
        (p)[0] = (uint8_t)((v));                                               \
        (p)[1] = (uint8_t)((v) >> 8);                                          \
        (p)[2] = (uint8_t)((v) >> 16);                                         \
        (p)[3] = (uint8_t)((v) >> 24);                                         \
    }
#define ROTL(x, b) (U64)(((x) << (b)) | ((x) >> (64 - (b))))
#define SIPROUND                                                               \
    {                                                                          \
        v0 += v1;                                                              \
        v1 = ROTL(v1, 13);                                                     \
        v1 ^= v0;                                                              \
        v0 = ROTL(v0, 32);                                                     \
        v2 += v3;                                                              \
        v3 = ROTL(v3, 16);                                                     \
        v3 ^= v2;                                                              \
        v0 += v3;                                                              \
        v3 = ROTL(v3, 21);                                                     \
        v3 ^= v0;                                                              \
        v2 += v1;                                                              \
        v1 = ROTL(v1, 17);                                                     \
        v1 ^= v2;                                                              \
        v2 = ROTL(v2, 32);                                                     \
    }
    U64 k0 = U8TO64_LE((uint8_t *)&seed0);
    U64 k1 = U8TO64_LE((uint8_t *)&seed1);
    U64 v3 = UINT64_C(0x7465646279746573) ^ k1;
    U64 v2 = UINT64_C(0x6c7967656e657261) ^ k0;
    U64 v1 = UINT64_C(0x646f72616e646f6d) ^ k1;
    U64 v0 = UINT64_C(0x736f6d6570736575) ^ k0;
    const uint8_t *end = in + inlen - (inlen % sizeof(U64));
    for (; in != end; in += 8) {
        U64 m = U8TO64_LE(in);
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }
    const I32 left = inlen & 7;
    U64 b = ((U64)inlen) << 56;
    switch (left) {
    case 7:
        b |= ((U64)in[6]) << 48; /* fall through */
    case 6:
        b |= ((U64)in[5]) << 40; /* fall through */
    case 5:
        b |= ((U64)in[4]) << 32; /* fall through */
    case 4:
        b |= ((U64)in[3]) << 24; /* fall through */
    case 3:
        b |= ((U64)in[2]) << 16; /* fall through */
    case 2:
        b |= ((U64)in[1]) << 8; /* fall through */
    case 1:
        b |= ((U64)in[0]);
        break;
    case 0:
        break;
    }
    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    b = v0 ^ v1 ^ v2 ^ v3;
    U64 out = 0;
    U64TO8_LE((uint8_t *)&out, b);
    return out;
}

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// Murmur3_86_128
//-----------------------------------------------------------------------------
static U64 MM86128(const void *key, const I32 len, uint32_t seed) {
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))
#define FMIX32(h)                                                              \
    h ^= h >> 16;                                                              \
    h *= 0x85ebca6b;                                                           \
    h ^= h >> 13;                                                              \
    h *= 0xc2b2ae35;                                                           \
    h ^= h >> 16;
    const uint8_t *data = (const uint8_t *)key;
    const I32 nblocks = len / 16;
    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;
    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;
    const uint32_t *blocks = (const uint32_t *)(data + nblocks * 16);
    for (I32 i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i * 4 + 0];
        uint32_t k2 = blocks[i * 4 + 1];
        uint32_t k3 = blocks[i * 4 + 2];
        uint32_t k4 = blocks[i * 4 + 3];
        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        h1 = ROTL32(h1, 19);
        h1 += h2;
        h1 = h1 * 5 + 0x561ccd1b;
        k2 *= c2;
        k2 = ROTL32(k2, 16);
        k2 *= c3;
        h2 ^= k2;
        h2 = ROTL32(h2, 17);
        h2 += h3;
        h2 = h2 * 5 + 0x0bcaa747;
        k3 *= c3;
        k3 = ROTL32(k3, 17);
        k3 *= c4;
        h3 ^= k3;
        h3 = ROTL32(h3, 15);
        h3 += h4;
        h3 = h3 * 5 + 0x96cd1c35;
        k4 *= c4;
        k4 = ROTL32(k4, 18);
        k4 *= c1;
        h4 ^= k4;
        h4 = ROTL32(h4, 13);
        h4 += h1;
        h4 = h4 * 5 + 0x32ac3b17;
    }
    const uint8_t *tail = (const uint8_t *)(data + nblocks * 16);
    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;
    switch (len & 15) {
    case 15:
        k4 ^= tail[14] << 16; /* fall through */
    case 14:
        k4 ^= tail[13] << 8; /* fall through */
    case 13:
        k4 ^= tail[12] << 0;
        k4 *= c4;
        k4 = ROTL32(k4, 18);
        k4 *= c1;
        h4 ^= k4;
        /* fall through */
    case 12:
        k3 ^= tail[11] << 24; /* fall through */
    case 11:
        k3 ^= tail[10] << 16; /* fall through */
    case 10:
        k3 ^= tail[9] << 8; /* fall through */
    case 9:
        k3 ^= tail[8] << 0;
        k3 *= c3;
        k3 = ROTL32(k3, 17);
        k3 *= c4;
        h3 ^= k3;
        /* fall through */
    case 8:
        k2 ^= tail[7] << 24; /* fall through */
    case 7:
        k2 ^= tail[6] << 16; /* fall through */
    case 6:
        k2 ^= tail[5] << 8; /* fall through */
    case 5:
        k2 ^= tail[4] << 0;
        k2 *= c2;
        k2 = ROTL32(k2, 16);
        k2 *= c3;
        h2 ^= k2;
        /* fall through */
    case 4:
        k1 ^= tail[3] << 24; /* fall through */
    case 3:
        k1 ^= tail[2] << 16; /* fall through */
    case 2:
        k1 ^= tail[1] << 8; /* fall through */
    case 1:
        k1 ^= tail[0] << 0;
        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        /* fall through */
    };
    h1 ^= len;
    h2 ^= len;
    h3 ^= len;
    h4 ^= len;
    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;
    FMIX32(h1);
    FMIX32(h2);
    FMIX32(h3);
    FMIX32(h4);
    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;
    return (((U64)h2) << 32) | h1;
}

//-----------------------------------------------------------------------------
// xxHash Library
// Copyright (c) 2012-2021 Yann Collet
// All rights reserved.
//
// BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
//
// xxHash3
//-----------------------------------------------------------------------------
#define XXH_PRIME_1 11400714785074694791ULL
#define XXH_PRIME_2 14029467366897019727ULL
#define XXH_PRIME_3 1609587929392839161ULL
#define XXH_PRIME_4 9650029242287828579ULL
#define XXH_PRIME_5 2870177450012600261ULL

static U64 XXH_read64(const void *memptr) {
    U64 val;
    memcpy(&val, memptr, sizeof(val));
    return val;
}

static uint32_t XXH_read32(const void *memptr) {
    uint32_t val;
    memcpy(&val, memptr, sizeof(val));
    return val;
}

static U64 XXH_rotl64(U64 x, I32 r) { return (x << r) | (x >> (64 - r)); }

static U64 xxh3(const void *data, U32 len, U64 seed) {
    const uint8_t *p = (const uint8_t *)data;
    const uint8_t *const end = p + len;
    U64 h64;

    if (len >= 32) {
        const uint8_t *const limit = end - 32;
        U64 v1 = seed + XXH_PRIME_1 + XXH_PRIME_2;
        U64 v2 = seed + XXH_PRIME_2;
        U64 v3 = seed + 0;
        U64 v4 = seed - XXH_PRIME_1;

        do {
            v1 += XXH_read64(p) * XXH_PRIME_2;
            v1 = XXH_rotl64(v1, 31);
            v1 *= XXH_PRIME_1;

            v2 += XXH_read64(p + 8) * XXH_PRIME_2;
            v2 = XXH_rotl64(v2, 31);
            v2 *= XXH_PRIME_1;

            v3 += XXH_read64(p + 16) * XXH_PRIME_2;
            v3 = XXH_rotl64(v3, 31);
            v3 *= XXH_PRIME_1;

            v4 += XXH_read64(p + 24) * XXH_PRIME_2;
            v4 = XXH_rotl64(v4, 31);
            v4 *= XXH_PRIME_1;

            p += 32;
        } while (p <= limit);

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) +
              XXH_rotl64(v4, 18);

        v1 *= XXH_PRIME_2;
        v1 = XXH_rotl64(v1, 31);
        v1 *= XXH_PRIME_1;
        h64 ^= v1;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v2 *= XXH_PRIME_2;
        v2 = XXH_rotl64(v2, 31);
        v2 *= XXH_PRIME_1;
        h64 ^= v2;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v3 *= XXH_PRIME_2;
        v3 = XXH_rotl64(v3, 31);
        v3 *= XXH_PRIME_1;
        h64 ^= v3;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v4 *= XXH_PRIME_2;
        v4 = XXH_rotl64(v4, 31);
        v4 *= XXH_PRIME_1;
        h64 ^= v4;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;
    } else {
        h64 = seed + XXH_PRIME_5;
    }

    h64 += (U64)len;

    while (p + 8 <= end) {
        U64 k1 = XXH_read64(p);
        k1 *= XXH_PRIME_2;
        k1 = XXH_rotl64(k1, 31);
        k1 *= XXH_PRIME_1;
        h64 ^= k1;
        h64 = XXH_rotl64(h64, 27) * XXH_PRIME_1 + XXH_PRIME_4;
        p += 8;
    }

    if (p + 4 <= end) {
        h64 ^= (U64)(XXH_read32(p)) * XXH_PRIME_1;
        h64 = XXH_rotl64(h64, 23) * XXH_PRIME_2 + XXH_PRIME_3;
        p += 4;
    }

    while (p < end) {
        h64 ^= (*p) * XXH_PRIME_5;
        h64 = XXH_rotl64(h64, 11) * XXH_PRIME_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= XXH_PRIME_2;
    h64 ^= h64 >> 29;
    h64 *= XXH_PRIME_3;
    h64 ^= h64 >> 32;

    return h64;
}

// hashmap_sip returns a hash value for `data` using SipHash-2-4.
U64 hashmap_sip(const void *data, U32 len, U64 seed0, U64 seed1) {
    return SIP64((uint8_t *)data, len, seed0, seed1);
}

// hashmap_murmur returns a hash value for `data` using Murmur3_86_128.
U64 hashmap_murmur(const void *data, U32 len, U64 seed0, U64 seed1) {
    (void)seed1;
    return MM86128(data, len, seed0);
}

U64 hashmap_xxhash3(const void *data, U32 len, U64 seed0, U64 seed1) {
    (void)seed1;
    return xxh3(data, len, seed0);
}

//==============================================================================
// TESTS AND BENCHMARKS
// $ cc -DHASHMAP_TEST hashmap.c && ./a.out              # run tests
// $ cc -DHASHMAP_TEST -O3 hashmap.c && BENCH=1 ./a.out  # run benchmarks
//==============================================================================
#ifdef HASHMAP_TEST

static U32 deepcount(HM_Hashmap *map) {
    U32 count = 0;
    for (U32 i = 0; i < map->nbuckets; i++) {
        if (bucket_at(map, i)->dib) {
            count++;
        }
    }
    return count;
}

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wcompound-token-split-by-macro"
#pragma GCC diagnostic ignored "-Wgnu-statement-expression-from-macro-expansion"
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hashmap.h"

static B8 rand_alloc_fail = false;
static I32 rand_alloc_fail_odds = 3; // 1 in 3 chance malloc will fail.
static uintptr_t total_allocs = 0;
static uintptr_t total_mem = 0;

static void *xmalloc(U32 size) {
    if (rand_alloc_fail && rand() % rand_alloc_fail_odds == 0) {
        return NULL;
    }
    void *mem = malloc(sizeof(uintptr_t) + size);
    assert(mem);
    *(uintptr_t *)mem = size;
    total_allocs++;
    total_mem += size;
    return (char *)mem + sizeof(uintptr_t);
}

static void xfree(void *ptr) {
    if (ptr) {
        total_mem -= *(uintptr_t *)((char *)ptr - sizeof(uintptr_t));
        free((char *)ptr - sizeof(uintptr_t));
        total_allocs--;
    }
}

static void shuffle(void *array, U32 numels, U32 elsize) {
    char tmp[elsize];
    char *arr = array;
    for (U32 i = 0; i < numels - 1; i++) {
        I32 j = i + rand() / (RAND_MAX / (numels - i) + 1);
        memcpy(tmp, arr + j * elsize, elsize);
        memcpy(arr + j * elsize, arr + i * elsize, elsize);
        memcpy(arr + i * elsize, tmp, elsize);
    }
}

static B8 iter_ints(const void *item, void *udata) {
    I32 *vals = *(i32 **)udata;
    vals[*(I32 *)item] = 1;
    return true;
}

static I32 compare_ints_udata(const void *a, const void *b, void *udata) {
    return *(I32 *)a - *(i32 *)b;
}

static I32 compare_strs(const void *a, const void *b, void *udata) {
    return strcmp(*(char **)a, *(char **)b);
}

static U64 hash_int(const void *item, U64 seed0, U64 seed1) {
    return hashmap_xxhash3(item, sizeof(int), seed0, seed1);
    // return hashmap_sip(item, sizeof(int), seed0, seed1);
    // return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

static U64 hash_str(const void *item, U64 seed0, U64 seed1) {
    return hashmap_xxhash3(*(char **)item, strlen(*(char **)item), seed0,
                           seed1);
    // return hashmap_sip(*(char**)item, strlen(*(char**)item), seed0, seed1);
    // return hashmap_murmur(*(char**)item, strlen(*(char**)item), seed0,
    // seed1);
}

static void free_str(void *item) { xfree(*(char **)item); }

static void all(void) {
    I32 seed = getenv("SEED") ? atoi(getenv("SEED")) : time(NULL);
    I32 N = getenv("N") ? atoi(getenv("N")) : 2000;
    printf("seed=%d, count=%d, item_size=%zu\n", seed, N, sizeof(int));
    srand(seed);

    rand_alloc_fail = true;

    // test sip and murmur hashes
    assert(hashmap_sip("hello", 5, 1, 2) == 2957200328589801622);
    assert(hashmap_murmur("hello", 5, 1, 2) == 1682575153221130884);
    assert(hashmap_xxhash3("hello", 5, 1, 2) == 2584346877953614258);

    I32 *vals;
    while (!(vals = xmalloc(N * sizeof(int)))) {
    }
    for (I32 i = 0; i < N; i++) {
        vals[i] = i;
    }

    HM_Hashmap *map;

    while (!(map = hashmap_new(sizeof(int), 0, seed, seed, hash_int,
                               compare_ints_udata, NULL, NULL))) {
    }
    shuffle(vals, N, sizeof(int));
    for (I32 i = 0; i < N; i++) {
        // // printf("== %d ==\n", vals[i]);
        assert(map->count == (U32)i);
        assert(map->count == hashmap_count(map));
        assert(map->count == deepcount(map));
        const I32 *v;
        assert(!hashmap_get(map, &vals[i]));
        assert(!hashmap_delete(map, &vals[i]));
        while (true) {
            assert(!hashmap_set(map, &vals[i]));
            if (!hashmap_oom(map)) {
                break;
            }
        }

        for (I32 j = 0; j < i; j++) {
            v = hashmap_get(map, &vals[j]);
            assert(v && *v == vals[j]);
        }
        while (true) {
            v = hashmap_set(map, &vals[i]);
            if (!v) {
                assert(hashmap_oom(map));
                continue;
            } else {
                assert(!hashmap_oom(map));
                assert(v && *v == vals[i]);
                break;
            }
        }
        v = hashmap_get(map, &vals[i]);
        assert(v && *v == vals[i]);
        v = hashmap_delete(map, &vals[i]);
        assert(v && *v == vals[i]);
        assert(!hashmap_get(map, &vals[i]));
        assert(!hashmap_delete(map, &vals[i]));
        assert(!hashmap_set(map, &vals[i]));
        assert(map->count == (U32)(i + 1));
        assert(map->count == hashmap_count(map));
        assert(map->count == deepcount(map));
    }

    I32 *vals2;
    while (!(vals2 = xmalloc(N * sizeof(int)))) {
    }
    memset(vals2, 0, N * sizeof(int));
    assert(hashmap_scan(map, iter_ints, &vals2));

    // Test hashmap_iter. This does the same as hashmap_scan above.
    U32 iter = 0;
    void *iter_val;
    while (hashmap_iter(map, &iter, &iter_val)) {
        assert(iter_ints(iter_val, &vals2));
    }
    for (I32 i = 0; i < N; i++) {
        assert(vals2[i] == 1);
    }
    xfree(vals2);

    shuffle(vals, N, sizeof(int));
    for (I32 i = 0; i < N; i++) {
        const I32 *v;
        v = hashmap_delete(map, &vals[i]);
        assert(v && *v == vals[i]);
        assert(!hashmap_get(map, &vals[i]));
        assert(map->count == (U32)(N - i - 1));
        assert(map->count == hashmap_count(map));
        assert(map->count == deepcount(map));
        for (I32 j = N - 1; j > i; j--) {
            v = hashmap_get(map, &vals[j]);
            assert(v && *v == vals[j]);
        }
    }

    for (I32 i = 0; i < N; i++) {
        while (true) {
            assert(!hashmap_set(map, &vals[i]));
            if (!hashmap_oom(map)) {
                break;
            }
        }
    }

    assert(map->count != 0);
    U32 prev_cap = map->cap;
    hashmap_clear(map, true);
    assert(prev_cap < map->cap);
    assert(map->count == 0);

    for (I32 i = 0; i < N; i++) {
        while (true) {
            assert(!hashmap_set(map, &vals[i]));
            if (!hashmap_oom(map)) {
                break;
            }
        }
    }

    prev_cap = map->cap;
    hashmap_clear(map, false);
    assert(prev_cap == map->cap);

    hashmap_free(map);

    xfree(vals);

    while (!(map = hashmap_new(sizeof(char *), 0, seed, seed, hash_str,
                               compare_strs, free_str, NULL)))
        ;

    for (I32 i = 0; i < N; i++) {
        char *str;
        while (!(str = xmalloc(16)))
            ;
        snprintf(str, 16, "s%i", i);
        while (!hashmap_set(map, &str))
            ;
    }

    hashmap_clear(map, false);
    assert(hashmap_count(map) == 0);

    for (I32 i = 0; i < N; i++) {
        char *str;
        while (!(str = xmalloc(16)))
            ;
        snprintf(str, 16, "s%i", i);
        while (!hashmap_set(map, &str))
            ;
    }

    hashmap_free(map);

    if (total_allocs != 0) {
        fprintf(stderr, "total_allocs: expected 0, got %lu\n", total_allocs);
        exit(1);
    }
}

#define bench(name, N, code)                                                   \
    {                                                                          \
        {                                                                      \
            if (strlen(name) > 0) {                                            \
                printf("%-14s ", name);                                        \
            }                                                                  \
            U32 tmem = total_mem;                                              \
            U32 tallocs = total_allocs;                                        \
            U64 bytes = 0;                                                     \
            clock_t begin = clock();                                           \
            for (I32 i = 0; i < N; i++) {                                      \
                (code);                                                        \
            }                                                                  \
            clock_t end = clock();                                             \
            F64 elapsed_secs = (F64)(end - begin) / CLOCKS_PER_SEC;            \
            F64 bytes_sec = (F64)bytes / elapsed_secs;                         \
            printf("%d ops in %.3f secs, %.0f ns/op, %.0f op/sec", N,          \
                   elapsed_secs, elapsed_secs / (F64)N * 1e9,                  \
                   (F64)N / elapsed_secs);                                     \
            if (bytes > 0) {                                                   \
                printf(", %.1f GB/sec", bytes_sec / 1024 / 1024 / 1024);       \
            }                                                                  \
            if (total_mem > tmem) {                                            \
                U32 used_mem = total_mem - tmem;                               \
                printf(", %.2f bytes/op", (F64)used_mem / N);                  \
            }                                                                  \
            if (total_allocs > tallocs) {                                      \
                U32 used_allocs = total_allocs - tallocs;                      \
                printf(", %.2f allocs/op", (F64)used_allocs / N);              \
            }                                                                  \
            printf("\n");                                                      \
        }                                                                      \
    }

static void benchmarks(void) {
    I32 seed = getenv("SEED") ? atoi(getenv("SEED")) : time(NULL);
    I32 N = getenv("N") ? atoi(getenv("N")) : 5000000;
    printf("seed=%d, count=%d, item_size=%zu\n", seed, N, sizeof(int));
    srand(seed);

    I32 *vals = xmalloc(N * sizeof(int));
    for (I32 i = 0; i < N; i++) {
        vals[i] = i;
    }

    shuffle(vals, N, sizeof(int));

    HM_Hashmap *map;
    shuffle(vals, N, sizeof(int));

    map = hashmap_new(sizeof(int), 0, seed, seed, hash_int, compare_ints_udata,
                      NULL, NULL);
    bench("set", N, {
        const I32 *v = hashmap_set(map, &vals[i]);
        assert(!v);
    }) shuffle(vals, N, sizeof(int));
    bench("get", N, {
        const I32 *v = hashmap_get(map, &vals[i]);
        assert(v && *v == vals[i]);
    }) shuffle(vals, N, sizeof(int));
    bench("delete", N, {
        const I32 *v = hashmap_delete(map, &vals[i]);
        assert(v && *v == vals[i]);
    }) hashmap_free(map);

    map = hashmap_new(sizeof(int), N, seed, seed, hash_int, compare_ints_udata,
                      NULL, NULL);
    bench("set (cap)", N, {
        const I32 *v = hashmap_set(map, &vals[i]);
        assert(!v);
    }) shuffle(vals, N, sizeof(int));
    bench("get (cap)", N, {
        const I32 *v = hashmap_get(map, &vals[i]);
        assert(v && *v == vals[i]);
    }) shuffle(vals, N, sizeof(int));
    bench("delete (cap)", N,
          {
              const I32 *v = hashmap_delete(map, &vals[i]);
              assert(v && *v == vals[i]);
          })

        hashmap_free(map);

    xfree(vals);

    if (total_allocs != 0) {
        fprintf(stderr, "total_allocs: expected 0, got %lu\n", total_allocs);
        exit(1);
    }
}

I32 main(void) {
    hashmap_set_allocator(xmalloc, xfree);

    if (getenv("BENCH")) {
        printf("Running hashmap.c benchmarks...\n");
        benchmarks();
    } else {
        printf("Running hashmap.c tests...\n");
        all();
        printf("PASSED\n");
    }
}

#endif

