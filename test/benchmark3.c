#include "kjson.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "benchmark.h"

#define SIZE 1024*16
static char *loadLine(FILE *fp, char *buf)
{
    bzero(buf, SIZE);
    return fgets(buf, SIZE, fp);
}
#define JSON_FILE "./test/twitter.json"

static void test0()
{
    size_t len;
    char buf[SIZE], *str;
    FILE *fp = fopen(JSON_FILE, "r");
    reset_timer();
    while ((str = loadLine(fp, buf)) != NULL) {
        len = strlen(str);
        JSON json = parseJSON(str, str+len, 0);
        assert(JSON_type(json) == JSON_Object);
        JSON_free(json);
    }
    show_timer("parse");
    fclose(fp);
}
static void test1()
{
    size_t len;
    char buf[SIZE], *str;
    FILE *fp = fopen(JSON_FILE, "r");
    reset_timer();
    while ((str = loadLine(fp, buf)) != NULL) {
        len = strlen(str);
        JSON json = parseJSON(str, str+len, 0);
        char *p = JSON_toString(json, &len);
        assert(JSON_type(json) == JSON_Object);
        assert(p);
        JSON_free(json);
        free(p);
    }
    show_timer("pack/unpack");
    fclose(fp);
}

int main(int argc, char const* argv[])
{
    int i, size = 1;
    for (i = 0; i < size; i++) {
        test0();
        test1();
    }
    return 0;
}
