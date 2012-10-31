#ifndef KJSON_H
#include "kjson.h"
#endif

#ifndef KSTACK_H
#define KSTACK_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef KJSON_MALLOC
#define KJSON_MALLOC(N) malloc(N)
#define KJSON_FREE(PTR) free(PTR)
#endif

DEF_ARRAY_STRUCT0(JSON, unsigned);
DEF_ARRAY_T(JSON);
DEF_ARRAY_OP_NOPOINTER(JSON);

typedef ARRAY(JSON) kstack_t;

static inline unsigned kstack_size(kstack_t *stack)
{
    return stack->size;
}

static inline void kstack_init(kstack_t *stack)
{
    ARRAY_init(JSON, stack, 8);
}

static inline void kstack_deinit(kstack_t *stack)
{
    assert(kstack_size(stack) == 0);
    ARRAY_dispose(JSON, stack);
}

static inline void kstack_push(kstack_t *stack, JSON v)
{
    ARRAY_add(JSON, stack, v);
}

static inline JSON kstack_pop(kstack_t *stack)
{
    unsigned size = --stack->size;
    assert(size >= 0);
    return ARRAY_get(JSON, stack, size);
}

static inline void kstack_move(kstack_t *stack, JSON *list, unsigned beginIdx, unsigned length)
{
    memcpy(list, stack->list+beginIdx, length*sizeof(JSON));
    stack->size -= length;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* end of include guard */
