#include "kjson.h"
#include "stream.h"
#include "string_builder.h"
#include "map.h"
#include "hash.h"
#include "internal.h"

#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

static void _JSON_free(JSON o);
static uintptr_t json_keygen1(char *key, uint32_t klen)
{
    (void)klen;
    return (uintptr_t) key;
}

struct _string {
    char  *str;
    size_t len;
};

static uintptr_t json_keygen0(char *key, uint32_t klen)
{
    JSONString *s = toJSONString((JSON)key);
#ifdef USE_NUMBOX
    kjson_type type = JSON_type((JSON)s);
    if (type > 0) {
        s = toJSONString(toStr(toVal(s)));
        key  = s->str;
        klen = s->length;
    } else {
        struct _string *s = (struct _string *) key;
        key  = s->str;
        klen = s->len;
    }
#endif
    return djbhash(key, klen);
}

static int json_keycmp(uintptr_t k0, uintptr_t k1)
{
    JSONString *s0 = toJSONString((JSON)k0);
    JSONString *s1 = toJSONString((JSON)k1);
#ifdef USE_NUMBOX
    char  *key;
    size_t len;
    s0 = toJSONString(toStr(toVal(s0)));
    kjson_type type = JSON_type((JSON)s1);
    if (type > 0) {
        s1 = toJSONString(toStr(toVal(s1)));
        key = s1->str;
        len = s1->length;
    } else {
        key = ((struct _string *) k1)->str;
        len = ((struct _string *) k1)->len;
    }
    return s0->length == len &&
        strncmp(s0->str, key, len) == 0;
#else
    return s0->length == s1->length &&
        strncmp(s0->str, s1->str, s0->length) == 0;
#endif
}

static void json_recfree(map_record_t *r)
{
    JSON json = (JSON) r->v;
    _JSON_free(json);
}

static inline JSON toJSON(Value v) { return (JSON) v.pval; }

#define JSON_NEW_(T, SIZE) (JSON##T *) JSON_new(JSON_##T, (sizeof(union JSONValue) + SIZE))
#define JSON_NEW(T)        JSON_NEW_(T, 0);

static inline JSON JSON_new(kjson_type type, size_t size)
{
    JSON json = (JSON) malloc(size);
#ifndef USE_NUMBOX
    JSON_set_type(json, type);
#endif
    return json;
}
static JSON JSONString_new2(string_builder *builder)
{
    size_t len;
    char *s = string_builder_tostring(builder, &len, 1);
    JSONString *o = JSON_NEW_(UString, len);
    memcpy(o->str, s, len);
    o->length = len - 1;
    o->str[len] = 0;
    free(s);
#ifdef USE_NUMBOX
    return toJSON(ValueU((JSON)o));
#else
    return (JSON) o;
#endif
}

JSON JSONString_new(char *s, size_t len)
{
    JSONString *o = JSON_NEW_(String, len+1);
    memcpy(o->str, s, len);
    o->length = len;
    o->str[len] = 0;
#ifdef USE_NUMBOX
    return toJSON(ValueS((JSON)o));
#else
    return (JSON) o;
#endif
}

JSON JSONNull_new()
{
#ifdef USE_NUMBOX
    return toJSON(ValueN());
#else
    return JSON_NEW(Null);
#endif
}

static const struct kmap_entry_api API = {
    json_keygen0, json_keygen1, json_keycmp, json_recfree
};

JSON JSONObject_new()
{
    JSONObject *o = JSON_NEW(Object);
    kmap_init(&(o->child), 0, &API);
#ifdef USE_NUMBOX
    o = toJSONObject(toJSON(ValueO((JSON)o)));
#endif
    return (JSON) o;
}

JSON JSONArray_new()
{
    JSONArray *o = JSON_NEW(Array);
    o->length   = 0;
    o->capacity = 0;
    o->list   = NULL;
#ifdef USE_NUMBOX
    o = toJSONArray(toJSON(ValueA((JSON)o)));
#endif
    return (JSON) o;
}

JSON JSONDouble_new(double val)
{
    JSONNumber *o;
#ifndef USE_NUMBOX
    union v { double f; long v; } v;
    v.f = val;
    o = JSON_NEW(Double);
    o->val = v.v;
    JSON_set_type(o, JSON_Double);
#else
    o = (JSONNumber *) ValueF(val).pval;
#endif
    return (JSON) o;
}

JSON JSONInt_new(int64_t val)
{
    JSONNumber *o;
    if (val > (int64_t)INT32_MAX || val < (int64_t)INT32_MIN) {
        JSONInt64 *i64 = (JSONInt64 *) JSON_NEW(Int64);
        i64->val = val;
        o = (JSONNumber *) i64;
#ifdef USE_NUMBOX
        o = (JSONNumber *) toJSON(ValueIO((JSON)o));
#endif
    } else {
#ifndef USE_NUMBOX
        o = JSON_NEW(Int32);
        o->val = val;
        JSON_set_type(o, JSON_Int32);
#else
        o = (JSONNumber *) ValueI(val).pval;
#endif
    }
    return (JSON) o;
}

JSON JSONBool_new(int val)
{
    JSONNumber *o;
#ifndef USE_NUMBOX
    o = JSON_NEW(Bool);
    o->val = val;
    JSON_set_type(o, JSON_Bool);
#else
    o = (JSONNumber *) ValueB(val).pval;
#endif
    return (JSON) o;
}

static void JSONObject_free(JSON json)
{
    JSONObject *o = toJSONObject(json);
#ifdef USE_NUMBOX
    o = toJSONObject(toObj(toVal((JSON)o)));
#endif
    kmap_dispose(&o->child);
    free(o);
}

static void JSONString_free(JSON json)
{
    JSONString *o = toJSONString(json);
#ifdef USE_NUMBOX
    o = toJSONString(toStr(toVal((JSON)o)));
#endif
    free(o);
}

static void JSONArray_free(JSON o)
{
    JSONArray *a = toJSONArray(o);
    JSON *s, *e;
    JSON_ARRAY_EACH(a, s, e) {
        _JSON_free(*s);
    }
    free(a->list);
    free(a);
}

static void JSONInt64_free(JSON json)
{
    JSONInt64 *o = (JSONInt64*) (json);
#ifdef USE_NUMBOX
    o = (JSONInt64*) toInt64(toVal((JSON)o));
#endif
    free(o);
}
static void JSONNOP_free(JSON o) {}
static void _JSON_free(JSON o)
{
#ifdef USE_NUMBOX
    kjson_type type = JSON_type(o);
    typedef void (*freeJSON)(JSON);
    static const freeJSON dispatch_free[] = {
        /* 00 */JSONNOP_free,
        /* 01 */JSONString_free,
        /* 02 */JSONNOP_free,
        /* 03 */JSONObject_free,
        /* 04 */JSONNOP_free,
        /* 05 */JSONArray_free,
        /* 06 */JSONNOP_free,
        /* 07 */JSONNOP_free,
        /* 08 */JSONNOP_free,
        /* 09 */JSONString_free,
        /* 10 */JSONNOP_free,
        /* 11 */JSONInt64_free,
        /* 12 */JSONNOP_free,
        /* 13 */JSONNOP_free,
        /* 14 */JSONNOP_free,
        /* 15 */JSONNOP_free};
    dispatch_free[type](o);
#else
    switch (JSON_type(o)) {
        case JSON_Object:
            JSONObject_free(o);
            break;
        case JSON_String:
        case JSON_UString:
            JSONString_free(o);
            break;
        case JSON_Array:
            JSONArray_free(o);
            break;
        case JSON_Int64:
            JSONInt64_free(o);
            break;
        default:
            break;
    }
#endif
}
void JSON_free(JSON o) {
    _JSON_free(o);
}

void JSONArray_append(JSONArray *a, JSON o)
{
#ifdef USE_NUMBOX
    a = toJSONArray(toAry(toVal((JSON)a)));
#endif
    if (a->length + 1 >= a->capacity) {
        uint32_t newsize = 1 << SizeToKlass(a->capacity * 2 + 1);
        a->list = (JSON*) realloc(a->list, newsize * sizeof(JSON));
        a->capacity = newsize;
    }
    a->list[a->length++] = o;
}

static void _JSONObject_set(JSONObject *o, JSONString *key, JSON value)
{
#ifdef USE_NUMBOX
    o = toJSONObject(toObj(toVal((JSON)o)));
#endif
    assert(key && value);
    assert(JSON_type(value) < 16);
    kmap_set(&o->child, (char *) key, 0, value);
}

void JSONObject_set(JSONObject *o, JSON key, JSON value)
{
    assert(JSON_TYPE_CHECK(Object, o));
    assert(JSON_TYPE_CHECK(String, key));
    _JSONObject_set(o, toJSONString(key), value);
}

/* Parser functions */
#define NEXT(ins) string_input_stream_next(ins)
#define EOS(ins)  string_input_stream_eos(ins)
static JSON parseNull(input_stream *ins, unsigned char c);
static JSON parseNumber(input_stream *ins, unsigned char c);
static JSON parseBoolean(input_stream *ins, unsigned char c);
static JSON parseObject(input_stream *ins, unsigned char c);
static JSON parseArray(input_stream *ins, unsigned char c);
static JSON parseString(input_stream *ins, unsigned char c);

#define _N 0x40 |
#define _M 0x80 |
static const unsigned string_table[] = {
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,_N 0,_N 0,0   ,0   ,_N 0,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    _N 0,0   ,_M 2,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,4   ,0   ,0,
    4   ,4   ,4   ,4   ,4   ,4   ,4   ,4   ,4   ,4   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,3   ,_M 0,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,5   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,6   ,0,
    0   ,0   ,0   ,0   ,5   ,0   ,0   ,0   ,0   ,0   ,0   ,1   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0,
    0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0
};
#undef _M

static unsigned char skip_space(input_stream *ins, unsigned char c)
{
    int ch = c;
    for (ch = !ch?NEXT(ins):ch; EOS(ins); ch = NEXT(ins)) {
        assert(ch >= 0);
        if (!(0x40 & string_table[ch])) {
            return (unsigned char) ch;
        }
    }
    return 0;
}

static unsigned char skipBSorDoubleQuote(input_stream *ins, unsigned char c)
{
    register unsigned ch = c;
    register unsigned char *      str = (unsigned char *) ins->d0.str;
    register unsigned char *const end = (unsigned char *) ins->d1.str;
    for(; str != end; ch = *str++) {
        if (0x80 & string_table[ch]) {
            break;
        }
    }
    ins->d0.str = str;
    return ch;
}

static JSON parseNOP(input_stream *ins, unsigned char c) { return NULL; }

static JSON parseChild(input_stream *ins, unsigned char c)
{
    c = skip_space(ins, c);
    typedef JSON (*parseJSON)(input_stream *ins, unsigned char c);
    static const parseJSON dispatch_func[] = {
        parseNOP,
        parseObject,
        parseString,
        parseArray,
        parseNumber,
        parseBoolean,
        parseNull};
    return dispatch_func[0x7 & string_table[(int)c]](ins, c);
}

static unsigned int toHex(unsigned char c)
{
    return (c >= '0' && c <= '9') ? c - '0' :
        (c >= 'a' && c <= 'f') ? c - 'a' + 10:
        (c >= 'A' && c <= 'F') ? c - 'A' + 10:
        (assert(0 && "invalid hex digit"), 0);
}

static void writeUnicode(unsigned int data, string_builder *sb)
{
    if (data <= 0x7f) {
        string_builder_add(sb, (char)data);
    } else if (data <= 0x7ff) {
        string_builder_add(sb, (char)(0xc0 | (data >> 6)));
        string_builder_add(sb, (char)(0x80 | (0x3f & data)));
    } else if (data <= 0xffff) {
        string_builder_add(sb, (char)(0xe0 | (data >> 12)));
        string_builder_add(sb, (char)(0x80 | (0x3f & (data >> 6))));
        string_builder_add(sb, (char)(0x80 | (0x3f & data)));
    } else if (data <= 0x10FFFF) {
        string_builder_add(sb, (char)(0xf0 | (data >> 18)));
        string_builder_add(sb, (char)(0x80 | (0x3f & (data >> 12))));
        string_builder_add(sb, (char)(0x80 | (0x3f & (data >>  6))));
        string_builder_add(sb, (char)(0x80 | (0x3f & data)));
    }
}

static void parseUnicode(input_stream *ins, string_builder *sb)
{
    unsigned int data = 0;
    data  = toHex(NEXT(ins)) * 4096; assert(EOS(ins));
    data += toHex(NEXT(ins)) *  256; assert(EOS(ins));
    data += toHex(NEXT(ins)) *   16; assert(EOS(ins));
    data += toHex(NEXT(ins)) *    1; assert(EOS(ins));
    writeUnicode(data, sb);
}

static void parseEscape(input_stream *ins, string_builder *sb, unsigned char c)
{
    switch (c) {
        case '"':  c = '"';  break;
        case '\\': c = '\\'; break;
        case '/': c = '/';  break;
        case 'b': c = '\b';  break;
        case 'f': c = '\f';  break;
        case 'n': c = '\n';  break;
        case 'r': c = '\r';  break;
        case 't': c = '\t';  break;
        case 'u': parseUnicode(ins, sb); return;
        default: assert(0 && "Unknown espace");
    }
    string_builder_add(sb, c);
}

static JSON parseString(input_stream *ins, unsigned char c)
{
    union io_data state, state2;
    assert(c == '"' && "Missing open quote at start of JSONString");
    state = _input_stream_save(ins);
    c = skipBSorDoubleQuote(ins, NEXT(ins));
    state2 = _input_stream_save(ins);
    if (c == '"') {/* fast path */
        return (JSON) JSONString_new((char *)state.str,
                state2.str - state.str - 1);
    }
    string_builder sb; string_builder_init(&sb);
    if (state2.str - state.str - 1 > 0) {
        string_builder_add_string(&sb, (const char *) state.str,
                state2.str - state.str - 1);
    }
    assert(c == '\\');
    goto L_escape;
    for(; EOS(ins); c = NEXT(ins)) {
        switch (c) {
            case '\\':
            L_escape:;
                parseEscape(ins, &sb, NEXT(ins));
                continue;
            case '"':
                goto L_end;
            default:
                break;
        }
        string_builder_add(&sb, c);
    }
    L_end:;
    return (JSON)JSONString_new2(&sb);
}

static JSON parseObject(input_stream *ins, unsigned char c)
{
    assert(c == '{' && "Missing open brace '{' at start of json object");
    JSON json = JSONObject_new();
    for (c = skip_space(ins, 0); EOS(ins); c = skip_space(ins, 0)) {
        JSONString *key = NULL;
        JSON val = NULL;
        if (c == '}') {
            break;
        }
        assert(c == '"' && "Missing open quote for element key");
        key = (JSONString *) parseString(ins, c);
        c = skip_space(ins, 0);
        assert(c == ':' && "Missing ':' after key in object");
        val = parseChild(ins, 0);
        _JSONObject_set(toJSONObject(json), key, val);
        c = skip_space(ins, 0);
        if (c == '}') {
            break;
        }
        assert(c == ',' && "Missing comma or end of JSON Object '}'");
    }
    return json;
}

static JSON parseArray(input_stream *ins, unsigned char c)
{
    JSON json = JSONArray_new();
    JSONArray *a = toJSONArray(json);
    assert(c == '[' && "Missing open brace '[' at start of json array");
    c = skip_space(ins, 0);
    if (c == ']') {
        /* array with no elements "[]" */
        return json;
    }
    for (; EOS(ins); c = skip_space(ins, 0)) {
        JSON val = parseChild(ins, c);
        JSONArray_append(a, val);
        c = skip_space(ins, 0);
        if (c == ']') {
            break;
        }
        assert(c == ',' && "Missing comma or end of JSON Array ']'");
    }
    return json;
}

static JSON parseBoolean(input_stream *ins, unsigned char c)
{
    int val = 0;
    if (c == 't') {
        if(NEXT(ins) == 'r' && NEXT(ins) == 'u' && NEXT(ins) == 'e') {
            val = 1;
        }
    }
    else if (c == 'f') {
        if (NEXT(ins) == 'a' && NEXT(ins) == 'l' &&
                NEXT(ins) == 's' && NEXT(ins) == 'e') {
        }
    }
    else {
        assert(0 && "Cannot parse JSON bool variable");
    }
    return JSONBool_new(val);
}

static JSON parseNumber(input_stream *ins, unsigned char c)
{
    assert((c == '-' || ('0' <= c && c <= '9')) && "It do not seem as Number");
    kjson_type type = JSON_Int32;
    union io_data state, state2;
    state = _input_stream_save(ins);
    bool negative = false;
    int64_t val = 0;
    JSON n;
    if (c == '-') { negative = true; c = NEXT(ins); }
    if (c == '0') { c = NEXT(ins); }
    else if ('1' <= c && c <= '9') {
        for (; '0' <= c && c <= '9' && EOS(ins); c = NEXT(ins)) {
            val = val * 10 + (c - '0');
        }
    }
    if (c != '.' && c != 'e' && c != 'E') {
        goto L_emit;
    }
    if (c == '.') {
        type = JSON_Double;
        for (c = NEXT(ins); '0' <= c && c <= '9' &&
                EOS(ins); c = NEXT(ins)) {}
    }
    if (c == 'e' || c == 'E') {
        type = JSON_Double;
        c = NEXT(ins);
        if (c == '+' || c == '-') {
            c = NEXT(ins);
        }
        for (; '0' <= c && c <= '9' && EOS(ins); c = NEXT(ins)) {}
    }
    L_emit:;
    state2 = _input_stream_save(ins);
    state2.str -= 1;
    _input_stream_resume(ins, state2);
    if (type != JSON_Double) {
        val = (negative)? -val : val;
        n = JSONInt_new(val);
    } else {
        char *s = (char *)state.str-1;
        char *e = (char *)state2.str;
        double d = strtod(s, &e);
        n = JSONDouble_new(d);
    }
    return n;
}

static JSON parseNull(input_stream *ins, unsigned char c)
{
    if (c == 'n') {
        if(NEXT(ins) == 'u' && NEXT(ins) == 'l' && NEXT(ins) == 'l') {
            return JSONNull_new();
        }
    }
    assert(0 && "Cannot parse JSON null variable");
    return NULL;
}

static JSON parse(input_stream *ins)
{
    unsigned char c = 0;
    for_each_istream(ins, c) {
        JSON json;
        if ((c = skip_space(ins, c)) == 0) {
            break;
        }
        if ((json = parseChild(ins, c)) != NULL)
            return json;
    }
    return NULL;
}

#undef EOS
#undef NEXT

/* [Dump functions] */
static void JSONString_dump(FILE *fp, JSONString *json)
{
#ifdef USE_NUMBOX
    json = toJSONString(toStr(toVal((JSON)json)));
#endif
    fprintf(stderr, "\"%s\"", json->str);
}

static void JSONUString_dump(FILE *fp, JSONString *json)
{
    JSONString_dump(fp, json);
}

static void JSONNull_dump(FILE *fp, JSONNull *json)
{
    fputs("null", stderr);
}

static void JSONBool_dump(FILE *fp, JSONNumber *json)
{
#ifdef USE_NUMBOX
    fprintf(stderr, "%s", toBool(toVal(json))?"ture":"false");
#else
    fprintf(stderr, "%s", json->val?"ture":"false");
#endif
}

static void JSONArray_dump(FILE *fp, JSONArray *a)
{
    JSON *s, *e;
    fputs("[", stderr);
    JSON_ARRAY_EACH(a, s, e) {
        JSON_dump(fp, *s);
        fputs(",", stderr);
    }
    fputs("]", stderr);
}

static void JSONInt32_dump(FILE *fp, JSONNumber *json)
{
#ifdef USE_NUMBOX
    fprintf(stderr, "%d", toInt32(toVal(json)));
#else
    fprintf(stderr, "%d", (int)json->val);
#endif
}

static void JSONInt64_dump(FILE *fp, JSONInt64 *json)
{
#ifdef USE_NUMBOX
    JSONInt64 *I = (JSONInt64*)toInt64(toVal((JSON)json));
    fprintf(stderr, "%" PRIi64, I->val);
#else
    fprintf(stderr, "%" PRIi64, json->val);
#endif
}

static void JSONDouble_dump(FILE *fp, JSONNumber *json)
{
#ifdef USE_NUMBOX
    fprintf(stderr, "%g", toDouble(toVal(json)));
#else
    union v { double f; long v; } v;
    v.v = json->val;
    fprintf(stderr, "%g", v.f);
#endif
}

static void JSONObject_dump(FILE *fp, JSONObject *o)
{
    map_record_t *r;
    kmap_iterator itr = {0};
    fputs("{", fp);
#ifdef USE_NUMBOX
    o = toJSONObject(toObj(toVal((JSON)o)));
#endif
    while ((r = kmap_next(&o->child, &itr)) != NULL) {
        fputs("", fp);
        JSONString_dump(fp, (JSONString*)r->k);
        fputs(" : ", fp);
        JSON_dump(fp, (JSON)r->v);
        fputs(",", fp);
    }
    fputs("}", fp);
}

void JSON_dump(FILE *fp, JSON json)
{
#ifdef USE_NUMBOX
    if (IsDouble((toVal(json)))) {
        JSONDouble_dump(fp, (JSONDouble*)json);
        return;
    }
#endif
    switch (JSON_type(json)) {
#define CASE(T, O) case JSON_##T: JSON##T##_dump(fp, (JSON##T *) O); break
        CASE(Object, json);
        CASE(Array, json);
        CASE(String, json);
        CASE(Int32, json);
        CASE(Int64, json);
        CASE(Double, json);
        CASE(Bool, json);
        CASE(Null, json);
        CASE(UString, json);
        default:
            assert(0 && "NO dump func");
#undef CASE
    }
}

static JSON parseJSON_stream(input_stream *ins)
{
    return parse(ins);
}

JSON parseJSON(const char *s, const char *e)
{
    input_stream *ins = new_string_input_stream(s, e - s, 0);
    JSON json = parseJSON_stream(ins);
    input_stream_delete(ins);
    return json;
}

static JSON _JSON_get(JSON json, const char *key)
{
    JSONObject *o = toJSONObject(json);
    size_t len = strlen(key);

#ifdef USE_NUMBOX
    struct _string tmp;
    tmp.str = (char *)key;
    tmp.len = len;
    o = toJSONObject(toObj(toVal((JSON)o)));
    map_record_t *r = kmap_get(&o->child, (char *)&tmp, 0);
#else
    char tmp[sizeof(union JSONValue) + len];
    JSONString *s = (JSONString *) tmp;
    s->length = len;
    memcpy(s->str, key, len);
    assert(JSON_type(json) == JSON_Object);
    JSON_set_type((JSON) s, JSON_String);
    map_record_t *r = kmap_get(o->child, (char *)s, 0);
#endif
    return (JSON) r->v;
}

JSON JSON_get(JSON json, const char *key)
{
    return _JSON_get(json, key);
}

int JSON_getInt(JSON json, const char *key)
{
    JSON v = _JSON_get(json, key);
#ifdef USE_NUMBOX
    return toInt32(toVal(v));
#else
    return ((JSONInt*)v)->val;
#endif
}

int JSON_getBool(JSON json, const char *key)
{
    JSON v = _JSON_get(json, key);
#ifdef USE_NUMBOX
    return toBool(toVal(v));
#else
    return ((JSONBool*)v)->val;
#endif

}

double JSON_getDouble(JSON json, const char *key)
{
    JSON v = _JSON_get(json, key);
#ifdef USE_NUMBOX
    return toDouble(toVal(v));
#else
    union v { double f; long v; } val;
    val.v = toJSONDouble(v)->val;
    return val.f;
#endif
}

const char *JSON_getString(JSON json, const char *key, size_t *len)
{
    JSONString *s = (JSONString *) _JSON_get(json, key);
#ifdef USE_NUMBOX
    s = (JSONString *) toStr(toVal((void*)s));
#endif
    *len = s->length;
    return s->str;
}

JSON *JSON_getArray(JSON json, const char *key, size_t *len)
{
    JSONArray *a = (JSONArray *) _JSON_get(json, key);
#ifdef USE_NUMBOX
    a = toJSONArray(toAry(toVal((JSON)a)));
#endif
    *len = a->length;
    return a->list;
}

unsigned JSON_length(JSON json)
{
    JSONArray *a = toJSONArray(json);
    assert((JSON_type(json) & 0x3) == 0x1);
#ifdef USE_NUMBOX
    a = toJSONArray(toAry(toVal((JSON)a)));
#endif
    return a->length;
}

int JSONObject_iterator_init(JSONObject_iterator *itr, JSONObject *obj)
{
    JSON json = (JSON)obj;
    if (!JSON_type(json) ==  JSON_Object)
        return 0;
#ifdef USE_NUMBOX
    itr->obj = toJSONObject(toObj(toVal(obj)));
#else
    itr->obj = obj;
#endif
    itr->index = 0;
    return 1;
}

JSONString *JSONObject_iterator_next(JSONObject_iterator *itr, JSON *val)
{
    JSONObject *o = itr->obj;
    map_record_t *r;
    while ((r = kmap_next(&o->child, (kmap_iterator*) itr)) != NULL) {
        *val = (JSON)r->v;
        return (JSONString*)r->k;
    }
    *val = NULL;
    return NULL;
}

static void JSONString_toString(string_builder *sb, JSON json);
static void _JSON_toString(string_builder *sb, JSON json);

static void JSONObject_toString(string_builder *sb, JSON json)
{
    JSONObject *o = toJSONObject(json);
    map_record_t *r;
    kmap_iterator itr = {0};
    string_builder_add(sb, '{');
#ifdef USE_NUMBOX
    o = toJSONObject(toObj(toVal((JSON)o)));
#endif
    if ((r = kmap_next(&o->child, &itr)) != NULL) {
        goto L_internal;
        while ((r = kmap_next(&o->child, &itr)) != NULL) {
            string_builder_add(sb, ',');
            L_internal:
            JSONString_toString(sb, (JSON)r->k);
            string_builder_add(sb, ':');
            _JSON_toString(sb, (JSON)r->v);
        }
    }
    string_builder_add(sb, '}');
}

static void JSONArray_toString(string_builder *sb, JSON json)
{
    JSONArray *a = toJSONArray(json);
    JSON *s, *e;
    string_builder_add(sb, '[');
#ifdef USE_NUMBOX
    a = toJSONArray(toAry(toVal((JSON)a)));
#endif
    s = (a)->list;
    e = (a)->list+(a)->length;
    if (s < e)
        goto L_internal;
    for (; s < e; ++s) {
        string_builder_add(sb, ',');
        L_internal:
        _JSON_toString(sb, *s);
    }
    string_builder_add(sb, ']');
}

static void JSONString_toString(string_builder *sb, JSON json)
{
    JSONString *o = toJSONString(json);
#ifdef USE_NUMBOX
    o = toJSONString(toStr(toVal((JSON)o)));
#endif
    string_builder_add(sb, '"');
    string_builder_add_string(sb, o->str, o->length);
    string_builder_add(sb, '"');
}

static int utf8_check_size(unsigned char s)
{
    uint8_t u = (uint8_t) s;
    assert (u >= 0x80);
    if (0xc2 <= u && u <= 0xdf)
        return 2;
    else if (0xe0 <= u && u <= 0xef)
        return 3;
    else if (0xf0 <= u && u <= 0xf4)
        return 4;
    //assert(0 && "Invalid encoding");
    return 0;
}

static char *toUTF8(string_builder *sb, char *s, char *e)
{
    uint32_t v = 0;
    int i, length = utf8_check_size((unsigned char) (*s));
    if (length == 2) v = *s++ & 0x1f;
    else if (length == 3) v = *s++ & 0xf;
    else if (length == 4) v = *s++ & 0x7;
    for (i = 1; i < length && s < e; ++i) {
        uint8_t tmp = (uint8_t) *s++;
        if (tmp < 0x80 || tmp > 0xbf) {
            return 0;
        }
        v = (v << 6) | (tmp & 0x3f);
    }
    string_builder_add_hex(sb, v);
    return s;
}

static void JSONUString_toString(string_builder *sb, JSON json)
{
    JSONString *o = toJSONString(json);
#ifdef USE_NUMBOX
    o = toJSONString(toStr(toVal((JSON)o)));
#endif
    string_builder_add(sb, '"');
    char *s = o->str;
    char *e = o->str + o->length;
    while (s < e) {
        unsigned char c;
        if (*s & 0x80) {
            string_builder_add_string(sb, "\\u", 2);
            s = toUTF8(sb, s, e);
            continue;
        }
        c = *s++;
        switch (c) {
            case '"':  string_builder_add_string(sb, "\\\"", 2); break;
            case '\\': string_builder_add_string(sb, "\\\\", 2); break;
            case '/':  string_builder_add_string(sb, "\\/" , 2); break;
            case '\b': string_builder_add_string(sb, "\\b", 2); break;
            case '\f': string_builder_add_string(sb, "\\f", 2); break;
            case '\n': string_builder_add_string(sb, "\\n", 2); break;
            case '\r': string_builder_add_string(sb, "\\r", 2); break;
            case '\t': string_builder_add_string(sb, "\\t", 2); break;
            default:
                string_builder_add(sb, c);
        }
    }
    string_builder_add(sb, '"');
}

static void JSONInt32_toString(string_builder *sb, JSON json)
{
    JSONInt *o = (JSONInt *) json;
    int32_t i;
#ifdef USE_NUMBOX
    i = toInt32(toVal(o));
#else
    i = (int)o->val;
#endif
    string_builder_add_int(sb, i);
}
static void JSONInt64_toString(string_builder *sb, JSON json)
{
    JSONInt64 *o = (JSONInt64 *) json;
    int64_t i;
#ifdef USE_NUMBOX
    o = ((JSONInt64 *) toInt64(toVal(o)));
#endif
    i = o->val;
    string_builder_add_int(sb, i);
}

static void JSONDouble_toString(string_builder *sb, JSON json)
{
    JSONDouble *o = (JSONDouble *) json;
    char buf[64];
    double d;
#ifdef USE_NUMBOX
    d = toDouble(toVal(o));
#else
    union v { double d; long v; } v;
    v.v = o->val;
    d = v.d;
#endif
    int len = snprintf(buf, 64, "%g", d);
    string_builder_add_string(sb, buf, len);
}

static void JSONBool_toString(string_builder *sb, JSON json)
{
    JSONBool *o = (JSONBool *) json;
    int b;
#ifdef USE_NUMBOX
    b = toBool(toVal(o));
#else
    b = o->val;
#endif
    const char *str  = (b) ? "true" : "false";
    size_t len = (b) ? 4/*strlen("ture")*/ : 5/*strlen("false")*/;
    string_builder_add_string(sb, str, len);
}

static void JSONNull_toString(string_builder *sb, JSON json)
{
    string_builder_add_string(sb, "null", 4);
}

static void JSONNOP_toString(string_builder *sb, JSON json)
{
    assert(0 && "Invalid type");
}

static void _JSON_toString(string_builder *sb, JSON json)
{
#ifdef USE_NUMBOX
    kjson_type type = JSON_type(json);
    typedef void (*toString)(string_builder *sb, JSON);
    static const toString dispatch_toStr[] = {
        /* 00 */JSONDouble_toString,
        /* 01 */JSONString_toString,
        /* 02 */JSONInt32_toString,
        /* 03 */JSONObject_toString,
        /* 04 */JSONBool_toString,
        /* 05 */JSONArray_toString,
        /* 06 */JSONNull_toString,
        /* 07 */JSONDouble_toString,
        /* 08 */JSONNOP_toString,
        /* 09 */JSONUString_toString,
        /* 10 */JSONNOP_toString,
        /* 11 */JSONInt64_toString,
        /* 12 */JSONNOP_toString,
        /* 13 */JSONNOP_toString,
        /* 14 */JSONNOP_toString,
        /* 15 */JSONNOP_toString};
    dispatch_toStr[type](sb, json);
#else
    switch (JSON_type(json)) {
#define CASE(T, SB, O) case JSON_##T: JSON##T##_toString(SB, (JSON##T *) O); break
        CASE(Object, sb, json);
        CASE(Array, sb, json);
        CASE(String, sb, json);
        CASE(Int32, sb, json);
        CASE(Int64, sb, json);
        CASE(Double, sb, json);
        CASE(Bool, sb, json);
        CASE(Null, sb, json);
        CASE(UString, sb, json);
        default:
#ifdef USE_NUMBOX
            if (IsDouble((toVal(json)))) {
                JSONDouble_toString(sb, (JSONDouble*)json);
                return;
            }
#endif
            assert(0 && "NO tostring func");
#undef CASE
    }
#endif
}

char *JSON_toString(JSON json, size_t *len)
{
    string_builder sb; string_builder_init(&sb);
    _JSON_toString(&sb, json);
    size_t length;
    char *str = string_builder_tostring(&sb, &length, 1);
    if (len) {
        *len = length;
    }
    return str;
}

void JSON_dump_(JSON s)
{
    JSON_dump(stderr, s);
}

#ifdef __cplusplus
}
#endif
