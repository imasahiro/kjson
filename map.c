#include "kjson.h"
#include "hash.h"
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

static void pmap_record_copy(pmap_record_t *dst, const pmap_record_t *src)
{
    memcpy(dst, src, sizeof(pmap_record_t));
}

static inline pmap_record_t *hmap_at(hashmap_t *m, unsigned idx)
{
    assert(idx < (m->record_size_mask+1));
    return m->records+idx;
}

static void hmap_record_reset(hashmap_t *m, size_t newsize)
{
    unsigned alloc_size = sizeof(pmap_record_t) * newsize;
    m->used_size = 0;
    (m->record_size_mask) = newsize - 1;
    m->records = cast(pmap_record_t *, calloc(1, alloc_size));
}

static pmap_status_t _hmap_set_no_resize(hashmap_t *m, pmap_record_t *rec)
{
    unsigned i = 0, idx = rec->hash & m->record_size_mask;
    pmap_record_t *r;
    do {
        r = m->records+idx;
        if (r->hash == 0) {
            pmap_record_copy(r, rec);
            ++m->used_size;
            return POOLMAP_ADDED;
        }
        if (r->hash == rec->hash && m->api->fcmp(r->k, rec->k)) {
            uintptr_t old0 = r->v;
            unsigned  old1 = r->v2;
            m->api->ffree(r);
            pmap_record_copy(r, rec);
            rec->v  = old0;
            rec->v2 = old1;
            return POOLMAP_UPDATE;
        }
        idx = (idx + 1) & m->record_size_mask;
    } while (i++ < m->used_size);
    assert(0);
}

static void hmap_record_resize(hashmap_t *m)
{
    pmap_record_t *old = m->records;
    unsigned i, oldsize = (m->record_size_mask+1);

    hmap_record_reset(m, oldsize*2);
    for (i = 0; i < oldsize; ++i) {
        pmap_record_t *r = old + i;
        if (r->hash) {
            _hmap_set_no_resize(m, r);
        }
    }
    free(old/*, oldsize*sizeof(pmap_record_t)*/);
}

static pmap_status_t _hmap_set(hashmap_t *m, pmap_record_t *rec)
{
    if (m->used_size > (m->record_size_mask+1) * 3 / 4) {
        hmap_record_resize(m);
    }
    return _hmap_set_no_resize(m, rec);
}

static pmap_record_t *_hmap_get(hashmap_t *m, unsigned hash, uintptr_t key)
{
    unsigned i = 0;
    unsigned idx = hash & m->record_size_mask;
    do {
        pmap_record_t *r = hmap_at(m, idx);
        if (r->hash == hash && m->api->fcmp(r->k, key)) {
            return r;
        }
        idx = (idx + 1) & m->record_size_mask;
    } while (i++ < m->used_size);
    return NULL;
}

static void _hashmap_init(hashmap_t *m, unsigned init, const struct poolmap_entry_api *api)
{
    if (init < POOLMAP_INITSIZE)
        init = POOLMAP_INITSIZE;
    hmap_record_reset(m, 1U << (SizeToKlass(init)));
    m->api = api;
}

static poolmap_t *hashmap_new(unsigned init, const struct poolmap_entry_api *api)
{
    poolmap_t *m = cast(poolmap_t *, malloc(sizeof(*m)));
    _hashmap_init((hashmap_t *) m, init, api);
    return m;
}

static void hashmap_init(poolmap_t *m, unsigned init, const struct poolmap_entry_api *api)
{
    _hashmap_init((hashmap_t *) m, init, api);
}

static void hashmap_dispose(poolmap_t *_m)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned i, size = (m->record_size_mask+1);
    for (i = 0; i < size; ++i) {
        pmap_record_t *r = hmap_at(m, i);
        if (r->hash) {
            JSON_free((JSON)r->k);
            m->api->ffree(r);
        }
    }
    free(m->records/*, (m->record_size_mask+1) * sizeof(pmap_record_t)*/);
}

static pmap_record_t *hashmap_get(poolmap_t *_m, char *key, unsigned klen)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned hash = m->api->fkey0(key, klen);
    pmap_record_t *r = _hmap_get(m, hash, m->api->fkey1(key, klen));
    return r;
}

static pmap_status_t hashmap_set(poolmap_t *_m, char *key, unsigned klen, void *val)
{
    hashmap_t *m = (hashmap_t *) _m;
    pmap_record_t r;
    r.hash = m->api->fkey0(key, klen);
    r.k  = m->api->fkey1(key, klen);
    r.v  = cast(uintptr_t, val);
    r.v2 = 0;
    return _hmap_set(m, &r);
}

static void hashmap_remove(poolmap_t *_m, char *key, unsigned klen)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned hash = m->api->fkey0(key, klen);
    pmap_record_t *r = _hmap_get(m, hash, m->api->fkey1(key, klen));
    if (r) {
        r->hash = 0;
        r->k = 0;
        m->used_size -= 1;
    }
}

static pmap_record_t *hashmap_next(poolmap_t *_m, poolmap_iterator *itr)
{
    hashmap_t *m = (hashmap_t *) _m;
    unsigned i, size = (m->record_size_mask+1);
    for (i = itr->index; i < size; ++i) {
        pmap_record_t *r = hmap_at(m, i);
        if (r->hash) {
            itr->index = i+1;
            return r;
        }
    }
    itr->index = i;
    assert(itr->index == size);
    return NULL;
}

/* [DICTMAP] */
static poolmap_t *_dictmap_init(dictmap_t *m, const poolmap_entry_api_t *api)
{
    int i;
    const size_t allocSize = sizeof(pmap_record_t)*DICTMAP_THRESHOLD;
    m->api = api;
    m->records = cast(pmap_record_t *,malloc(allocSize));
    uint64_t *hash_list = (uint64_t *) m->hash_list;
    m->used_size = 0;
    for (i = 0; i < DICTMAP_THRESHOLD/2; ++i) {
        hash_list[i] = 0;
    }
    return (poolmap_t *) m;
}

static void dictmap_init(poolmap_t *_m, unsigned init, const poolmap_entry_api_t *api)
{
    _dictmap_init((dictmap_t *) _m, api);
}

static poolmap_t *dictmap_new(const poolmap_entry_api_t *api)
{
    poolmap_t *_m = cast(poolmap_t *, malloc(sizeof(*_m)));
    return _dictmap_init((dictmap_t *) _m, api);
}

static void dmap_record_copy(pmap_record_t *dst, const pmap_record_t *src)
{
    memcpy(dst, src, sizeof(pmap_record_t));
}

static inline pmap_record_t *dmap_at(dictmap_t *m, unsigned idx)
{
    return (pmap_record_t *)(m->records+idx);
}

static pmap_status_t _dmap_set_new(dictmap_t *m, pmap_record_t *rec, int i)
{
    pmap_record_t *r = dmap_at(m, i);
    m->hash_list[i] = rec->hash;
    dmap_record_copy(r, rec);
    ++m->used_size;
    return POOLMAP_ADDED;
}

static void dictmap_convert2hashmap(dictmap_t *_m);
static pmap_status_t _dmap_set(dictmap_t *m, pmap_record_t *rec)
{
    int i;
    const poolmap_entry_api_t *api = m->api;
    for (i = 0; i < DICTMAP_THRESHOLD; ++i) {
        unsigned hash = m->hash_list[i];
        if (hash == 0) {
            return _dmap_set_new(m, rec, i);
        }
        else if (hash == rec->hash) {
            pmap_record_t *r = dmap_at(m, i);
            if (!unlikely(api->fcmp(r->k, rec->k))) {
                continue;
            }
            uintptr_t old0 = r->v;
            unsigned  old1 = r->v2;
            api->ffree(r);
            dmap_record_copy(r, rec);
            rec->v  = old0;
            rec->v2 = old1;
            return POOLMAP_UPDATE;
        }
    }
    dictmap_convert2hashmap(m);
    return _hmap_set((hashmap_t *) m, rec);
}

static pmap_record_t *_dmap_get(dictmap_t *m, unsigned hash, uintptr_t key)
{
    int i;
    fn_keycmp fcmp = m->api->fcmp;
    for (i = 0; i < DICTMAP_THRESHOLD; ++i) {
        if (hash == m->hash_list[i]) {
            pmap_record_t *r = dmap_at(m, i);
            if (fcmp(r->k, key)) {
                return r;
            }
        }
    }
    return NULL;
}

static pmap_status_t dictmap_set(poolmap_t *_m, char *key, unsigned klen, void *val)
{
    dictmap_t *m = (dictmap_t *)_m;
    pmap_record_t r;
    r.hash = m->api->fkey0(key, klen);
    r.k  = m->api->fkey1(key, klen);
    r.v  = cast(uintptr_t, val);
    r.v2 = 0;
    return _dmap_set(m, &r);
}

static void dictmap_remove(poolmap_t *_m, char *key, unsigned klen)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned hash = m->api->fkey0(key, klen);
    pmap_record_t *r = _dmap_get(m, hash, m->api->fkey1(key, klen));
    if (r) {
        r->hash = 0;
        r->k = 0;
        m->used_size -= 1;
    }
}

static pmap_record_t *dictmap_next(poolmap_t *_m, poolmap_iterator *itr)
{
    dictmap_t *m = (dictmap_t *)_m;
    int i;
    for (i = itr->index; i < m->used_size; ++i) {
        pmap_record_t *r = dmap_at(m, i);
        itr->index = i+1;
        return r;
    }
    itr->index = m->used_size;
    return NULL;
}

static pmap_record_t *dictmap_get(poolmap_t *_m, char *key, unsigned klen)
{
    dictmap_t *m = (dictmap_t *)_m;
    unsigned hash = m->api->fkey0(key, klen);
    pmap_record_t *r = _dmap_get(m, hash, m->api->fkey1(key, klen));
    return r;
}

static void dictmap_dispose(poolmap_t *_m)
{
    dictmap_t *m = (dictmap_t *)_m;
    int i;
    for (i = 0; i < m->used_size; ++i) {
        if (likely(m->hash_list[i])) {
            pmap_record_t *r = dmap_at(m, i);
            JSON_free((JSON)r->k);
            m->api->ffree(r);
        }
    }
    free((pmap_record_t *)m->records);
}

static const poolmap_api_t DICT = {
    dictmap_get,
    dictmap_set,
    dictmap_next,
    dictmap_remove,
    dictmap_init,
    dictmap_dispose
};

static const poolmap_api_t HASH = {
    hashmap_get,
    hashmap_set,
    hashmap_next,
    hashmap_remove,
    hashmap_init,
    hashmap_dispose
};

static const poolmap_api_t *DICT_OP = &DICT;
static const poolmap_api_t *HASH_OP = &HASH;

poolmap_t *poolmap_new(unsigned init, const poolmap_entry_api_t *api)
{
    const poolmap_api_t *op = (init > DICTMAP_THRESHOLD)? HASH_OP:DICT_OP;
    poolmap_t *m = (init <= DICTMAP_THRESHOLD) ?
        dictmap_new(api) : hashmap_new(init, api);
    m->h.op = op;
    return m;
}

void poolmap_init(poolmap_t *m, unsigned init, const poolmap_entry_api_t *api)
{
    const poolmap_api_t *op = (init > DICTMAP_THRESHOLD)? HASH_OP:DICT_OP;
    m->h.op = op;
    op->_init(m, init, api);
}

void poolmap_delete(poolmap_t *m)
{
    poolmap_dispose(m);
    free(m);
}

static void dictmap_convert2hashmap(dictmap_t *_m)
{
    hashmap_t *m = (hashmap_t *) _m;
    m->op = HASH_OP;
    m->record_size_mask = DICTMAP_THRESHOLD-1;
    hmap_record_resize(m);
}

#ifdef __cplusplus
}
#endif
