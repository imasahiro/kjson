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

#include "array.h"
#include <string.h>
#include <stdint.h>

#ifndef KJSON_STRING_BUILDER_H_
#define KJSON_STRING_BUILDER_H_

DEF_ARRAY_STRUCT0(char, unsigned);
DEF_ARRAY_T(char);
DEF_ARRAY_OP_NOPOINTER(char);

typedef struct string_builder {
    ARRAY(char) buf;
} string_builder;

static inline void string_builder_init(string_builder *builder)
{
    ARRAY_init(char, &builder->buf, 4);
}

static inline void string_builder_add(string_builder *builder, char c)
{
    ARRAY_add(char, &builder->buf, c);
}

static void reverse(char *const start, char *const end)
{
    char *m = start + (end - start) / 2;
    char tmp, *s = start, *e = end - 1;
    while (s < m) {
        tmp  = *s;
        *s++ = *e;
        *e-- = tmp;
    }
}

static inline char toHexChar(unsigned char c)
{
    return c < 10 ? c + '0': c - 10 + 'a';
}

static inline char *put_x(char *p, uint64_t v)
{
    char *base = p;
    do {
        *p++ = toHexChar(v % 16);
    } while ((v /= 16) != 0);
    reverse(base, p);
    return p;
}

static inline char *put_d(char *p, uint64_t v)
{
    char *base = p;
    do {
        *p++ = '0' + ((unsigned char)(v % 10));
    } while ((v /= 10) != 0);
    reverse(base, p);
    return p;
}

static inline char *put_i(char *p, int64_t value)
{
    if(value < 0) {
        p[0] = '-'; p++;
        value = -value;
    }
    return put_d(p, (uint64_t)value);
}
static inline void string_builder_add_hex(string_builder *builder, uint32_t i)
{
    ARRAY_ensureSize(char, &builder->buf, 4);
    char *p = builder->buf.list + ARRAY_size(builder->buf);
    char *e = put_x(p, i);
    builder->buf.size += e - p;
}
static inline void string_builder_add_int(string_builder *builder, int32_t i)
{
    ARRAY_ensureSize(char, &builder->buf, 12);/* sizeof("-2147483648") */
    char *p = builder->buf.list + ARRAY_size(builder->buf);
    char *e = put_i(p, i);
    builder->buf.size += e - p;
}
static inline void string_builder_add_int64(string_builder *builder, int64_t i)
{
    ARRAY_ensureSize(char, &builder->buf, 20);/* sizeof("-9223372036854775807") */
    char *p = builder->buf.list + ARRAY_size(builder->buf);
    char *e = put_i(p, i);
    builder->buf.size += e - p;
}
static inline void string_builder_add_string(string_builder *builder, const char *s, size_t len)
{
    ARRAY_ensureSize(char, &builder->buf, len);
    char *p = builder->buf.list + ARRAY_size(builder->buf);
    const char *const e = s + len;
    while (s < e) {
        *p++ = *s++;
    }
    builder->buf.size += len;
}

static inline void string_builder_dispose(string_builder *builder)
{
    ARRAY_dispose(char, &builder->buf);
}

static inline char *string_builder_tostring(string_builder *builder,
        size_t *len, int ensureZero)
{
    if (ensureZero) {
        ARRAY_add(char, &builder->buf, '\0');
    }
    char *list = builder->buf.list;
    *len = (size_t) builder->buf.size;
    builder->buf.list     = NULL;
    builder->buf.size     = 0;
    builder->buf.capacity = 0;
    return list;
}

#endif /* end of include guard */
