/****************************************************************************
 * Copyright (c) 2012, Masahiro Ide <ide@konohascript.org>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#define KMAP_INITSIZE DICTMAP_THRESHOLD
#define DELTA 16

#ifndef unlikely
#define unlikely(x)   __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x)     __builtin_expect(!!(x), 1)
#endif

static inline uint32_t djbhash(const char *p, uint32_t len)
{
    uint32_t hash = 5381;
    const uint8_t *s = (const uint8_t *) p;
    const uint8_t *e = (const uint8_t *) p + len;
    while (s < e) {
        hash = ((hash << 5) + hash) + *s++;
    }
    return hash;
}

static inline uint32_t one_at_a_time(const char *p, uint32_t len)
{
    uint32_t i, hash = 0;

    for (i = 0; i < len; ++i) {
        hash += p[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

static inline uint32_t fnv1a(const char *p, uint32_t len)
{
    uint32_t hash = 0x811C9DC5;
    const uint8_t *s = (const uint8_t *) p;
    const uint8_t *e = (const uint8_t *) p + len;
    while (s < e) {
        hash = (*s++ ^ hash) * 0x01000193;
    }
    return hash;
}


static unsigned JSONString_hashCode(JSONString *key)
{

    if (!key->hashcode)
        key->hashcode =
#define USE_DJBHASH
#ifdef USE_DJBHASH
            djbhash(key->str, key->length);
#elif defined(USE_ONE_AT_A_TIME)
            one_at_a_time(key->str, key->length);
#else
            fnv1a(key->str, key->length);
#endif
    return key->hashcode;
}

static int JSONString_equal(JSONString *k0, JSONString *k1)
{
    if (k0->length != k1->length)
        return 0;
    if (JSONString_hashCode(k0) != JSONString_hashCode(k1))
        return 0;
    if (k0->str[0] != k1->str[0])
        return 0;
    return strncmp(k0->str, k1->str, k0->length) == 0;
}

static void map_record_copy(map_record_t *dst, const map_record_t *src)
{
    *dst = *src;
    // which one is faster??
    //memcpy(dst, src, sizeof(map_record_t));
}

/* [HASHMAP] */
static inline map_record_t *hashmap_at(hashmap_t *m, unsigned idx)
{
    assert(idx < (m->record_size_mask+1));
    return m->base.records+idx;
}

static void hashmap_record_reset(hashmap_t *m, size_t newsize)
{
    unsigned alloc_size = sizeof(map_record_t) * newsize;
    m->used_size = 0;
    (m->record_size_mask) = newsize - 1;
    m->base.records = (map_record_t *) calloc(1, alloc_size);
}

static map_status_t hashmap_set_no_resize(hashmap_t *m, map_record_t *rec)
{
    unsigned i, idx = rec->hash & m->record_size_mask;
    for (i = 0; i < DELTA; ++i) {
        map_record_t *r = m->base.records+idx;
        if (r->hash == 0) {
            map_record_copy(r, rec);
            ++m->used_size;
            return KMAP_ADDED;
        }
        if (r->hash == rec->hash && JSONString_equal(r->k, rec->k)) {
            JSON_free(toJSON(ValueP(r->v)));
            map_record_copy(r, rec);
            return KMAP_UPDATE;
        }
        idx = (idx + 1) & m->record_size_mask;
    }
    return KMAP_FAILED;
}

static void hashmap_record_resize(hashmap_t *m, unsigned newsize)
{
    unsigned oldsize = (m->record_size_mask+1);
    map_record_t *head = m->base.records;

    do {
        unsigned i;
        newsize *= 2;
        hashmap_record_reset(m, newsize);
        for (i = 0; i < oldsize; ++i) {
            map_record_t *r = head + i;
            if (r->hash && hashmap_set_no_resize(m, r) == KMAP_FAILED)
                continue;
        }
    } while (0);
    free(head/*, oldsize*sizeof(map_record_t)*/);
}

static map_status_t hashmap_set(hashmap_t *m, map_record_t *rec)
{
    map_status_t res;
    do {
        if ((res = hashmap_set_no_resize(m, rec)) != KMAP_FAILED)
            return res;
        hashmap_record_resize(m, (m->record_size_mask+1)*2);
    } while (1);
    /* unreachable */
    return KMAP_FAILED;
}

static map_record_t *hashmap_get(hashmap_t *m, unsigned hash, JSONString *key)
{
    unsigned i, idx = hash & m->record_size_mask;
    for (i = 0; i < DELTA; ++i) {
        map_record_t *r = m->base.records+idx;
        if (r->hash == hash && JSONString_equal(r->k, key)) {
            return r;
        }
        idx = (idx + 1) & m->record_size_mask;
    }
    return NULL;
}

static void hashmap_init(hashmap_t *m, unsigned init)
{
    if (init < KMAP_INITSIZE)
        init = KMAP_INITSIZE;
    hashmap_record_reset(m, 1U << LOG2(init));
}

static void hashmap_api_init(kmap_t *m, unsigned init)
{
    hashmap_init((hashmap_t *) m, init);
}

static void hashmap_api_dispose(kmap_t *_m)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned i, size = (m->record_size_mask+1);
    for (i = 0; i < size; ++i) {
        map_record_t *r = hashmap_at(m, i);
        if (r->hash) {
            JSON_free(toJSON(ValueS(r->k)));
        }
    }
    free(m->base.records/*, (m->record_size_mask+1) * sizeof(map_record_t)*/);
}

static map_record_t *hashmap_api_get(kmap_t *_m, JSONString *key)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned hash = JSONString_hashCode(key);
    map_record_t *r = hashmap_get(m, hash, key);
    return r;
}

static map_status_t hashmap_api_set(kmap_t *_m, JSONString *key, uint64_t val)
{
    hashmap_t *m = (hashmap_t *) _m;
    map_record_t r;
    r.hash = JSONString_hashCode(key);
    r.k    = key;
    r.v    = val;
    return hashmap_set(m, &r);
}

static void hashmap_api_remove(kmap_t *_m, JSONString *key)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned hash = JSONString_hashCode(key);
    map_record_t *r = hashmap_get(m, hash, key);
    if (r) {
        r->hash = 0; r->k = NULL;
        m->used_size -= 1;
    }
}

static map_record_t *hashmap_api_next(kmap_t *_m, kmap_iterator *itr)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned i, size = (m->record_size_mask+1);
    for (i = itr->index; i < size; ++i) {
        map_record_t *r = hashmap_at(m, i);
        if (r->hash) {
            itr->index = i+1;
            return r;
        }
    }
    itr->index = i;
    assert(itr->index == size);
    return NULL;
}

const kmap_api_t HASH = {
    hashmap_api_get,
    hashmap_api_set,
    hashmap_api_next,
    hashmap_api_remove,
    hashmap_api_init,
    hashmap_api_dispose
};

/* [DICTMAP] */
static kmap_t *dictmap_init(dictmap_t *m)
{
    int i;
    const size_t allocSize = sizeof(map_record_t)*DICTMAP_THRESHOLD;
    m->base.records = (map_record_t *) malloc(allocSize);
    m->used_size = 0;
    for (i = 0; i < DICTMAP_THRESHOLD; ++i) {
        m->hash_list[i] = 0;
    }
    return (kmap_t *) m;
}

static void dictmap_api_init(kmap_t *_m, unsigned init)
{
    dictmap_init((dictmap_t *) _m);
}

static void dictmap_record_copy(map_record_t *dst, const map_record_t *src)
{
    memcpy(dst, src, sizeof(map_record_t));
}

static inline map_record_t *dictmap_at(dictmap_t *m, unsigned idx)
{
    return (map_record_t *)(m->base.records+idx);
}

static map_status_t dictmap_set_new(dictmap_t *m, map_record_t *rec, int i)
{
    map_record_t *r = dictmap_at(m, i);
    m->hash_list[i] = rec->hash;
    dictmap_record_copy(r, rec);
    ++m->used_size;
    return KMAP_ADDED;
}

static void dictmap_convert2hashmap(dictmap_t *_m);

static map_status_t dictmap_set(dictmap_t *m, map_record_t *rec)
{
    int i;
    for (i = 0; i < DICTMAP_THRESHOLD; ++i) {
        unsigned hash = m->hash_list[i];
        if (hash == 0) {
            return dictmap_set_new(m, rec, i);
        }
        else if (hash == rec->hash) {
            map_record_t *r = dictmap_at(m, i);
            if (!unlikely(JSONString_equal(r->k, rec->k))) {
                continue;
            }
            JSON_free(toJSON(ValueP(r->v)));
            dictmap_record_copy(r, rec);
            return KMAP_UPDATE;
        }
    }
    dictmap_convert2hashmap(m);
    return hashmap_set((hashmap_t *) m, rec);
}

static map_record_t *dictmap_get(dictmap_t *m, unsigned hash, JSONString *key)
{
    int i;
    for (i = 0; i < DICTMAP_THRESHOLD; ++i) {
        if (hash == m->hash_list[i]) {
            map_record_t *r = dictmap_at(m, i);
            if (JSONString_equal(r->k, key)) {
                return r;
            }
        }
    }
    return NULL;
}

static map_status_t dictmap_api_set(kmap_t *_m, JSONString *key, uint64_t val)
{
    dictmap_t *m = (dictmap_t *)_m;
    map_record_t r;
    r.hash = JSONString_hashCode(key);
    r.k  = key;
    r.v  = val;
    return dictmap_set(m, &r);
}

static void dictmap_api_remove(kmap_t *_m, JSONString *key)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned hash = JSONString_hashCode(key);
    map_record_t *r = dictmap_get(m, hash, key);
    if (r) {
        r->hash = 0; r->k = 0;
        m->used_size -= 1;
    }
}

static map_record_t *dictmap_api_next(kmap_t *_m, kmap_iterator *itr)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned i;
    for (i = itr->index; i < m->used_size; ++i) {
        map_record_t *r = dictmap_at(m, i);
        itr->index = i+1;
        return r;
    }
    itr->index = m->used_size;
    return NULL;
}

static map_record_t *dictmap_api_get(kmap_t *_m, JSONString *key)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned hash = JSONString_hashCode(key);
    map_record_t *r = dictmap_get(m, hash, key);
    return r;
}

static void dictmap_api_dispose(kmap_t *_m)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned i;
    for (i = 0; i < m->used_size; ++i) {
        if (likely(m->hash_list[i])) {
            map_record_t *r = dictmap_at(m, i);
            _JSONString_free(r->k);
            JSON_free(toJSON(ValueP(r->v)));
        }
    }
    free(m->base.records/*, m->used_size * sizeof(map_record_t)*/);
}

const kmap_api_t DICT = {
    dictmap_api_get,
    dictmap_api_set,
    dictmap_api_next,
    dictmap_api_remove,
    dictmap_api_init,
    dictmap_api_dispose
};

static void dictmap_convert2hashmap(dictmap_t *_m)
{
    hashmap_t *m = (hashmap_t *) _m;
    m->base.api = &HASH;
    m->record_size_mask = DICTMAP_THRESHOLD-1;
    hashmap_record_resize(m, DELTA);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
