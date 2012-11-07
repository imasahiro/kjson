#include <setjmp.h>
#include <string.h>

#ifndef KEXCEPTION_H
#define KEXCEPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#define TRY(HANDLER)  int jumped__ = 0; if((jumped__ = setjmp((HANDLER).handler)) == 0)
#define CATCH(X) if(jumped__ == (X))
#define FINALLY() L_finally:
#define THROW(HANDLER, EXCEPTION, ERROR_MSG) do {\
    (HANDLER)->error_message = (ERROR_MSG);\
    longjmp((HANDLER)->handler, (EXCEPTION));\
} while(0)

enum kjson_exception_type {
    KJSON_EXCEPTION     = 1,
    PARSER_EXCEPTION,
    STRINGIFY_EXCEPTION
};

typedef struct kexception_handler {
    jmp_buf handler;
    const char *error_message;
    int has_error;
} kexception_handler_t;

static void kexception_handler_reset(kexception_handler_t *eh)
{
    memset(eh->handler, 0, sizeof(jmp_buf));
}

static void kexception_handler_init(kexception_handler_t *eh)
{
    kexception_handler_reset(eh);
}

static int kexception_handler_deinit(kexception_handler_t *eh)
{
    kexception_handler_reset(eh);
    return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* end of include guard */
