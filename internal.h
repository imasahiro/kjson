#ifndef KJSON_INTERNAL_H_
#define KJSON_INTERNAL_H_

#define cast(T, V) ((T)(V))
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
#endif /* end of include guard */
