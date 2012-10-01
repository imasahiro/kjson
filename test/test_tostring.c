#include "kjson.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void test_obj()
{
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    JSON o = JSONObject_new(&jm);
    JSONObject_set(&jm, o, JSONString_new(&jm, "a", 1), JSONBool_new(1));
    JSONObject_set(&jm, o, JSONString_new(&jm, "b", 1), JSONInt_new(&jm, 100));
    JSONObject_set(&jm, o, JSONString_new(&jm, "c", 1), JSONDouble_new(3.14));
    size_t len;
    char *s = JSON_toStringWithLength(o, &len);
    assert(strncmp(s, "{\"a\":true,\"b\":100,\"c\":3.14}", len) == 0);
    JSON_free(o);
    fprintf(stderr, "'%s'\n", s);
    free((char*)s);
    JSONMemoryPool_Delete(&jm);
}
static void test_array()
{
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    JSON a = JSONArray_new(&jm);
    JSONArray_append(&jm, a, (JSON) JSONString_new(&jm, "a", 1));
    JSONArray_append(&jm, a, (JSON) JSONString_new(&jm, "b", 1));
    JSONArray_append(&jm, a, (JSON) JSONString_new(&jm, "c", 1));
    size_t len;
    char *s = JSON_toStringWithLength(a, &len);
    assert(strncmp(s, "[\"a\",\"b\",\"c\"]", len) == 0);
    JSON_free(a);
    fprintf(stderr, "'%s'\n", s);
    free(s);
    JSONMemoryPool_Delete(&jm);

}
static void test_int()
{
    size_t len;
    char *s;
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    {
        JSON n = JSONInt_new(&jm, 100);
        s = JSON_toStringWithLength(n, &len);
        assert(strncmp(s, "100", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    {
        JSON n = JSONInt_new(&jm, INT32_MIN);
        s = JSON_toStringWithLength(n, &len);
        assert(strncmp(s, "-2147483648", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    {
        JSON n = JSONInt_new(&jm, INT32_MAX);
        s = JSON_toStringWithLength(n, &len);
        assert(strncmp(s, "2147483647", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    JSONMemoryPool_Delete(&jm);
}
static void test_double()
{
    size_t len;
    char *s;
    {
        JSON n = JSONDouble_new(M_PI);
        s = JSON_toStringWithLength(n, &len);
        char buf[128] = {};
        snprintf(buf, 128, "%g", M_PI);
        assert(strncmp(buf, buf, len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    {
        JSON n = JSONDouble_new(10.01);
        s = JSON_toStringWithLength(n, &len);
        assert(strncmp(s, "10.01", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
}
static void test_string()
{
    size_t len;
    char *s;
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
#define STRING(S) S, S+strlen(S)
    {
        JSON n = parseJSON(&jm, STRING("\"ABC\""));
        s = JSON_toStringWithLength(n, &len);
        assert(strncmp(s, "\"ABC\"", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    {
        JSON n = parseJSON(&jm, STRING("\"http:\\/\\/twitter.com\\/\""));
        s = JSON_toStringWithLength(n, &len);
        assert(strncmp(s, "\"http:\\/\\/twitter.com\\/\"", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    {
        JSON n = parseJSON(&jm, STRING("\"A\\nB\\r\\nC\""));
        s = JSON_toStringWithLength(n, &len);
        assert(strncmp(s, "\"A\\nB\\r\\nC\"", len) == 0);
        JSON_free(n);
        fprintf(stderr, "'%s'\n", s);
        free(s);
    }
    JSONMemoryPool_Delete(&jm);

#undef STRING
}
int main(int argc, char const* argv[])
{
    test_int();
    test_double();
    test_array();
    test_obj();
    test_string();
    return 0;
}
