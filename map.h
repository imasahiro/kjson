#include <stdint.h>

#ifndef KJSON_MAP_H
#define KJSON_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

struct JSONString;
typedef struct kmap_record {
    unsigned hash;
    struct JSONString *k;
    uint64_t v;
} __attribute__((__aligned__(8))) map_record_t;

typedef int (*fn_keycmp)(struct JSONString *s0, struct JSONString *s1);

struct map_api;
typedef struct map_api kmap_api_t;

struct map_base {
    const kmap_api_t *op;
    map_record_t *records;
} __attribute__ ((packed));

typedef struct hashmap_t {
    struct map_base base;
    unsigned used_size;
    unsigned record_size_mask;
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
    map_record_t *(*_get)(kmap_t *m, struct JSONString *key);
    map_status_t  (*_set)(kmap_t *m, struct JSONString *key, uint64_t val);
    map_record_t *(*_next)(kmap_t *m, kmap_iterator *itr);
    void (*_remove)(kmap_t *m, struct JSONString *key);
    void (*_init)(kmap_t *m, unsigned init);
    void (*_dispose)(kmap_t *m);
};

kmap_t *kmap_new(unsigned init);
void kmap_init(kmap_t *m, unsigned init);
void kmap_delete(kmap_t *m);

static inline void kmap_dispose(kmap_t *m)
{
    m->h.base.op->_dispose(m);
}

static inline map_record_t *kmap_get(kmap_t *m, struct JSONString *key)
{
    return m->h.base.op->_get(m, key);
}

static inline map_status_t kmap_set(kmap_t *m, struct JSONString *key, uint64_t val)
{
    return m->h.base.op->_set(m, key, val);
}

static inline void kmap_remove(kmap_t *m, struct JSONString *key)
{
    return m->h.base.op->_remove(m, key);
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
