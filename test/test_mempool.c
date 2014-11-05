#include "kmemory_pool.h"
int main(int argc, char const* argv[])
{
#if 1
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    void *ptr;
    int i, j;
    for(j = 0; j < 128*128*32; j++) {
        for(i = 0; i < 4096/(1 << j); i++) {
            ptr = JSONMemoryPool_Alloc(&jm, 1 + (1 << j) % 512, NULL);
        }
    }
    JSONMemoryPool_Delete(&jm);
    (void)ptr;
#else
    void *ptr;
    int i, j;
    for(j = 0; j < 128*128*32; j++) {
        for(i = 0; i < 4096/(1 << j); i++) {
            ptr = malloc(1 + (1 << j) % 512);
        }
    }
#endif
    return 0;
}
