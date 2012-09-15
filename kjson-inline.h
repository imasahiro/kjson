#include "kmemory.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifndef KJSON_INLINE_H_
#define KJSON_INLINE_H_

#ifndef LOG2
#define LOG2(N) ((uint32_t)((sizeof(void*) * 8) - __builtin_clzl(N - 1)))
#endif

#ifdef __cplusplus
static inline JSON toJSON(Value v) {
    JSON json;
    json.bits = v.bits;
    return json;
}
#else
#define toJSON(O) ((JSON) O)
#endif

#ifndef INT32_MAX
#define INT32_MAX        2147483647
#endif

#ifndef INT32_MIN
#define INT32_MIN        (-INT32_MAX-1)
#endif

/* [Getter API] */
static inline char *JSONString_get(JSON json)
{
    JSONString *s = toStr(json.val);
    return s->str;
}

static inline int32_t JSONInt_get(JSON json)
{
    return toInt32(json.val);
}

static inline double JSONDouble_get(JSON json)
{
    return toDouble(json.val);
}

static inline int JSONBool_get(JSON json)
{
    return toBool(json.val);
}

/* [New API] */
static inline JSON JSONString_new(char *s, size_t len)
{
    JSONString *o = (JSONString *) KJSON_MALLOC(sizeof(*o)+len+1);
    o->str = (char *) (o+1);
    memcpy(o->str, s, len);
    o->hashcode = 0;
    o->length = len;
    o->str[len] = 0;
    return toJSON(ValueS(o));
}

static inline JSON JSONNull_new()
{
    return toJSON(ValueN());
}

static inline JSON JSONObject_new()
{
    JSONObject *o = (JSONObject *) KJSON_MALLOC(sizeof(*o));
    kmap_init(&(o->child), 0);
    return toJSON(ValueO(o));
}

static inline JSON JSONArray_new()
{
    JSONArray *o = (JSONArray *) KJSON_MALLOC(sizeof(*o));
    o->length   = 0;
    o->capacity = 0;
    o->list   = NULL;
    return toJSON(ValueA(o));
}

static inline JSON JSONDouble_new(double val)
{
    return toJSON(ValueF(val));
}

static inline JSON JSONInt_new(int64_t val)
{
    if (val > (int64_t)INT32_MAX || val < (int64_t)INT32_MIN) {
        JSONInt64 *i64 = (JSONInt64 *) KJSON_MALLOC(sizeof(JSONInt64));
        i64->val = val;
        return toJSON(ValueIO(i64));
    } else {
        return toJSON(ValueI(val));
    }
}

static inline JSON JSONBool_new(bool val)
{
    return toJSON(ValueB(val));
}

#endif /* end of include guard */