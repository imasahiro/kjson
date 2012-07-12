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

#define POOLMAP_INITSIZE      4

static inline void map_do_bzero(void *ptr, size_t size)
{
    memset(ptr, 0, size);
}

static inline void *map_do_malloc(size_t size)
{
    void *ptr = malloc(size);
    map_do_bzero(ptr, size);
    return ptr;
}

static inline void map_do_free(void *ptr, size_t size)
{
    map_do_bzero(ptr, size);
    free(ptr);
}

static void pmap_record_copy(pmap_record_t *dst, const pmap_record_t *src)
{
    memcpy(dst, src, sizeof(pmap_record_t));
}
//static void pmap_record_copy_data(pmap_record_t *dst, const pmap_record_t *src)
//{
//    assert(dst->hash == src->hash);
//    memcpy(dst, src, sizeof(pmap_record_t));
//    //dst->v2 = src->v2;
//    //dst->v  = src->v;
//}

static inline pmap_record_t *pmap_at(poolmap_t *m, uint32_t idx)
{
    assert(idx < (m->record_size_mask+1));
    return m->records+idx;
}

static void pmap_record_reset(poolmap_t *m, size_t newsize)
{
    uint32_t alloc_size = sizeof(pmap_record_t) * newsize;
    m->used_size = 0;
    (m->record_size_mask) = newsize - 1;
    m->records = cast(pmap_record_t *, map_do_malloc(alloc_size));
}

static pmap_status_t pmap_set_no_resize(poolmap_t *m, pmap_record_t *rec)
{
    uint32_t i = 0, idx = rec->hash & m->record_size_mask;
    pmap_record_t *r;
    do {
        r = m->records+idx;
        if (r->hash == 0) {
            pmap_record_copy(r, rec);
            ++m->used_size;
            return POOLMAP_ADDED;
        }
        if (r->hash == rec->hash && m->fcmp(r->k, rec->k)) {
            uintptr_t old0 = r->v;
            uint32_t  old1 = r->v2;
            m->ffree(r);
            pmap_record_copy(r, rec);
            rec->v  = old0;
            rec->v2 = old1;
            return POOLMAP_UPDATE;
        }
        idx = (idx + 1) & m->record_size_mask;
    } while (i++ < m->used_size);
    assert(0);
}

static void pmap_record_resize(poolmap_t *m)
{
    pmap_record_t *old = m->records;
    uint32_t i, oldsize = (m->record_size_mask+1);

    pmap_record_reset(m, oldsize*2);
    for (i = 0; i < oldsize; ++i) {
        pmap_record_t *r = old + i;
        if (r->hash) {
            pmap_set_no_resize(m, r);
        }
    }
    map_do_free(old, oldsize*sizeof(pmap_record_t));
}

static pmap_status_t pmap_set_(poolmap_t *m, pmap_record_t *rec)
{
    if (m->used_size > (m->record_size_mask+1) * 3 / 4) {
        pmap_record_resize(m);
    }
    return pmap_set_no_resize(m, rec);
}

static pmap_record_t *pmap_get_(poolmap_t *m, uint32_t hash, uintptr_t key)
{
    uint32_t i = 0;
    uint32_t idx = hash & m->record_size_mask;
    do {
        pmap_record_t *r = pmap_at(m, idx);
        if (r->hash == hash && m->fcmp(r->k, key)) {
            return r;
        }
        idx = (idx + 1) & m->record_size_mask;
    } while (i++ < m->used_size);
    return NULL;
}
static void _poolmap_init(poolmap_t* m, uint32_t init, fn_keygen fkey0, fn_keygen fkey1, fn_keycmp fcmp, fn_efree ffree)
{
    if (init < POOLMAP_INITSIZE)
        init = POOLMAP_INITSIZE;
    pmap_record_reset(m, 1U << (SizeToKlass(init)));
    m->fkey0 = fkey0;
    m->fkey1 = fkey1;
    m->fcmp  = fcmp;
    m->ffree = ffree;
}

poolmap_t* poolmap_new(uint32_t init, fn_keygen fkey0, fn_keygen fkey1, fn_keycmp fcmp, fn_efree ffree)
{
    poolmap_t *m = cast(poolmap_t *, map_do_malloc(sizeof(*m)));
    _poolmap_init(m, init, fkey0, fkey1, fcmp, ffree);
    return m;
}

void poolmap_init(poolmap_t* m, uint32_t init, fn_keygen fkey0, fn_keygen fkey1, fn_keycmp fcmp, fn_efree ffree)
{
    _poolmap_init(m, init, fkey0, fkey1, fcmp, ffree);
}

void poolmap_dispose(poolmap_t *m)
{
    uint32_t i, size = (m->record_size_mask+1);
    for (i = 0; i < size; ++i) {
        pmap_record_t *r = pmap_at(m, i);
        if (r->hash) {
            JSON_free((JSON)r->k);
            m->ffree(r);
        }
    }
    map_do_free(m->records, (m->record_size_mask+1) * sizeof(pmap_record_t));
}

void poolmap_delete(poolmap_t *m)
{
    assert(m != 0);
    poolmap_dispose(m);
    map_do_free(m, sizeof(*m));
}

pmap_record_t *poolmap_get(poolmap_t *m, char *key, uint32_t klen)
{
    uint32_t hash = m->fkey0(key, klen);
    pmap_record_t *r = pmap_get_(m, hash, m->fkey1(key, klen));
    return r;
}

pmap_status_t poolmap_set(poolmap_t *m, char *key, uint32_t klen, void *val)
{
    pmap_record_t r;
    r.hash = m->fkey0(key, klen);
    r.k  = m->fkey1(key, klen);
    r.v  = cast(uintptr_t, val);
    r.v2 = 0;
    return pmap_set_(m, &r);
}

pmap_status_t poolmap_set2(poolmap_t *m, char *key, uint32_t klen, pmap_record_t *r)
{
    r->k  = m->fkey1(key, klen);
    r->hash = m->fkey0(key, klen);
    return pmap_set_(m, r);
}

void poolmap_remove(poolmap_t *m, char *key, uint32_t klen)
{
    uint32_t hash = m->fkey0(key, klen);
    pmap_record_t *r = pmap_get_(m, hash, m->fkey1(key, klen));
    if (r) {
        r->hash = 0;
        r->k = 0;
        m->used_size -= 1;
    }
}

pmap_record_t *poolmap_next(poolmap_t *m, poolmap_iterator *itr)
{
    uint32_t i, size = (m->record_size_mask+1);
    for (i = itr->index; i < size; ++i) {
        pmap_record_t *r = pmap_at(m, i);
        if (r->hash) {
            itr->index = i+1;
            return r;
        }
    }
    itr->index = i;
    assert(itr->index == size);
    return NULL;
}

#ifdef __cplusplus
}
#endif
