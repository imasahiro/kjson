#ifndef KMAP_INLINE_H
#define KMAP_INLINE_H
#ifdef __cplusplus
extern "C" {
#endif

/* [kmap inline] */

static inline void kmap_dispose(kmap_t *m)
{
    m->h.base.api->_dispose(m);
}

static inline map_record_t *kmap_get(kmap_t *m, struct JSONString *key)
{
    return m->h.base.api->_get(m, key);
}

static inline map_status_t kmap_set(kmap_t *m, struct JSONString *key, uint64_t val)
{
    return m->h.base.api->_set(m, key, val);
}

static inline void kmap_remove(kmap_t *m, struct JSONString *key)
{
    return m->h.base.api->_remove(m, key);
}

static inline map_record_t *kmap_next(kmap_t *m, kmap_iterator *itr)
{
    return m->h.base.api->_next(m, itr);
}

static inline unsigned kmap_size(kmap_t *m)
{
    return m->h.used_size;
}

extern const kmap_api_t DICT;
extern const kmap_api_t HASH;
static inline void kmap_init(kmap_t *m, unsigned init)
{
    const kmap_api_t *api = (init > DICTMAP_THRESHOLD) ? &HASH:&DICT;
    m->h.base.api = api;
    api->_init(m, init);
}

#ifdef __cplusplus
}
#endif
#endif /* end of include guard */
