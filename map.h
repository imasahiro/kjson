#include <stdint.h>

#ifndef KJSON_MAP_H
#define KJSON_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct poolmap_record {
    unsigned hash;
    unsigned v2;
    uintptr_t k;
    uintptr_t v;
} __attribute__((__aligned__(8))) pmap_record_t;

typedef uintptr_t (*fn_keygen)(char *key, unsigned klen);
typedef int  (*fn_keycmp)(uintptr_t k0, uintptr_t k1);
typedef void (*fn_efree)(pmap_record_t *r);

typedef struct poolmap_entry_api {
    fn_keygen fkey0;
    fn_keygen fkey1;
    fn_keycmp fcmp;
    fn_efree  ffree;
} poolmap_entry_api_t;

struct map_api;
typedef struct map_api poolmap_api_t;

typedef struct hashmap_t {
    const poolmap_api_t *op;
    const poolmap_entry_api_t *api;
    pmap_record_t *records;
    unsigned used_size;
    unsigned record_size_mask;
    char alignment[16];
} __attribute__((__aligned__(8))) hashmap_t;

#define DICTMAP_THRESHOLD 4
typedef struct dictmap_t {
    const poolmap_api_t *op;
    const struct poolmap_entry_api *api;
    const pmap_record_t *records;
    unsigned  used_size;
    unsigned  hash_list[DICTMAP_THRESHOLD];
} __attribute__((__aligned__(8))) dictmap_t;

typedef union poolmap_t {
    hashmap_t h;
    dictmap_t d;
} poolmap_t;

typedef struct map_iterator {
    long index;
} poolmap_iterator;

typedef enum pmap_status_t {
    POOLMAP_FAILED = 0,
    POOLMAP_UPDATE = 1,
    POOLMAP_ADDED  = 2
} pmap_status_t;

struct map_api {
    pmap_record_t *(*_get)(poolmap_t *m, char *key, unsigned tlen);
    pmap_status_t  (*_set)(poolmap_t *m, char *key, unsigned klen, void *val);
    pmap_record_t *(*_next)(poolmap_t *m, poolmap_iterator *itr);
    void (*_remove)(poolmap_t *m, char *key, unsigned klen);
    void (*_init)(poolmap_t *m, unsigned init, const poolmap_entry_api_t *api);
    void (*_dispose)(poolmap_t *m);
};

poolmap_t *poolmap_new(unsigned init, const poolmap_entry_api_t *api);
void poolmap_init(poolmap_t *m, unsigned init, const poolmap_entry_api_t *api);
void poolmap_delete(poolmap_t *m);

static inline void poolmap_dispose(poolmap_t *m)
{
    m->h.op->_dispose(m);
}

static inline pmap_record_t *poolmap_get(poolmap_t *m, char *key, unsigned klen)
{
    return m->h.op->_get(m, key, klen);
}

static inline pmap_status_t poolmap_set(poolmap_t *m, char *key, unsigned klen, void *val)
{
    return m->h.op->_set(m, key, klen, val);
}

static inline void poolmap_remove(poolmap_t *m, char *key, unsigned klen)
{
    return m->h.op->_remove(m, key, klen);
}

static inline pmap_record_t *poolmap_next(poolmap_t *m, poolmap_iterator *itr)
{
    return m->h.op->_next(m, itr);
}

static inline unsigned poolmap_size(poolmap_t *m)
{
    return m->h.used_size;
}

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
