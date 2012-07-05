/*
 * Map with Inline Cache
 */

typedef struct icmap_record {
    uint32_t hash;
    uintptr_t k;
    uintptr_t v;
} icmap_record_t;

typedef uintptr_t (*fn_keygen)(char *key, uint32_t klen);
typedef int  (*fn_keycmp)(uintptr_t k0, uintptr_t k1);
typedef void (*fn_efree)(pmap_record_t *r);

struct icmap_t;

typedef struct ic_cache {
    struct icmap_t *map;
    long offset;
} ic_cache;

typedef struct icmap_t {
    icmap_record_t *records;
    ic_cache *callsites;
    uint32_t  map_size;
    uint32_t  map_capacity;
    fn_keygen fkey0;
    fn_keygen fkey1;
    fn_keycmp fcmp;
    fn_efree  ffree;
} icmap_t;

typedef ic_cache icmap_iterator;

typedef enum icmap_status_t {
    ICMAP_FAILED = 0,
    ICMAP_UPDATE = 1,
    ICMAP_ADDED  = 2
} icmap_status_t;

icmap_record_t *icmap_get(poolmap_t *m, ic_cache **callsite, char *key, uint32_t klen)
{
    uint32_t i = 0;
    if (*callsite) {
        /* TODO */
        pmap_record_t *r = pmap_at(m, idx);
    }
    for (i = 0; i < m->map_size; i++) {
        pmap_record_t *r = pmap_at(m, idx);
        if (r->hash == hash && m->fcmp(r->k, key)) {
            return r;
        }
        idx = (idx + 1) & m->mask;
    } while (i++ < m->used_size);
    return NULL;
}
icmap_status_t icmap_set(poolmap_t *m, ic_cache **callsite, char *key, uint32_t klen, void *val);
