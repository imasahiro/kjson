#include "map.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

#define N (1024 * 1024 * 10)
#define M 1024*1024*4

static int entry_key_eq(uintptr_t k0, uintptr_t k1)
{
    char *s0 = (char *) (k0);
    char *s1 = (char *) (k1);
    return strcmp(s0, s1) == 0;
}

static void entry_free(map_record_t *r)
{
}

static uintptr_t entry_keygen(char *key, uint32_t len)
{
    return (uintptr_t) key;
}

static const struct kmap_entry_api API = {
    entry_keygen, entry_keygen, entry_key_eq, entry_free
};

static int number_key_eq(uintptr_t k0, uintptr_t k1)
{
    return k0 == k1;
}

static void number_free(map_record_t *r) {}
static uintptr_t number_keygen(char *key, uint32_t len)
{
    return (uintptr_t) key;
}

static const struct kmap_entry_api NUMBER_API = {
    number_keygen, number_keygen, number_key_eq, number_free
};

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

static kmap_t *P[N];
static char *data[] = {
    "0000000000000000",
    "1111111111111111",
    "2222222222222222",
    "3333333333333333",
    "4444444444444444",
    "5555555555555555"
};
static const int len[] = {
    sizeof("0000000000000000")-1,
    sizeof("1111111111111111")-1,
    sizeof("2222222222222222")-1,
    sizeof("3333333333333333")-1,
    sizeof("4444444444444444")-1,
    sizeof("5555555555555555")-1
};

static void test_pool() {
    int i;
    map_record_t *r;
    kmap_t *p = kmap_new(4, &API);
    reset_timer();
    for (i = 0; i < N; i++) {
        P[i] = kmap_new(4, &API);
    }
    show_timer("pool:new");
    reset_timer();
    for (i = 0; i < N; i++) {
        kmap_set(p, data[i % 4], len[i % 4], p);
    }
    show_timer("pool:set");
    reset_timer();
    for (i = 0; i < N; i++) {
        r = kmap_get(p, data[i % 4], len[i % 4]);
    }
    show_timer("pool:get");
    reset_timer();
    for (i = 0; i < N; i++) {
        kmap_delete(P[i]);
    }
    show_timer("pool:delete");
    kmap_delete(p);
}

static void test_converter() {
    int i;
    kmap_t *p = kmap_new(4, &API);
    map_record_t *r;
    for (i = 0; i < 6; i++) {
        kmap_set(p, data[i], len[i], p);
    }
    for (i = 0; i < 6; i++) {
        r = kmap_get(p, data[i], len[i]);
        assert(r != NULL);
    }
    kmap_delete(p);
}

static void test_huge_map() {
    int i;
    map_record_t *r;
    kmap_t *p = kmap_new(8, &NUMBER_API);
    reset_timer();
    for (i = 1; i <= M; i++) {
        uintptr_t val = (uintptr_t)(i);
        kmap_set(p, (char*)val, 0, (void*)val);
    }
    show_timer("map:set");

    reset_timer();
    for (i = 1; i <= M; i++) {
        uintptr_t val = (uintptr_t)(0-(i+1));
        r = kmap_get(p, (char*)val, 0);
        assert(r == NULL);
    }
    show_timer("map:get:miss");

    reset_timer();
    for (i = 1; i <= M; i++) {
        uintptr_t val = (uintptr_t)(i);
        r = kmap_get(p, (char*)val, 0);
        assert(r->v == val);
    }
    show_timer("map:get");
    kmap_delete(p);
}

int main(int argc, char const* argv[])
{
    int i;
    assert(sizeof(kmap_t) == sizeof(dictmap_t));
    int loop_count = 1;
    if (argc > 1 && strncmp(argv[1], "-t", 2) == 0) {
        loop_count = atoi(argv[1]+2);
    }
    test_converter();
    for (i = 0; i < loop_count; i++) {
        test_pool();
    }
    test_huge_map();
    return 0;
}
void JSON_free(void* o) {};

