#include "../kstack.h"

int main(int argc, char const* argv[])
{
    kstack_t stack;
    kstack_init(&stack);
    kstack_push(&stack, JSONNull_new());
    kstack_push(&stack, JSONNull_new());
    kstack_push(&stack, JSONNull_new());
    kstack_push(&stack, JSONNull_new());
    assert(JSON_type(kstack_pop(&stack)) == JSON_Null);
    assert(JSON_type(kstack_pop(&stack)) == JSON_Null);
    assert(JSON_type(kstack_pop(&stack)) == JSON_Null);
    assert(JSON_type(kstack_pop(&stack)) == JSON_Null);
    kstack_deinit(&stack);
    return 0;
}
