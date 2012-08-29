#ifndef KJSON_INTERNAL_H_
#define KJSON_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLZ
#define CLZ(n) __builtin_clzl(n)
#endif

#ifndef BITS
#define BITS (sizeof(void*) * 8)
#endif

#ifndef SizeToKlass
#define SizeToKlass(N) ((uint32_t)(BITS - CLZ(N - 1)))
#endif

#ifndef unlikely
#define unlikely(x)   __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x)     __builtin_expect(!!(x), 1)
#endif

void _JSONString_free(JSONString *obj);

#ifdef KJSON_DEBUG_MODE
void JSON_dump(FILE *fp, JSON json);
void JSON_dump_(JSON s);
#endif

#ifdef __cplusplus
}
#endif
#endif /* end of include guard */
