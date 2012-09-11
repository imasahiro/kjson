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

#include "kmemory.h"
#include "stream.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline input_stream *new_input_stream(void **args, input_stream_api *api,
        long flags)
{
    input_stream *ins = (input_stream *) KJSON_CALLOC(1, sizeof(input_stream));
    api->finit(ins, args);
    ins->fdeinit = api->fdeinit;
    ins->fnext   = api->fnext;
    ins->feos    = api->feos;
    ins->flags   = flags;
    return ins;
}

static void string_input_stream_init(input_stream *ins, void **args)
{
    unsigned char *text;
    size_t len;
    text = (unsigned char *) args[0];
    len  = (size_t) args[1];
    ins->d0.str = text;
    ins->d1.str = text + len + 1;
    ins->d2.u   = 0;
}

static void string_input_stream_deinit(input_stream *ins)
{
    ins->d0.str = ins->d1.str = NULL;
}

input_stream *new_string_input_stream(const char *buf, size_t len, long flags)
{
    void *args[] = {
        (void*)buf, (void*)len
    };
    static const input_stream_api string_input_stream_api = {
        string_input_stream_init,
        string_input_stream_next,
        string_input_stream_eos,
        string_input_stream_deinit
    };
    input_stream *ins = new_input_stream(args, &string_input_stream_api, flags);
    return ins;
}

void input_stream_delete(input_stream *ins)
{
    ins->fdeinit(ins);
    KJSON_FREE(ins);
}

#ifdef __cplusplus
}
#endif
