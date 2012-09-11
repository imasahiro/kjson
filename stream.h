/****************************************************************************
 * Copyright (c) 2012, Masahiro Ide <ide@konohascript.org>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

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
typedef bool (*istream_eos)(struct input_stream *);

union io_data {
    unsigned char *str;
    FILE *fp;
    uintptr_t u;
    void *ptr;
};

typedef struct input_stream {
    union io_data d0;
    union io_data d1;
    union io_data d2;
    long flags;
    istream_next   fnext;
    istream_eos    feos;
    istream_deinit fdeinit;
} input_stream;

typedef const struct input_stream_api_t {
    istream_init   finit;
    istream_next   fnext;
    istream_eos    feos;
    istream_deinit fdeinit;
} input_stream_api;

input_stream *new_string_input_stream(const char *buf, size_t len, long option);
input_stream *new_file_input_stream(char *filename, size_t bufsize);

void input_stream_delete(input_stream *ins);

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

static inline bool string_input_stream_eos(input_stream *ins)
{
    return ins->d0.str != ins->d1.str;
}

#define for_each_istream(INS, CUR)\
    for (CUR = string_input_stream_next(INS);\
            string_input_stream_eos(INS); CUR = string_input_stream_next(INS))

#ifdef __cplusplus
}
#endif
#endif /* end of include guard */
