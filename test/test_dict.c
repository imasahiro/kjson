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
static void test_converter() {
    static char *data2[] = {
        "0000000000000000",
        "1111111111111111",
        "2222222222222222",
        "3333333333333333",
        "4444444444444444",
        "5555555555555555"
    };
    static const int len2[] = {
        sizeof("0000000000000000")-1,
        sizeof("1111111111111111")-1,
        sizeof("2222222222222222")-1,
        sizeof("3333333333333333")-1,
        sizeof("4444444444444444")-1,
        sizeof("5555555555555555")-1
    };
    int i;
    poolmap_t *p = poolmap_new(4, &API);
    pmap_record_t *r;
    for (i = 0; i < 6; i++) {
        poolmap_set(p, data2[i], len2[i], p);
    }
    for (i = 0; i < 6; i++) {
        r = poolmap_get(p, data2[i], len2[i]);
        assert(r != NULL);
    }
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
    test_converter();
    for (i = 0; i < loop_count; i++) {
        test_pool();
    }
    return 0;
}
void JSON_free(void* o) {};

