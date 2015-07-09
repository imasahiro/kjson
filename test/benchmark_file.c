#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include "kjson.h"
#include "kjson_test.h"

static struct timeval g_timer;
static void reset_timer()
{
    gettimeofday(&g_timer, NULL);
}

static double show_timer(const char *s)
{
    struct timeval endtime;
    gettimeofday(&endtime, NULL);
    double msec = (endtime.tv_sec - g_timer.tv_sec) * 1000
        + (double)(endtime.tv_usec - g_timer.tv_usec) / 1000;
    return msec;
}

static inline void _show_timer(const char *s, size_t bufsz)
{
    double msec = show_timer(s);
    printf("%20s: %f msec, %f MB, %f Mbps\n",
            s, msec,
            ((double)bufsz)/1024/1024,
            ((double)bufsz)*8/msec/1000);
}

int main(int argc, char const* argv[])
{
    char *buf;
    size_t len;
    unsigned i;
    JSONMemoryPool jm;

    for (i = 0; i < 8; i++) {
        reset_timer();
        buf = load_file(argv[1], &len);
        JSONMemoryPool_Init(&jm);
        JSON json = JSON_parse_(&jm, buf, buf+len);
        // char *str = JSON_toString(json);
        // JSON_free(json);
        // free(str);
        JSONMemoryPool_Delete(&jm);
        free(buf);
        _show_timer("kjson", len);
    }
    return 0;
}
