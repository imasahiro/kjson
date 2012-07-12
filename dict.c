#include "kjson.h"
#include "hash.h"
#include "map.h"
#include "internal.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

static dictmap_t *_dictmap_init(dictmap_t* m, const poolmap_entry_api_t *api)
{
    int i;
    const size_t allocSize = sizeof(pmap_record_t)*DICTMAP_THRESHOLD;
    uint64_t *hash_list = (uint64_t *) m->hash_list;
    for (i = 0; i < DICTMAP_THRESHOLD/2; ++i) {
        hash_list[i] = 0;
    }
    m->api = api;
    m->records = cast(pmap_record_t *,malloc(allocSize));
    m->used_size = 0;
    return m;
}

void dictmap_init(dictmap_t* m, const poolmap_entry_api_t *api)
{
    _dictmap_init(m, api);
}

dictmap_t* dictmap_new(const poolmap_entry_api_t *api)
{
    dictmap_t *m = cast(dictmap_t *, malloc(sizeof(*m)));
    return _dictmap_init(m, api);
}

static void dmap_record_copy(pmap_record_t *dst, const pmap_record_t *src)
{
    memcpy(dst, src, sizeof(pmap_record_t));
}

static inline pmap_record_t *dmap_at(dictmap_t *m, uint32_t idx)
{
    return (pmap_record_t*)(m->records+idx);
}

static pmap_status_t _dmap_set(dictmap_t *m, pmap_record_t *rec)
{
    int i = 0;
    pmap_record_t *r;
    L_head:;
    for (; i < DICTMAP_THRESHOLD; ++i) {
        uint32_t hash = m->hash_list[i];
        if (hash == rec->hash) {
            goto L_update;
        }
        if (hash == 0) {
            goto L_new;
        }
    }
    assert(0);
    return -1;
    L_new:;
    r = dmap_at(m, i);
    m->hash_list[i] = rec->hash;
    dmap_record_copy(r, rec);
    ++m->used_size;
    return POOLMAP_ADDED;

    L_update:;
    r = dmap_at(m, i);
    if (!m->api->fcmp(r->k, rec->k)) {
        goto L_head;
    }
    uintptr_t old0 = r->v;
    uint32_t  old1 = r->v2;
    m->api->ffree(r);
    dmap_record_copy(r, rec);
    rec->v  = old0;
    rec->v2 = old1;
    return POOLMAP_UPDATE;
}

static pmap_record_t *dmap_get_(dictmap_t *m, uint32_t hash, uintptr_t key)
{
    int i = 0;
    for (i = 0; i < DICTMAP_THRESHOLD; ++i) {
        if (hash == m->hash_list[i]) {
            pmap_record_t *r = dmap_at(m, i);
            if (m->api->fcmp(r->k, key)) {
                return r;
            }
        }
    }
    return NULL;
}

pmap_status_t dictmap_set(dictmap_t *m, char *key, uint32_t klen, void *val)
{
    pmap_record_t r;
    r.hash = m->api->fkey0(key, klen);
    r.k  = m->api->fkey1(key, klen);
    r.v  = cast(uintptr_t, val);
    r.v2 = 0;
    return _dmap_set(m, &r);
}

void dictmap_remove(dictmap_t *m, char *key, uint32_t klen)
{
    uint32_t hash = m->api->fkey0(key, klen);
    pmap_record_t *r = dmap_get_(m, hash, m->api->fkey1(key, klen));
    if (r) {
        r->hash = 0;
        r->k = 0;
        m->used_size -= 1;
    }
}

pmap_record_t *dictmap_next(dictmap_t *m, poolmap_iterator *itr)
{
    int i;
    for (i = itr->index; i < DICTMAP_THRESHOLD; ++i) {
        pmap_record_t *r = dmap_at(m, i);
        itr->index = i+1;
        return r;
    }
    itr->index = i;
    assert(itr->index == DICTMAP_THRESHOLD);
    return NULL;
}

pmap_record_t *dictmap_get(dictmap_t *m, char *key, uint32_t klen)
{
    uint32_t hash = m->api->fkey0(key, klen);
    pmap_record_t *r = dmap_get_(m, hash, m->api->fkey1(key, klen));
    return r;
}

void dictmap_dispose(dictmap_t *m)
{
    int i;
    for (i = 0; i < m->used_size; ++i) {
        if (likely(m->hash_list[i])) {
            pmap_record_t *r = dmap_at(m, i);
            JSON_free((JSON)r->k);
            m->api->ffree(r);
        }
    }
    free((pmap_record_t*)m->records);
}

void dictmap_delete(dictmap_t *m)
{
    assert(m != 0);
    dictmap_dispose(m);
    free(m);
}

#ifdef __cplusplus
}
#endif
