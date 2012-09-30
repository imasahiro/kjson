#include "../kmemory_pool.h"
int main(int argc, char const* argv[])
{
#if 1
    memory_pool *pool = memory_pool_new();
    void *ptr;
    int i, j;
    for (j = 0; j < 128*128*32; j++) {
        for (i = 0; i < 4096/(1 << j); i++) {
            ptr = mpool_alloc(pool, 1 + (1 << j) % 512, NULL);
        }
    }
    memory_pool_delete(pool);
#else
    void *ptr;
    int i, j;
    for (j = 0; j < 128*128*32; j++) {
        for (i = 0; i < 4096/(1 << j); i++) {
            ptr = malloc(1 + (1 << j) % 512);
        }
    }
#endif
    return 0;
}
