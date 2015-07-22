#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "kjson.h"
#include "benchmark.h"

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
    for(i = 0; i < N; i++) {
        o = JSON_parse_(&jm, test0, test0+len0);
        if(o.bits == 0) {
            fprintf(stderr, "Errro\n");
        }
        assert(JSON_length(o) == 8);
        JSON_free(o);
    }
    show_timer("parse true");
    for(i = 0; i < N; i++) {
        o = JSON_parse_(&jm, test1, test1+len1);
        if(o.bits == 0) {
            fprintf(stderr, "Errro\n");
        }
        assert(JSON_length(o) == 8);
        JSON_free(o);
    }
    show_timer("parse false");
    for(i = 0; i < N; i++) {
        o = JSON_parse_(&jm, test2, test2+len2);
        if(o.bits == 0) {
            fprintf(stderr, "Errro\n");
        }
        assert(JSON_length(o) == 8);
        JSON_free(o);
    }
    show_timer("parse null");

    JSONMemoryPool_Delete(&jm);
}

int main(int argc, char* argv[])
{
    int i;
    for(i = 0; i < 4; i++) {
        f();
    }
    return 0;
}
