#ifndef KJSON_MEMORY_H
#define KJSON_MEMORY_H
//#undef HAVE_JEMALLOC_H
#ifdef HAVE_JEMALLOC_H
#include <jemalloc/jemalloc.h>
#define KJSON_MALLOC(SIZE)        (malloc(SIZE))
#define KJSON_REALLOC(PTR, SIZE)  (realloc(PTR, SIZE))
#define KJSON_CALLOC(COUNT, SIZE) (calloc(COUNT, SIZE))
#define KJSON_FREE(PTR)           (free(PTR))
#else
#define KJSON_MALLOC(SIZE)        (malloc(SIZE))
#define KJSON_REALLOC(PTR, SIZE)  (realloc(PTR, SIZE))
#define KJSON_CALLOC(COUNT, SIZE) (calloc(COUNT, SIZE))
#define KJSON_FREE(PTR)           (free(PTR))
#endif
#endif /* end of include guard */
