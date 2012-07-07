#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef KJSON_STREAM_H
#define KJSON_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

struct input_stream;
typedef void (*istream_init)(struct input_stream *, void **args);
typedef void (*istream_deinit)(struct input_stream *);
typedef char (*istream_next)(struct input_stream *);
typedef void (*istream_prev)(struct input_stream *, char c);
typedef bool (*istream_eos)(struct input_stream *);

union io_data {
    char *str;
    FILE *fp;
    uintptr_t u;
    void *ptr;
};

typedef struct input_stream {
    union io_data d0;
    union io_data d1;
    union io_data d2;
    istream_next   fnext;
    istream_eos    feos;
    istream_prev   fprev;
    istream_deinit fdeinit;
} input_stream;

typedef const struct input_stream_api_t {
    istream_init   finit;
    istream_next   fnext;
    istream_prev fprev;
    istream_eos    feos;
    istream_deinit fdeinit;
} input_stream_api;

input_stream *new_string_input_stream(char *buf, size_t len);
input_stream *new_file_input_stream(char *filename, size_t bufsize);

void input_stream_delete(input_stream *ins);

static inline void input_stream_unput(input_stream *ins, char c)
{
    ins->fprev(ins, c);
}

static inline union io_data _input_stream_save(input_stream *ins)
{
    return ins->d0;
}

static inline void _input_stream_resume(input_stream *ins, union io_data data)
{
    ins->d0 = data;
}

static inline char string_input_stream_next(input_stream *ins)
{
    return *(ins->d0.str)++;
}

static inline void string_input_stream_prev(input_stream *ins, char c)
{
    --(ins->d0.str);
    *ins->d0.str = c;
}

static inline bool string_input_stream_eos(input_stream *ins)
{
    return ins->d0.str != ins->d1.str;
}

#define for_each_istream(INS, CUR)\
        for (CUR = string_input_stream_next(INS); string_input_stream_eos(INS); CUR = string_input_stream_next(INS))

#ifdef __cplusplus
}
#endif
#endif /* end of include guard */
