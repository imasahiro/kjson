#include <stdio.h>
#include "numbox.h"
#include "map.h"

#ifndef KJSON_H_
#define KJSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#define USE_NUMBOX
typedef enum kjson_type {
    /** ($type & 1 == 0) means $type extends Number */
    JSON_Double =  0, /* 0000 */
    JSON_String =  1, /* 0001 */
    JSON_Int32  =  2, /* 0010 */
    JSON_Object =  3, /* 0011 */
    JSON_Bool   =  4, /* 0100 */
    JSON_Array  =  5, /* 0101 */
    JSON_Null   =  6, /* 0110 */
    JSON_UString = 9, /* 1001 */
    JSON_Int64  = 11, /* 1011 */
    JSON_Error  = 15, /* 1111 */
    /* '7' is reserved by numbox*/
    JSON_reserved  =  7 /* 0111 */
} kjson_type;

union JSON;
typedef union JSON *JSON;

typedef struct JSONString {
#ifndef USE_NUMBOX
    kjson_type type;
#endif
    int length;
    char str[1];
} JSONString;
typedef JSONString JSONUString;

typedef struct JSONArray {
#ifndef USE_NUMBOX
    kjson_type type;
#endif
    int  length;
    int  capacity;
    JSON *list;
} JSONArray;

#ifndef USE_NUMBOX
typedef struct JSONNumber {
    kjson_type type;
    long val;
} JSONNumber;
#else
typedef Value JSONNumber;
#endif

typedef JSONNumber JSONInt;
typedef JSONInt JSONInt32;
typedef JSONNumber JSONDouble;
typedef JSONNumber JSONBool;

typedef struct JSONInt64 {
    kjson_type type;
    int64_t val;
} JSONInt64;

typedef struct JSONObject {
#ifndef USE_NUMBOX
    kjson_type type;
#endif
    poolmap_t child;
} JSONObject;

union JSON {
#ifndef USE_NUMBOX
    struct JSON_base {
        kjson_type type;
    } base;
#endif
    JSONString str;
    JSONArray  ary;
    JSONNumber num;
    JSONObject obj;
};

#ifdef USE_NUMBOX
typedef JSONNumber JSONNull;
#else
typedef union JSON JSONNull;
#endif

/* [Converter API] */
#define JSON_CONVERTER(T) static inline JSON##T *toJSON##T (JSON o) { return (JSON##T*) o; }
JSON_CONVERTER(Object)
JSON_CONVERTER(Array)
JSON_CONVERTER(String)
JSON_CONVERTER(Bool)
JSON_CONVERTER(Number)
JSON_CONVERTER(Double)
JSON_CONVERTER(Null)
#undef CONVERTER

/* [Getter API] */
unsigned JSON_length(JSON json);
JSON *JSON_getArray(JSON json, char *key, size_t *len);
char *JSON_getString(JSON json, char *key, size_t *len);
double JSON_getDouble(JSON json, char *key);
int JSON_getBool(JSON json, char *key);
int JSON_getInt(JSON json, char *key);
JSON JSON_get(JSON json, char *key);

static inline char *JSONString_get(JSON json)
{
    JSONString *s = toJSONString(json);
#ifdef USE_NUMBOX
    s = toJSONString(toStr(toVal(s)));
#endif
    return s->str;
}

static inline int32_t JSONInt_get(JSON json)
{
#ifdef USE_NUMBOX
    return toInt32(toVal(json));
#else
    return (int32_t)((JSONNumber*) json)->val;
#endif
}

static inline double JSONDouble_get(JSON json)
{
#ifdef USE_NUMBOX
    return toDouble(toVal(json));
#else
    union v { double f; long v; } v;
    v.v = ((JSONNumber*)json)->val;
    return v.f;
#endif
}

static inline int JSONBool_get(JSON json)
{
#ifdef USE_NUMBOX
    return toBool(toVal(json));
#else
    return ((JSONNumber*)json)->val;
#endif
}

/* [New API] */
JSON JSONNull_new(void);
JSON JSONArray_new(void);
JSON JSONObject_new(void);
JSON JSONDouble_new(double val);
JSON JSONString_new(char *s, size_t len);
JSON JSONInt_new(int64_t val);
JSON JSONBool_new(int val);

/* [Other API] */
void JSONObject_set(JSONObject *o, JSON key, JSON value);
void JSONArray_set(JSONObject *o, int id, JSON value);
void JSONArray_append(JSONArray *a, JSON o);
void JSON_free(JSON o);
void JSON_dump(FILE *fp, JSON json);

typedef enum kjson_parse_option {
    KJSON_NOP = 0,
    KJSON_USE_BUFFERD,
    KJSON_USE_ALLOC
} kjson_parse_option;

JSON parseJSON(char *s, char *e, kjson_parse_option opt);
char *JSON_toString(JSON json, size_t *len);

#ifdef USE_NUMBOX
static inline kjson_type JSON_type(JSON json) {
    Value v; v.bits = (uint64_t)json;
    uint64_t tag = Tag(v);
    return (IsDouble((v)))?
        JSON_Double : (kjson_type) ((tag >> TagBitShift) & 15);
}
#define JSON_set_type(json, type) assert(0 && "Not supported")
#else
#define JSON_type(json) (((JSON) (json))->base.type)
#define JSON_set_type(json, T) (((JSON) (json))->base.type) = T
#endif
#define JSON_TYPE_CHECK(T, O) (JSON_type(((JSON)O)) == JSON_##T)
#ifdef USE_NUMBOX
#define ARRAY_INIT(A) ((A = toJSONArray(toAry(toVal((JSON)A)))) != NULL)
#else
#define ARRAY_INIT(A) (A)
#endif

#define JSON_ARRAY_EACH(A, I, E) JSON_ARRAY_EACH_(A, I, E, 0)
#define JSON_ARRAY_EACH_(A, I, E, N) \
    if (!JSON_TYPE_CHECK(Array, (JSON)A)) {} else\
        if (!ARRAY_INIT(A)) {}\
        else\
        for (I = (A)->list + N,\
            E = (A)->list+(A)->length; I < E; ++I)


typedef struct JSONObject_iterator {
    long index;
    JSONObject *obj;
} JSONObject_iterator;

int JSONObject_iterator_init(JSONObject_iterator *itr, JSONObject *obj);
JSONString *JSONObject_iterator_next(JSONObject_iterator *itr, JSON *val);

#define OBJECT_INIT(O, ITR) (JSONObject_iterator_init(ITR, O))
#define JSON_OBJECT_EACH(O, ITR, KEY, VAL)\
    if (!JSON_TYPE_CHECK(Object, (JSON)O)) {} else\
    if (!OBJECT_INIT(O, &ITR)) {}\
    else\
    for (KEY = JSONObject_iterator_next(&ITR, &VAL); KEY;\
            KEY = JSONObject_iterator_next(&ITR, &VAL))

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
