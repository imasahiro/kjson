#ifndef KJSON_INTERNAL_H_
#define KJSON_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLZ
#define CLZ(n) __builtin_clzl(n)
#endif

#ifndef LOG2
#define LOG2(N) ((uint32_t)((sizeof(void*) * 8) - CLZ(N - 1)))
#endif

void _JSONString_free(JSONString *obj);

#ifdef __cplusplus
static inline JSON toJSON(Value v) {
    JSON json;
    json.bits = v.bits;
    return json;
}
#else
#define toJSON(O) ((JSON) O)
#endif

#ifdef __cplusplus
}
#endif
#endif /* end of include guard */
