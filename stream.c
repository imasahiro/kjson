#include "stream.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline input_stream *new_input_stream(void **args, input_stream_api *api)
{
    input_stream *ins = (input_stream *) calloc(1, sizeof(input_stream));
    api->finit(ins, args);
    ins->fdeinit = api->fdeinit;
    ins->fnext   = api->fnext;
    ins->fprev   = api->fprev;
    ins->feos    = api->feos;
    return ins;
}

static void string_input_stream_init(input_stream *ins, void **args)
{
    char *text;
    size_t len;
    text = (char *) args[0];
    len  = (size_t) args[1];
    ins->d0.str = text;
    ins->d1.str = text + len + 1;
    ins->d2.u   = 0;
}

static char string_input_stream_next(input_stream *ins)
{
    return *(ins->d0.str)++;
}

static void string_input_stream_prev(input_stream *ins, char c)
{
    --(ins->d0.str);
    *ins->d0.str = c;
}

static bool string_input_stream_eos(input_stream *ins)
{
    return ins->d0.str != ins->d1.str;
}

static void string_input_stream_deinit(input_stream *ins)
{
    ins->d0.str = ins->d1.str = NULL;
}

input_stream *new_string_input_stream(char *buf, size_t len)
{
    void *args[] = {
        (void*)buf, (void*)len
    };
    static const input_stream_api string_input_stream_api = {
        string_input_stream_init,
        string_input_stream_next,
        string_input_stream_prev,
        string_input_stream_eos,
        string_input_stream_deinit
    };
    input_stream *ins = new_input_stream(args, &string_input_stream_api);
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
