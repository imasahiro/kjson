#include "kjson.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "benchmark.h"

#define SIZE 1024*128
static char *loadLine(FILE *fp, char *buf)
{
    bzero(buf, SIZE);
    return fgets(buf, SIZE, fp);
}
#define JSON_FILE "./test/twitter_public.json"
static int filesize = 0;
static void test0()
{
    size_t len;
    char buf[SIZE], *str;
    FILE *fp = fopen(JSON_FILE, "r");
    reset_timer();
    while((str = loadLine(fp, buf)) != NULL) {
        len = strlen(str);
        filesize += len;
    }
    show_timer("nop");
    fclose(fp);
}

static void test1()
{
    size_t len;
    char buf[SIZE], *str;
    FILE *fp = fopen(JSON_FILE, "r");
    reset_timer();
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    while((str = loadLine(fp, buf)) != NULL) {
        len = strlen(str);
        JSON json = parseJSON(&jm, str, str+len);
        assert(JSON_type(json) == JSON_Array);
        JSON_free(json);
    }
    JSONMemoryPool_Delete(&jm);
    show_timer("parse");
    fclose(fp);
}

static void test2()
{
    size_t len;
    char buf[SIZE], *str;
    FILE *fp = fopen(JSON_FILE, "r");
    reset_timer();
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    while((str = loadLine(fp, buf)) != NULL) {
        len = strlen(str);
        JSON json = parseJSON(&jm, str, str+len);
        char *p = JSON_toStringWithLength(json, &len);
        assert(JSON_type(json) == JSON_Array);
        assert(p);
        JSON_free(json);
        free(p);
    }
    JSONMemoryPool_Delete(&jm);
    show_timer("pack/unpack");
    fclose(fp);
}

int main(int argc, char const* argv[])
{
    int i, size = 8;
    fprintf(stderr, "benchmark3\n");
    test0();
    for(i = 0; i < size; i++) {
        test1();
    }
    for(i = 0; i < size; i++) {
        test2();
    }

    return 0;
}
