#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <msgpack.h>
#include <msgpack/pack.h>
#include <msgpack/unpack.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include "kjson.h"


static struct timeval g_timer;

void reset_timer()
{
    gettimeofday(&g_timer, NULL);
}

void show_timer(size_t bufsz)
{
    struct timeval endtime;
    gettimeofday(&endtime, NULL);
    double sec = (endtime.tv_sec - g_timer.tv_sec)
        + (double)(endtime.tv_usec - g_timer.tv_usec) / 1000 / 1000;
    printf("%f sec\n", sec);
}

static const unsigned int TASK_INT_NUM = 1<<24;
static const unsigned int TASK_STR_LEN = 1<<15;
//static const unsigned int TASK_INT_NUM = 1<<20;
//static const unsigned int TASK_STR_LEN = 1<<12;
static const char* TASK_STR_PTR;


void bench_json(void)
{
    puts("== JSON ==");
    yajl_gen g = yajl_gen_alloc(NULL);
    yajl_callbacks callbacks = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };
    yajl_handle h = yajl_alloc(&callbacks, NULL, NULL);
    const unsigned char * buf;
    size_t len;
    puts("generate integer");
    reset_timer();
    {
        unsigned int i;
        yajl_gen_array_open(g);
        for(i=0; i < TASK_INT_NUM; ++i) {
            yajl_gen_integer(g, i);
        }
        yajl_gen_array_close(g);
    }
    show_timer(len);

    yajl_gen_get_buf(g, &buf, &len);

    puts("----");
    puts("parse integer");
    reset_timer();
    {
        yajl_status stat = yajl_parse(h, buf, len);
        if (stat != yajl_status_ok) {
            unsigned char * str = yajl_get_error(h, 1, buf, len);
            fprintf(stderr, "%s\n", (const char *) str);
        }
    }
    show_timer(len);
    yajl_gen_free(g);
    g = yajl_gen_alloc(NULL);
    yajl_free(h);
    h = yajl_alloc(&callbacks, NULL, NULL);
    puts("----");
    puts("generate string");
    reset_timer();
    {
        unsigned int i;
        yajl_gen_array_open(g);
        for(i=0; i < TASK_STR_LEN; ++i) {
            yajl_gen_string(g, (const unsigned char*)TASK_STR_PTR, i);
        }
        yajl_gen_array_close(g);
    }
    show_timer(len);

    yajl_gen_get_buf(g, &buf, &len);

    puts("----");
    puts("parse string");
    reset_timer();
    {
        yajl_status stat = yajl_parse(h, buf, len);
        if (stat != yajl_status_ok) {
            unsigned char * str = yajl_get_error(h, 1, buf, len);
            fprintf(stderr, "%s", (const char *) str);
        }
    }
    show_timer(len);
    yajl_gen_free(g);
    yajl_free(h);
}

void bench_msgpack(void)
{
    puts("== MessagePack ==");
    msgpack_sbuffer *sbuf = msgpack_sbuffer_new();
    msgpack_packer* mpk = msgpack_packer_new(sbuf, msgpack_sbuffer_write);

    msgpack_unpacker mpac;
    msgpack_unpacker_init(&mpac, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);
    msgpack_unpacked msg;

    puts("pack integer");
    reset_timer();
    {
        unsigned int i;
        msgpack_pack_array(mpk, TASK_INT_NUM);
        for(i=0; i < TASK_INT_NUM; ++i) {
            msgpack_pack_unsigned_int(mpk, i);
        }
    }
    show_timer(0);

    puts("----");
    puts("unpack integer");
    reset_timer();
    {
        size_t off = 0;
        bool ret = msgpack_unpack_next(&msg, sbuf->data, sbuf->size, &off);
        if(ret < 0) {
            fprintf(stderr, "Parse error.\n");
        } else if(ret == 0) {
            fprintf(stderr, "Not finished.\n");
        }
    }
    show_timer(msgpack_unpacker_message_size(&mpac));

    msgpack_packer_free(mpk);
    msgpack_unpacker_reset(&mpac);
    mpk = msgpack_packer_new(sbuf, msgpack_sbuffer_write);
    puts("----");
    puts("pack string");
    reset_timer();
    {
        unsigned int i;
        msgpack_pack_array(mpk, TASK_STR_LEN);
        for(i=0; i < TASK_STR_LEN; ++i) {
            msgpack_pack_raw(mpk, i);
            msgpack_pack_raw_body(mpk, TASK_STR_PTR, i);
        }
    }
    show_timer(0);

    puts("----");
    puts("unpack string");
    reset_timer();
    {
        size_t off = 0;
        bool ret = msgpack_unpack_next(&msg, sbuf->data, sbuf->size, &off);
        if(ret < 0) {
            fprintf(stderr, "Parse error.\n");
        } else if(ret == 0) {
            fprintf(stderr, "Not finished.\n");
        }
    }
    show_timer(msgpack_unpacker_message_size(&mpac));

}

void bench_kjson(void)
{
    puts("== KJSON ==");

    JSON o;
    char *buf;
    size_t len;
    puts("generate integer");
    reset_timer();
    {
        unsigned int i;
        o = JSONArray_new();
        for(i=0; i < TASK_INT_NUM; ++i) {
            JSON v = JSONInt_new(i);
            JSONArray_append((JSONArray*)o, v);
        }
    }
    show_timer(len);

    buf = JSON_toString(o, &len);
    JSON_free(o);
    puts("----");
    puts("parse integer");
    reset_timer();
    {
        o = parseJSON(buf, buf + len);
        if (o == NULL) {
            fprintf(stderr, "Error\n");
        }
    }
    show_timer(len);

    JSON_free(o);
    free(buf);

    puts("----");
    puts("generate string");
    reset_timer();
    {
        unsigned int i;
        o = JSONArray_new();
        for(i=0; i < TASK_STR_LEN; ++i) {
            JSON v = JSONString_new((char*)TASK_STR_PTR, i);
            JSONArray_append((JSONArray*)o, v);
        }
    }
    show_timer(len);
    buf = JSON_toString(o, &len);
    JSON_free(o);

    puts("----");
    puts("parse string");
    reset_timer();
    {
        o = parseJSON(buf, buf + len);
        if (o == NULL) {
            fprintf(stderr, "Errro\n");
        }
    }
    show_timer(len);
    JSON_free(o);
    free(buf);
}

int main(int argc, char* argv[])
{
    int i;
    char* str = malloc(TASK_STR_LEN);
    memset(str, 'a', TASK_STR_LEN);
    TASK_STR_PTR = str;
    bench_msgpack();
    bench_json();
    for (i = 0; i < 3; i++) {
        bench_kjson();
    }
    return 0;
}
