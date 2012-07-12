#include "map.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

#define N (1024 * 1024 * 10)

static int entry_key_eq(uintptr_t k0, uintptr_t k1)
{
    char *s0 = (char *) (k0);
    char *s1 = (char *) (k1);
    return strcmp(s0, s1) == 0;
}

static void entry_free(pmap_record_t *r)
{
}

static uintptr_t entry_keygen(char *key, uint32_t len)
{
    return (uintptr_t) key;
}

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
}

static dictmap_t *D[N];
static poolmap_t *P[N];
static char *data[] = {
    "0000000000000000",
    "1111111111111111",
    "2222222222222222",
    "3333333333333333",
};

static const int len[] = {
    sizeof("0000000000000000")-1,
    sizeof("1111111111111111")-1,
    sizeof("2222222222222222")-1,
    sizeof("3333333333333333")-1,
};

static const struct poolmap_entry_api API = {
    entry_keygen, entry_keygen, entry_key_eq, entry_free
};

static void test_dict() {
    int i;
    pmap_record_t *r;
    dictmap_t *d = dictmap_new(&API);
    reset_timer();
    for (i = 0; i < N; i++) {
        D[i] = dictmap_new(&API);
    }
    show_timer("dict:new");
    reset_timer();
    for (i = 0; i < N; i++) {
        dictmap_set(d, data[i % 4], len[i % 4], d);
    }
    show_timer("dict:set");
    reset_timer();
    for (i = 0; i < N; i++) {
        r = dictmap_get(d, data[i % 4], len[i % 4]);
    }
    show_timer("dict:get");
    reset_timer();
    for (i = 0; i < N; i++) {
        dictmap_delete(D[i]);
    }
    show_timer("dict:delete");
    dictmap_delete(d);
}

static void test_pool() {
    int i;
    pmap_record_t *r;
    poolmap_t *p = poolmap_new(4, &API);
    reset_timer();
    for (i = 0; i < N; i++) {
        P[i] = poolmap_new(4, &API);
    }
    show_timer("pool:new");
    reset_timer();
    for (i = 0; i < N; i++) {
        poolmap_set(p, data[i % 4], len[i % 4], p);
    }
    show_timer("pool:set");
    reset_timer();
    for (i = 0; i < N; i++) {
        r = poolmap_get(p, data[i % 4], len[i % 4]);
    }
    show_timer("pool:get");
    reset_timer();
    for (i = 0; i < N; i++) {
        poolmap_delete(P[i]);
    }
    show_timer("pool:delete");
    poolmap_delete(p);
}

int main(int argc, char const* argv[])
{
    int i;
    assert(sizeof(poolmap_t) == sizeof(dictmap_t));
    int loop_count = 1;
    if (argc > 1 && strncmp(argv[1], "-t", 2) == 0) {
        loop_count = atoi(argv[1]+2);
    }
    for (i = 0; i < loop_count; i++) {
        test_dict();
        test_pool();
    }
    return 0;
}
void JSON_free(void* o) {};

