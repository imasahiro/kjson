#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "kjson.h"

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
    printf("%20s: %f sec\n", s, sec);
    reset_timer();
}

static const char test0[] = "[true, true, true, true, true, true, true, true  ]";
static const char test1[] = "[false, false, false, false, false, false, false, false  ]";
static const char test2[] = "[null, null, null, null, null, null, null, null  ]";

void f(void)
{
    JSON o;
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    int i;
#define N (1 << 20)
    int len0 = strlen(test0);
    int len1 = strlen(test1);
    int len2 = strlen(test2);
    reset_timer();
    for (i = 0; i < N; i++) {
        o = parseJSON(&jm, test0, test0+len0);
        if (o.bits == 0) {
            fprintf(stderr, "Errro\n");
        }
        JSON_free(o);
    }
    show_timer("parse true");
    for (i = 0; i < N; i++) {
        o = parseJSON(&jm, test1, test1+len1);
        if (o.bits == 0) {
            fprintf(stderr, "Errro\n");
        }
        JSON_free(o);
    }
    show_timer("parse false");
    for (i = 0; i < N; i++) {
        o = parseJSON(&jm, test2, test2+len2);
        if (o.bits == 0) {
            fprintf(stderr, "Errro\n");
        }
        JSON_free(o);
    }
    show_timer("parse null");

    JSONMemoryPool_Delete(&jm);
}

int main(int argc, char* argv[])
{
    int i;
    for (i = 0; i < 4; i++) {
        f();
    }
    return 0;
}
