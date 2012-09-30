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

#include "kjson.h"
#include "karray.h"
#ifndef KMEMORY_H
#define KMEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef unlikely
#define unlikely(x)   __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#endif

#define PowerOf2(N) (1UL << N)
#define MIN_ALIGN_LOG2 (4)
#define MAX_ALIGN_LOG2 (9)
#define ALIGN(X, N) (((X)+((N)-1))&(~((N)-1)))
#define CEIL(F)     (F-(int)(F) > 0 ? (int)(F+1) : (int)(F))
#define MEMORYBLOCK_SIZE (1024*16)

typedef char *CharPtr;
DEF_ARRAY_STRUCT0(CharPtr, unsigned);
DEF_ARRAY_T(CharPtr);
DEF_ARRAY_OP_NOPOINTER(CharPtr);

typedef struct PageData  { ARRAY(CharPtr) block; } PageData;
typedef struct BlockInfo { char *current, *base; } BlockInfo;

DEF_ARRAY_STRUCT0(PageData, unsigned);
DEF_ARRAY_T(PageData);
DEF_ARRAY_OP(PageData);

DEF_ARRAY_STRUCT0(BlockInfo, unsigned);
DEF_ARRAY_T(BlockInfo);
DEF_ARRAY_OP(BlockInfo);

typedef struct memory_pool {
    ARRAY(BlockInfo) current_block;
    ARRAY(PageData)  array;
} memory_pool;

static memory_pool *memory_pool_new()
{
    int i;
    memory_pool *pool = KJSON_MALLOC(sizeof(*pool));
    ARRAY_init(PageData,  &pool->array, MAX_ALIGN_LOG2 - MIN_ALIGN_LOG2);
    ARRAY_init(BlockInfo, &pool->current_block, MAX_ALIGN_LOG2 - MIN_ALIGN_LOG2);
    for (i = MIN_ALIGN_LOG2; i <= MAX_ALIGN_LOG2; ++i) {
        PageData page;
        BlockInfo block;
        ARRAY_init(CharPtr, &page.block, 4);
        ARRAY_add(CharPtr, &page.block, KJSON_MALLOC(MEMORYBLOCK_SIZE));
        ARRAY_add(PageData, &pool->array, &page);
        block.base = ARRAY_get(CharPtr, &page.block, 0);
        block.current = block.base;
        ARRAY_add(BlockInfo, &pool->current_block, &block);
    }
    return pool;
}

static void *mpool_alloc(memory_pool *pool, size_t n, bool *malloced)
{
    assert(n > 0);
    if (unlikely(n > (1 << MAX_ALIGN_LOG2))) {
        *malloced = true;
        return KJSON_MALLOC(n);
    }

    size_t size = ALIGN(n, 1 << MIN_ALIGN_LOG2);
    unsigned index = LOG2(size) - MIN_ALIGN_LOG2;
    BlockInfo *block = ARRAY_get(BlockInfo, &pool->current_block, index);
    if (likely((block->current + size - block->base) <= MEMORYBLOCK_SIZE)) {
        char *ptr = block->current;
        block->current = block->current + size;
        return (void *) ptr;
    }
    PageData *page = ARRAY_get(PageData, &pool->array, index);
    void *newblock = KJSON_MALLOC(MEMORYBLOCK_SIZE);
    block->base    = (char *)newblock;
    block->current = (char *)newblock + size;
    ARRAY_add(CharPtr, &page->block, block->base);
    return newblock;
}

static void memory_pool_delete(memory_pool *pool)
{
    PageData *p, *end;
    FOR_EACH_ARRAY(pool->array, p, end) {
        CharPtr *x, *e;
        FOR_EACH_ARRAY(p->block, x, e) {
            KJSON_FREE(*x);
        }
    }
    ARRAY_dispose(PageData, &pool->array);
    ARRAY_dispose(BlockInfo,  &pool->current_block);
    KJSON_FREE(pool);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* end of include guard */
