/****************************************************************************
 * Copyright (c) 2012, Masahiro Ide <ide@konohascript.org>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#include "kjson.h"
#include "kstream.h"
#include "kstring_builder.h"

#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

static JSON JSONString_new2(string_builder *builder)
{
    size_t len;
    char *s = string_builder_tostring(builder, &len, 1);
    len -= 1;
    JSONString *o = (JSONString *) KJSON_CALLOC(1, sizeof(*o) + len);
    o->str = (const char *) (o+1);
    memcpy((char *) o->str, s, len);
    o->hashcode = 0;
    o->length = len;
    KJSON_FREE(s);
    return toJSON(ValueU(o));
}

static void _JSON_free(JSON o);
static void JSONNOP_free(JSON o) {}
void JSON_free(JSON o)
{
    _JSON_free(o);
}

static void JSONObject_free(JSON json)
{
    JSONObject *o = toObj(json.val);
    kmap_dispose(&o->child);
    KJSON_FREE(o);
}

static void _JSONString_free(JSONString *obj)
{
    KJSON_FREE(obj);
}

static void JSONString_free(JSON json)
{
    JSONString *o = toStr(json.val);
    _JSONString_free(o);
}

static void JSONArray_free(JSON json)
{
    JSONArray *a;
    JSON *s, *e;
    JSON_ARRAY_EACH_(json, a, s, e, 0) {
        _JSON_free(*s);
    }

    KJSON_FREE(a->list);
    KJSON_FREE(a);
}

static void JSONInt64_free(JSON json)
{
    JSONInt64 *o = toInt64(json.val);
    KJSON_FREE(o);
}

#define JSON_OP(OP)\
    OP(/* 00 */JSONDouble)\
    OP(/* 01 */JSONString)\
    OP(/* 02 */JSONInt32)\
    OP(/* 03 */JSONObject)\
    OP(/* 04 */JSONBool)\
    OP(/* 05 */JSONArray)\
    OP(/* 06 */JSONNull)\
    OP(/* 07 */JSONDouble)\
    OP(/* 08 */JSONNOP)\
    OP(/* 09 */JSONUString)\
    OP(/* 10 */JSONNOP)\
    OP(/* 11 */JSONInt64)\
    OP(/* 12 */JSONNOP)\
    OP(/* 13 */JSONNOP)\
    OP(/* 14 */JSONNOP)\
    OP(/* 15 */JSONNOP)

#define JSONDouble_free  JSONNOP_free
#define JSONInt32_free   JSONNOP_free
#define JSONBool_free    JSONNOP_free
#define JSONNull_free    JSONNOP_free
#define JSONUString_free JSONString_free
static void _JSON_free(JSON o)
{
    kjson_type type = JSON_type(o);
    typedef void (*freeJSON)(JSON);
    static const freeJSON dispatch_free[] = {
#define JSON_FREE(T) T##_free,
        JSON_OP(JSON_FREE)
#undef JSON_FREE
    };
    dispatch_free[type](o);
}

static void _JSONArray_append(JSONArray *a, JSON o)
{
    if (a->length + 1 >= a->capacity) {
        uint32_t newsize = 1 << LOG2(a->capacity * 2 + 1);
        a->list = (JSON*) KJSON_REALLOC(a->list, newsize * sizeof(JSON));
        a->capacity = newsize;
    }
    a->list[a->length++] = o;
}

void JSONArray_append(JSON json, JSON o)
{
    JSONArray *a = toAry(json.val);
    _JSONArray_append(a, o);
}

static void _JSONObject_set(JSONObject *o, JSONString *key, JSON value)
{
    assert(JSON_type(value) < 16);
    kmap_set(&o->child, key, value.bits);
}

void JSONObject_set(JSON json, JSON key, JSON value)
{
    assert(JSON_TYPE_CHECK(Object, json));
    assert(JSON_TYPE_CHECK(String, key));
    JSONObject *o = toObj(json.val);
    _JSONObject_set(o, toStr(key.val), value);
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
#undef _N

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

static JSON parseNOP(input_stream *ins, unsigned char c)
{
    JSON o; o.bits = 0;
    return o;
}

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
        return JSONString_new((char *)state.str, state2.str - state.str - 1);
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
    return JSONString_new2(&sb);
}

static JSON parseObject(input_stream *ins, unsigned char c)
{
    assert(c == '{' && "Missing open brace '{' at start of json object");
    JSON json = JSONObject_new();
    JSONObject *obj = toObj(json.val);
    for (c = skip_space(ins, 0); EOS(ins); c = skip_space(ins, 0)) {
        JSONString *key = NULL;
        JSON val;
        if (c == '}') {
            break;
        }
        assert(c == '"' && "Missing open quote for element key");
        key = toStr(parseString(ins, c).val);
        c = skip_space(ins, 0);
        assert(c == ':' && "Missing ':' after key in object");
        val = parseChild(ins, 0);
        _JSONObject_set(obj, key, val);
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
    JSONArray *a = toAry(json.val);
    assert(c == '[' && "Missing open brace '[' at start of json array");
    c = skip_space(ins, 0);
    if (c == ']') {
        /* array with no elements "[]" */
        return json;
    }
    for (; EOS(ins); c = skip_space(ins, 0)) {
        JSON val = parseChild(ins, c);
        _JSONArray_append(a, val);
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
        if (NEXT(ins) == 'r' && NEXT(ins) == 'u' && NEXT(ins) == 'e') {
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
        if (NEXT(ins) == 'u' && NEXT(ins) == 'l' && NEXT(ins) == 'l') {
            return JSONNull_new();
        }
    }
    assert(0 && "Cannot parse JSON null variable");
    JSON o; o.bits = 0;
    return o;
}

static JSON parse(input_stream *ins)
{
    unsigned char c = 0;
    for_each_istream(ins, c) {
        JSON json;
        if ((c = skip_space(ins, c)) == 0) {
            break;
        }
        json = parseChild(ins, c);
        if (json.obj != NULL)
            return json;
    }
    JSON o; o.bits = 0;
    return o;
}

#undef EOS
#undef NEXT

static const JSON JSON_default = {{0}};

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
    JSONObject *o = toObj(json.val);
    size_t len = strlen(key);

    struct JSONString tmp;
    tmp.str = (char *)key;
    tmp.length = len;
    tmp.hashcode = 0;
    map_record_t *r = kmap_get(&o->child, &tmp);
    return (r) ? toJSON(ValueP(r->v)) : JSON_default;
}

JSON JSON_get(JSON json, const char *key)
{
    return _JSON_get(json, key);
}

int JSON_getInt(JSON json, const char *key)
{
    JSON v = _JSON_get(json, key);
    return toInt32(v.val);
}

bool JSON_getBool(JSON json, const char *key)
{
    JSON v = _JSON_get(json, key);
    return toBool(v.val);
}

double JSON_getDouble(JSON json, const char *key)
{
    JSON v = _JSON_get(json, key);
    return toDouble(v.val);
}

const char *JSON_getString(JSON json, const char *key, size_t *len)
{
    JSON obj = _JSON_get(json, key);
    JSONString *s = toStr(obj.val);
    *len = s->length;
    return s->str;
}

JSON *JSON_getArray(JSON json, const char *key, size_t *len)
{
    JSON obj = _JSON_get(json, key);
    JSONArray *a = toAry(obj.val);
    *len = a->length;
    return a->list;
}

static void _JSONString_toString(string_builder *sb, JSONString *o)
{
    string_builder_add(sb, '"');
    string_builder_add_string(sb, o->str, o->length);
    string_builder_add(sb, '"');
}

static void _JSON_toString(string_builder *sb, JSON json);

static void JSONObject_toString(string_builder *sb, JSON json)
{
    map_record_t *r;
    kmap_iterator itr = {0};
    string_builder_add(sb, '{');
    JSONObject *o = toObj(json.val);
    if ((r = kmap_next(&o->child, &itr)) != NULL) {
        goto L_internal;
        while ((r = kmap_next(&o->child, &itr)) != NULL) {
            string_builder_add(sb, ',');
            L_internal:
            _JSONString_toString(sb, r->k);
            string_builder_add(sb, ':');
            _JSON_toString(sb, toJSON(ValueP(r->v)));
        }
    }
    string_builder_add(sb, '}');
}

static void JSONArray_toString(string_builder *sb, JSON json)
{
    JSON *s, *e;
    JSONArray *a = toAry(json.val);
    string_builder_add(sb, '[');
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
    JSONString *o = toStr(json.val);
    _JSONString_toString(sb, o);
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

static const char *toUTF8(string_builder *sb, const char *s, const char *e)
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
    string_builder_add_hex_no_check(sb, v);
    return s;
}

static void JSONUString_toString(string_builder *sb, JSON json)
{
    JSONString *o = toStr(json.val);
    const char *s = o->str;
    const char *e = o->str + o->length;
    string_builder_ensure_size(sb, o->length+2/* = strlen("\"\") */);
    string_builder_add(sb, '"');
    while (s < e) {
        unsigned char c = *s;
        string_builder_ensure_size(sb, 8);
        if (c & 0x80) {
            string_builder_add_string_no_check(sb, "\\u", 2);
            s = toUTF8(sb, s, e);
            continue;
        }
        c = *s++;
        switch (c) {
            case '"':  string_builder_add_string_no_check(sb, "\\\"", 2); break;
            case '\\': string_builder_add_string_no_check(sb, "\\\\", 2); break;
            case '/':  string_builder_add_string_no_check(sb, "\\/" , 2); break;
            case '\b': string_builder_add_string_no_check(sb, "\\b", 2); break;
            case '\f': string_builder_add_string_no_check(sb, "\\f", 2); break;
            case '\n': string_builder_add_string_no_check(sb, "\\n", 2); break;
            case '\r': string_builder_add_string_no_check(sb, "\\r", 2); break;
            case '\t': string_builder_add_string_no_check(sb, "\\t", 2); break;
            default:   string_builder_add_no_check(sb, c);
        }
    }
    string_builder_add_no_check(sb, '"');
}

static void JSONInt32_toString(string_builder *sb, JSON json)
{
    int32_t i = toInt32(json.val);
    string_builder_add_int(sb, i);
}
static void JSONInt64_toString(string_builder *sb, JSON json)
{
    JSONInt64 *o = toInt64(json.val);
    string_builder_add_int(sb, o->val);
}

static void JSONDouble_toString(string_builder *sb, JSON json)
{
    double d = toDouble(json.val);
    char buf[64];
    int len = snprintf(buf, 64, "%g", d);
    string_builder_add_string(sb, buf, len);
}

static void JSONBool_toString(string_builder *sb, JSON json)
{
    int b = toBool(json.val);
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
    kjson_type type = JSON_type(json);
    typedef void (*toString)(string_builder *sb, JSON);
    static const toString dispatch_toStr[] = {
#define TOSTRING(T) T##_toString,
        JSON_OP(TOSTRING)
#undef TOSTRING
    };
    dispatch_toStr[type](sb, json);
}

char *JSON_toStringWithLength(JSON json, size_t *len)
{
    char *str;
    size_t length;
    string_builder sb;

    string_builder_init(&sb);
    _JSON_toString(&sb, json);
    str = string_builder_tostring(&sb, &length, 1);
    if (len) {
        *len = length;
    }
    return str;
}

/* [Dump functions] */
#ifdef KJSON_DEBUG_MODE
static void JSON_dump(FILE *fp, JSON json);

static void JSONString_dump(FILE *fp, JSON json)
{
    JSONString *str = toStr(json.val);
    fprintf(fp, "\"%s\"", str->str);
}

static void JSONUString_dump(FILE *fp, JSON json)
{
    JSONString_dump(fp, json);
}

static void JSONNull_dump(FILE *fp, JSON json)
{
    fputs("null", fp);
}

static void JSONBool_dump(FILE *fp, JSON json)
{
    fprintf(fp, "%s", toBool(json.val)?"ture":"false");
}

static void JSONArray_dump(FILE *fp, JSON json)
{
    JSONArray *a;
    JSON *s, *e;
    fputs("[", fp);
    JSON_ARRAY_EACH(json, a, s, e) {
        JSON_dump(fp, *s);
        fputs(",", fp);
    }
    fputs("]", fp);
}

static void JSONInt32_dump(FILE *fp, JSON json)
{
    fprintf(fp, "%d", toInt32(json.val));
}

static void JSONInt64_dump(FILE *fp, JSON json)
{
    JSONInt64 *i64 = toInt64(json.val);
    fprintf(fp, "%" PRIi64, i64->val);
}

static void JSONDouble_dump(FILE *fp, JSON json)
{
    fprintf(fp, "%g", toDouble(json.val));
}

static void JSONObject_dump(FILE *fp, JSON json)
{
    map_record_t *r;
    kmap_iterator itr = {0};
    fputs("{", fp);
    JSONObject *o = toObj(json.val);
    while ((r = kmap_next(&o->child, &itr)) != NULL) {
        fprintf(fp, "\"%s\" : ", r->k->str);
        JSON_dump(fp, (JSON)r->v);
        fputs(",", fp);
    }
    fputs("}", fp);
}

void JSON_dump(FILE *fp, JSON json)
{
    if (IsDouble((json.val))) {
        JSONDouble_dump(fp, json);
        return;
    }
    switch (JSON_type(json)) {
#define CASE(T, O) case JSON_##T: JSON##T##_dump(fp, O); break
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

#endif /* defined(KJSON_DEBUG_MODE) */

#ifdef __cplusplus
}
#endif

#include "kmap.c"
