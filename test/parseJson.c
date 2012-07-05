#include "kjson.h"
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static struct timeval g_timer;

static void reset_timer()
{
    gettimeofday(&g_timer, NULL);
}

static void show_timer(const char *s)
{
    struct timeval endtime;
    gettimeofday(&endtime, NULL);
    double sec = (endtime.tv_sec - g_timer.tv_sec)
        + (double)(endtime.tv_usec - g_timer.tv_usec) / 1000 / 1000;
    printf("%-30s: %f sec\n", s, sec);
}


static void test_file(const char *file)
{
    size_t len = 1024, size, offset = 0;
    char *str = malloc(1024);
    char *end = str + 1024;
    FILE *fp = fopen(file, "r");
    char buf[1024];
    reset_timer();
    while (1) {
        size = fread(buf, 1, 1024, fp);
        if (size == 0)
            break;
        if (str + offset + size > end) {
            len *= 2;
            str = realloc(str, len);
            end = str + len;
        }
        memcpy(str+offset, buf, size);
        offset += size;
    }
    int i;
    for (i = 0; i < 128; ++i) {
        JSON json = parseJSON(str, str+len);
        JSON_free(json);
    }
    show_timer(file);
    free(str);
}

int main(int argc, char const* argv[])
{
    const char *files[] = {
        "./test/benchmark1.json",
        "./test/benchmark2.json",
        "./test/benchmark3.json",
        "./test/benchmark4.json",
        "./test/benchmark5.json",
        "./test/delicious_popular.json",
        "./test/twitter_public.json",
        "./test/lastfm.json",
        "./test/yelp.json",
    };
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif
    size_t size = ARRAY_SIZE(files);
    int i;
    for (i = 0; i < size; i++) {
        test_file(files[i]);
    }
    return 0;
}
