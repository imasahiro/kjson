#include <stdio.h>

#ifndef KJSON_H_
#define KJSON_H_

#include "numbox.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

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

union JSONValue;
typedef union JSONValue JSON;

typedef struct JSONString {
    unsigned length;
    unsigned hashcode;
    char *str;
} JSONString;
typedef JSONString JSONUString;

typedef struct JSONArray {
    int  length;
    int  capacity;
    JSON *list;
} JSONArray;

typedef Value JSONNumber;

typedef JSONNumber JSONInt;
typedef JSONInt    JSONInt32;
typedef JSONNumber JSONDouble;
typedef JSONNumber JSONBool;

typedef struct JSONInt64 {
    kjson_type type;
    int64_t val;
} JSONInt64;

typedef struct JSONObject {
    kmap_t child;
} JSONObject;

union JSONValue {
    Value       val;
    JSONNumber  num;
    JSONString *str;
    JSONArray  *ary;
    JSONObject *obj;
    uint64_t bits;
};

typedef JSONNumber JSONNull;

/* [Getter API] */
unsigned JSON_length(JSON json);
JSON *JSON_getArray(JSON json, const char *key, size_t *len);
const char *JSON_getString(JSON json, const char *key, size_t *len);
double JSON_getDouble(JSON json, const char *key);
int JSON_getBool(JSON json, const char *key);
int JSON_getInt(JSON json, const char *key);
JSON JSON_get(JSON json, const char *key);

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
JSON JSONNull_new(void);
JSON JSONArray_new(void);
JSON JSONObject_new(void);
JSON JSONDouble_new(double val);
JSON JSONString_new(char *s, size_t len);
JSON JSONInt_new(int64_t val);
JSON JSONBool_new(int val);

/* [Other API] */
void JSONObject_set(JSON obj, JSON key, JSON value);
void JSONArray_set(JSON ary, int id, JSON value);
void JSONArray_append(JSON ary, JSON o);
void JSON_free(JSON o);
void JSON_dump(FILE *fp, JSON json);

JSON parseJSON(const char *s, const char *e);
char *JSON_toStringWithLength(JSON json, size_t *len);
static inline char *JSON_toString(JSON json)
{
    size_t len;
    char *s = JSON_toStringWithLength(json, &len);
    return s;
}

static inline bool JSON_isValid(JSON json)
{
    return json.bits != 0;
}

static inline kjson_type JSON_type(JSON json) {
    Value v; v.bits = (uint64_t)json.val.bits;
    uint64_t tag = Tag(v);
    return (IsDouble((v)))?
        JSON_Double : (kjson_type) ((tag >> TagBitShift) & 15);
}
#define JSON_TYPE_CHECK(T, O) (JSON_type(((JSON)O)) == JSON_##T)

#define JSON_ARRAY_EACH(json, A, I, E) JSON_ARRAY_EACH_(json, A, I, E, 0)
#define JSON_ARRAY_EACH_(json, A, I, E, N)\
    if (!JSON_type((json)) == JSON_Array) {} else\
        if (!(A = toAry((json).val)) != 0) {}\
        else \
        for (I = (A)->list + N,\
                E = (A)->list+(A)->length; I != E; ++I)

typedef struct JSONObject_iterator {
    long index;
    JSONObject *obj;
} JSONObject_iterator;

int JSONObject_iterator_init(JSONObject_iterator *itr, JSON obj);
JSON JSONObject_iterator_next(JSONObject_iterator *itr, JSON *val);

#define OBJECT_INIT(O, ITR) (JSONObject_iterator_init(ITR, O))
#define JSON_OBJECT_EACH(O, ITR, KEY, VAL)\
    if (!JSON_TYPE_CHECK(Object, (JSON)O)) {} else\
    if (!OBJECT_INIT(O, &ITR)) {}\
    else\
    for (KEY = JSONObject_iterator_next(&ITR, &VAL); KEY.bits;\
            KEY = JSONObject_iterator_next(&ITR, &VAL))

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
