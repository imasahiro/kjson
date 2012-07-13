//#define USE_JSON_C
//#define USE_YAJL
//#define USE_JANSSON
#undef USE_JSON_C
#undef USE_YAJL
#undef USE_JANSSON

#ifdef USE_JSON_C
#include <json/json.h>
#endif

#ifdef USE_YAJL
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>
#endif

#ifdef USE_JANSSON
#include <jansson.h>
#endif

#include "kjson.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "benchmark.h"

static void kjson(char *buf, size_t len) {
    JSON json = parseJSON(buf, buf+len, 0);
    size_t length;
    char *str = JSON_toString(json, &length);
    JSON_free(json);
    free(str);
}

#ifdef USE_JSON_C
static void json_c(char *buf, size_t len) {
    struct json_object *new_obj;
    new_obj = json_tokener_parse(buf);
    json_object_to_json_string(new_obj);
    json_object_put(new_obj);
}
#endif

#ifdef USE_YAJL
static void yajl(char *buf, size_t len) {
    const unsigned char *t;
    yajl_gen g = yajl_gen_alloc(NULL
#if YAJL_MAJOR < 2
            , NULL
#endif
            );
    yajl_handle h = yajl_alloc(NULL, NULL
#if YAJL_MAJOR < 2
            , NULL
#endif
            , NULL);
    yajl_status stat = yajl_parse(h, (unsigned char*)buf, len);
#if YAJL_MAJOR < 2
    stat = yajl_parse_complete(h);
#else
    stat = yajl_complete_parse(h);
#endif
    assert(stat == yajl_status_ok);
    yajl_gen_get_buf(g, &t, &len);
    yajl_gen_free(g);
    yajl_free(h);
}
#endif

#ifdef USE_JANSSON
static void jansson(char *buf, size_t len) {
    json_t *obj;
    json_error_t err;
    obj = json_loadb(buf, len, 0, &err);
    char *s = json_dumps(obj, 0);
    json_delete(obj);
    free(s);
}
#endif

static int loop_count = 128;
static const char json_text[] = "\n"
"{\n"
"   \"a\" : \"Alpha\",\n"
"   \"b\" : true,\n"
"   \"c\" : 12345,\n"
"   \"d\" : [true, [false, [12345, null], 3.967, [\"something\", false], null]],\n"
"   \"e\" : { \"one\" : 1, \"two\" : 2},\n"
"   \"f\" : null,\n"
"   \"h\" : { \"a\" : { \"b\" : { \"c\" : "
"             { \"d\" : {\"e\" : { \"f\" : { \"g\" : null }}}}}}},\n"
"   \"i\" : [[[[[[[null]]]]]]]\n"
"}\n";

static void test_file(const char *file)
{
    char *str = (char*)json_text;
    size_t len = strlen(json_text);
    int i, j, k;
    struct f {
        const char *name;
        void (*func)(char *buf, size_t len);
    } data[] = {
#ifdef USE_JSON_C
        {"json-c", json_c},
#endif
        {"kjson",  kjson},
#ifdef USE_YAJL
        {"yajl",   yajl},
#endif
#ifdef USE_JANSSON
        {"jansson", jansson},
#endif

    };
    for (k = 0; k < loop_count; k++) {
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
        for (j = 0; j < ARRAY_SIZE(data); j++) {
            reset_timer();
            for (i = 0; i < 1024; ++i) {
                data[j].func(str, len);
            }
            show_timer(data[j].name);
        }
    }
}

int main(int argc, char const* argv[])
{
    if (argc > 1 && strncmp(argv[1], "-t", 2) == 0) {
        int n = atoi(argv[1]+2);
        loop_count = n;
    }
    test_file("simple.json");
    return 0;
}
