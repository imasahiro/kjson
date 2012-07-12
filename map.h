#include <stdint.h>

#ifndef KJSON_MAP_H
#define KJSON_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct poolmap_record {
    uint32_t hash;
    uint32_t v2;
    uintptr_t k;
    uintptr_t v;
} pmap_record_t;

typedef uintptr_t (*fn_keygen)(char *key, uint32_t klen);
typedef int  (*fn_keycmp)(uintptr_t k0, uintptr_t k1);
typedef void (*fn_efree)(pmap_record_t *r);
typedef struct poolmap_t {
    pmap_record_t *records;
    uint32_t record_size_mask;
    uint32_t used_size;
    fn_keygen fkey0;
    fn_keygen fkey1;
    fn_keycmp fcmp;
    fn_efree  ffree;
} poolmap_t;

typedef struct map_iterator {
    long index;
} poolmap_iterator;

typedef enum pmap_status_t {
    POOLMAP_FAILED = 0,
    POOLMAP_UPDATE = 1,
    POOLMAP_ADDED  = 2
} pmap_status_t;

static inline uint32_t poolmap_size(poolmap_t *m)
{
    return m->used_size;
}

int pool_global_init(void);
int pool_global_deinit(void);
poolmap_t* poolmap_new(uint32_t init, fn_keygen fkey0, fn_keygen fkey1, fn_keycmp fcmp, fn_efree ffree);
void poolmap_init(poolmap_t* m, uint32_t init, fn_keygen fkey0, fn_keygen fkey1, fn_keycmp fcmp, fn_efree ffree);
void poolmap_delete(poolmap_t *m);
void poolmap_dispose(poolmap_t *m);
pmap_record_t *poolmap_get(poolmap_t *m, char *key, uint32_t tlen);
pmap_status_t poolmap_set(poolmap_t *m, char *key, uint32_t klen, void *val);
pmap_status_t poolmap_set2(poolmap_t *m, char *key, uint32_t klen, pmap_record_t *);
void poolmap_remove(poolmap_t *m, char *key, uint32_t klen);
pmap_record_t *poolmap_next(poolmap_t *m, poolmap_iterator *itr);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
