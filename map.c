#include "kjson.h"
#include "map.h"
#include "internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POOLMAP_INITSIZE DICTMAP_THRESHOLD
#define DELTA 8

#define _MALLOC(SIZE)    malloc(SIZE)
#define _FREE(PTR, SIZE) free(PTR)

static inline poolmap_t *poolmap_create(const poolmap_api_t *api)
{
	poolmap_t *m = cast(poolmap_t *, _MALLOC(sizeof(*m)));
	m->h.base.op = api;
	return m;
}

static void map_record_copy(map_record_t *dst, const map_record_t *src)
{
    memcpy(dst, src, sizeof(map_record_t));
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
    m->base.records = cast(map_record_t *, calloc(1, alloc_size));
}

static map_status_t hashmap_set_no_resize(hashmap_t *m, map_record_t *rec)
{
    unsigned i, idx = rec->hash & m->record_size_mask;
    for (i = 0; i < DELTA; ++i) {
        map_record_t *r = m->base.records+idx;
        if (r->hash == 0) {
            map_record_copy(r, rec);
            ++m->used_size;
            return POOLMAP_ADDED;
        }
        if (r->hash == rec->hash && m->base.entry_api->fcmp(r->k, rec->k)) {
            uintptr_t old0 = r->v;
            unsigned  old1 = r->v2;
            m->base.entry_api->ffree(r);
            map_record_copy(r, rec);
            rec->v  = old0;
            rec->v2 = old1;
            return POOLMAP_UPDATE;
        }
        idx = (idx + 1) & m->record_size_mask;
    }
    return POOLMAP_FAILED;
}

static void hashmap_record_resize(hashmap_t *m)
{
    unsigned oldsize = (m->record_size_mask+1);
    unsigned newsize = oldsize;
    map_record_t *head = m->base.records;

    do {
        unsigned i;
        newsize *= 2;
        hashmap_record_reset(m, newsize);
        for (i = 0; i < oldsize; ++i) {
            map_record_t *r = head + i;
            if (r->hash && hashmap_set_no_resize(m, r) == POOLMAP_FAILED)
                continue;
        }
    } while (0);
    _FREE(head, oldsize*sizeof(map_record_t));
}

static map_status_t hashmap_set(hashmap_t *m, map_record_t *rec)
{
    map_status_t res;
    do {
        if ((res = hashmap_set_no_resize(m, rec)) != POOLMAP_FAILED)
            return res;
        hashmap_record_resize(m);
    } while (1);
}

static map_record_t *hashmap_get(hashmap_t *m, unsigned hash, uintptr_t key)
{
    unsigned i, idx = hash & m->record_size_mask;
    for (i = 0; i < DELTA; ++i) {
        map_record_t *r = m->base.records+idx;
        if (r->hash == hash && m->base.entry_api->fcmp(r->k, key)) {
            return r;
        }
        idx = (idx + 1) & m->record_size_mask;
    }
    return NULL;
}

static void hashmap_init(hashmap_t *m, unsigned init, const struct poolmap_entry_api *entry_api)
{
    if (init < POOLMAP_INITSIZE)
        init = POOLMAP_INITSIZE;
    hashmap_record_reset(m, 1U << (SizeToKlass(init)));
    m->base.entry_api = entry_api;
}

static void hashmap_api_init(poolmap_t *m, unsigned init, const struct poolmap_entry_api *entry_api)
{
    hashmap_init((hashmap_t *) m, init, entry_api);
}

static void hashmap_api_dispose(poolmap_t *_m)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned i, size = (m->record_size_mask+1);
    for (i = 0; i < size; ++i) {
        map_record_t *r = hashmap_at(m, i);
        if (r->hash) {
            JSON_free((JSON)r->k);
            m->base.entry_api->ffree(r);
        }
    }
    _FREE(m->base.records, (m->record_size_mask+1) * sizeof(map_record_t));
}

static map_record_t *hashmap_api_get(poolmap_t *_m, char *key, unsigned klen)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned hash = m->base.entry_api->fkey0(key, klen);
    map_record_t *r = hashmap_get(m, hash, m->base.entry_api->fkey1(key, klen));
    return r;
}

static map_status_t hashmap_api_set(poolmap_t *_m, char *key, unsigned klen, void *val)
{
    hashmap_t *m = (hashmap_t *) _m;
    map_record_t r;
    r.hash = m->base.entry_api->fkey0(key, klen);
    r.k    = m->base.entry_api->fkey1(key, klen);
    r.v    = cast(uintptr_t, val);
    r.v2   = 0;
    return hashmap_set(m, &r);
}

static void hashmap_api_remove(poolmap_t *_m, char *key, unsigned klen)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned hash = m->base.entry_api->fkey0(key, klen);
    map_record_t *r = hashmap_get(m, hash, m->base.entry_api->fkey1(key, klen));
    if (r) {
        r->hash = 0;
        r->k = 0;
        m->used_size -= 1;
    }
}

static map_record_t *hashmap_api_next(poolmap_t *_m, poolmap_iterator *itr)
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

static const poolmap_api_t HASH = {
    hashmap_api_get,
    hashmap_api_set,
    hashmap_api_next,
    hashmap_api_remove,
    hashmap_api_init,
    hashmap_api_dispose
};

static poolmap_t *hashmap_new(unsigned init, const struct poolmap_entry_api *entry_api)
{
    poolmap_t *m = poolmap_create(&HASH);
    hashmap_init((hashmap_t *) m, init, entry_api);
    return m;
}

/* [DICTMAP] */
static poolmap_t *dictmap_init(dictmap_t *m, const poolmap_entry_api_t *entry_api)
{
    int i;
    const size_t allocSize = sizeof(map_record_t)*DICTMAP_THRESHOLD;
    m->base.entry_api = entry_api;
    m->base.records = cast(map_record_t *, _MALLOC(allocSize));
    uint64_t *hash_list = (uint64_t *) m->hash_list;
    m->used_size = 0;
    for (i = 0; i < DICTMAP_THRESHOLD/2; ++i) {
        hash_list[i] = 0;
    }
    return (poolmap_t *) m;
}

static void dictmap_api_init(poolmap_t *_m, unsigned init, const poolmap_entry_api_t *entry_api)
{
    dictmap_init((dictmap_t *) _m, entry_api);
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
    return POOLMAP_ADDED;
}

static void dictmap_convert2hashmap(dictmap_t *_m);
static map_status_t dictmap_set(dictmap_t *m, map_record_t *rec)
{
    int i;
    const poolmap_entry_api_t *entry_api = m->base.entry_api;
    for (i = 0; i < DICTMAP_THRESHOLD; ++i) {
        unsigned hash = m->hash_list[i];
        if (hash == 0) {
            return dictmap_set_new(m, rec, i);
        }
        else if (hash == rec->hash) {
            map_record_t *r = dictmap_at(m, i);
            if (!unlikely(entry_api->fcmp(r->k, rec->k))) {
                continue;
            }
            uintptr_t old0 = r->v;
            unsigned  old1 = r->v2;
            entry_api->ffree(r);
            dictmap_record_copy(r, rec);
            rec->v  = old0;
            rec->v2 = old1;
            return POOLMAP_UPDATE;
        }
    }
    dictmap_convert2hashmap(m);
    return hashmap_set((hashmap_t *) m, rec);
}

static map_record_t *dictmap_get(dictmap_t *m, unsigned hash, uintptr_t key)
{
    int i;
    fn_keycmp fcmp = m->base.entry_api->fcmp;
    for (i = 0; i < DICTMAP_THRESHOLD; ++i) {
        if (hash == m->hash_list[i]) {
            map_record_t *r = dictmap_at(m, i);
            if (fcmp(r->k, key)) {
                return r;
            }
        }
    }
    return NULL;
}

static map_status_t dictmap_api_set(poolmap_t *_m, char *key, unsigned klen, void *val)
{
    dictmap_t *m = (dictmap_t *)_m;
    map_record_t r;
    r.hash = m->base.entry_api->fkey0(key, klen);
    r.k  = m->base.entry_api->fkey1(key, klen);
    r.v  = cast(uintptr_t, val);
    r.v2 = 0;
    return dictmap_set(m, &r);
}

static void dictmap_api_remove(poolmap_t *_m, char *key, unsigned klen)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned hash = m->base.entry_api->fkey0(key, klen);
    map_record_t *r = dictmap_get(m, hash, m->base.entry_api->fkey1(key, klen));
    if (r) {
        r->hash = 0;
        r->k = 0;
        m->used_size -= 1;
    }
}

static map_record_t *dictmap_api_next(poolmap_t *_m, poolmap_iterator *itr)
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

static map_record_t *dictmap_api_get(poolmap_t *_m, char *key, unsigned klen)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned hash = m->base.entry_api->fkey0(key, klen);
    map_record_t *r = dictmap_get(m, hash, m->base.entry_api->fkey1(key, klen));
    return r;
}

static void dictmap_api_dispose(poolmap_t *_m)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned i;
    for (i = 0; i < m->used_size; ++i) {
        if (likely(m->hash_list[i])) {
            map_record_t *r = dictmap_at(m, i);
            JSON_free((JSON)r->k);
            m->base.entry_api->ffree(r);
        }
    }
    _FREE(m->base.records, m->used_size * sizeof(map_record_t));
}

static const poolmap_api_t DICT = {
    dictmap_api_get,
    dictmap_api_set,
    dictmap_api_next,
    dictmap_api_remove,
    dictmap_api_init,
    dictmap_api_dispose
};

static poolmap_t *dictmap_new(const poolmap_entry_api_t *entry_api)
{
    poolmap_t *m = poolmap_create(&DICT);
    return dictmap_init((dictmap_t *) m, entry_api);
}

/* [POOLMAP] */
poolmap_t *poolmap_new(unsigned init, const poolmap_entry_api_t *entry_api)
{
    poolmap_t *m = (init <= DICTMAP_THRESHOLD) ?
        dictmap_new(entry_api) : hashmap_new(init, entry_api);
    return m;
}

void poolmap_init(poolmap_t *m, unsigned init, const poolmap_entry_api_t *entry_api)
{
    const poolmap_api_t *op = (init > DICTMAP_THRESHOLD) ? &HASH:&DICT;
    m->h.base.op = op;
    op->_init(m, init, entry_api);
}

void poolmap_delete(poolmap_t *m)
{
    poolmap_dispose(m);
    free(m);
}

static void dictmap_convert2hashmap(dictmap_t *_m)
{
    hashmap_t *m = (hashmap_t *) _m;
    m->base.op = &HASH;
    m->record_size_mask = DICTMAP_THRESHOLD-1;
    hashmap_record_resize(m);
}

#ifdef __cplusplus
}
#endif
