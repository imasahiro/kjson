#include "stream.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline input_stream *new_input_stream(void **args, input_stream_api *api,
        long flags)
{
    input_stream *ins = (input_stream *) calloc(1, sizeof(input_stream));
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
    free(ins);
}

#ifdef __cplusplus
}
#endif
