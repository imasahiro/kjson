#include <stdint.h>

#ifndef KJSON_MAP_H
#define KJSON_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kmap_record {
    unsigned hash;
    unsigned v2;
    uintptr_t k;
    uintptr_t v;
} __attribute__((__aligned__(8))) map_record_t;

typedef uintptr_t (*fn_keygen)(char *key, unsigned klen);
typedef int  (*fn_keycmp)(uintptr_t k0, uintptr_t k1);
typedef void (*fn_efree)(map_record_t *r);

typedef struct kmap_entry_api {
    fn_keygen fkey0;
    fn_keygen fkey1;
    fn_keycmp fcmp;
    fn_efree  ffree;
} kmap_entry_api_t;

struct map_api;
typedef struct map_api kmap_api_t;

struct map_base {
    const kmap_api_t *op;
    const kmap_entry_api_t *entry_api;
    map_record_t *records;
} __attribute__ ((packed));

typedef struct hashmap_t {
    struct map_base base;
    unsigned used_size;
    unsigned record_size_mask;
    char alignment[16];
} __attribute__((__aligned__(8))) hashmap_t;

#define DICTMAP_THRESHOLD 4
typedef struct dictmap_t {
    struct map_base base;
    unsigned  used_size;
    unsigned  hash_list[DICTMAP_THRESHOLD];
} __attribute__((__aligned__(8))) dictmap_t;

typedef union kmap_t {
    hashmap_t h;
    dictmap_t d;
} kmap_t;

typedef struct map_iterator {
    long index;
} kmap_iterator;

typedef enum map_status_t {
    KMAP_FAILED = 0,
    KMAP_UPDATE = 1,
    KMAP_ADDED  = 2
} map_status_t;

struct map_api {
    map_record_t *(*_get)(kmap_t *m, char *key, unsigned tlen);
    map_status_t  (*_set)(kmap_t *m, char *key, unsigned klen, void *val);
    map_record_t *(*_next)(kmap_t *m, kmap_iterator *itr);
    void (*_remove)(kmap_t *m, char *key, unsigned klen);
    void (*_init)(kmap_t *m, unsigned init, const kmap_entry_api_t *api);
    void (*_dispose)(kmap_t *m);
};

kmap_t *kmap_new(unsigned init, const kmap_entry_api_t *entry_api);
void kmap_init(kmap_t *m, unsigned init, const kmap_entry_api_t *entry_api);
void kmap_delete(kmap_t *m);

static inline void kmap_dispose(kmap_t *m)
{
    m->h.base.op->_dispose(m);
}

static inline map_record_t *kmap_get(kmap_t *m, char *key, unsigned klen)
{
    return m->h.base.op->_get(m, key, klen);
}

static inline map_status_t kmap_set(kmap_t *m, char *key, unsigned klen, void *val)
{
    return m->h.base.op->_set(m, key, klen, val);
}

static inline void kmap_remove(kmap_t *m, char *key, unsigned klen)
{
    return m->h.base.op->_remove(m, key, klen);
}

static inline map_record_t *kmap_next(kmap_t *m, kmap_iterator *itr)
{
    return m->h.base.op->_next(m, itr);
}

static inline unsigned kmap_size(kmap_t *m)
{
    return m->h.used_size;
}

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
