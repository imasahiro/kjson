#include "kexception.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int check_try_catch(int n)
{
    kexception_handler_t e = {{}};
    kexception_handler_init(&e);
    int num = 0;
    TRY(e) {
        num += 100;
        THROW(&e, n, "THROWED");
    }
    CATCH(PARSER_EXCEPTION) {
        num += 10;
        goto L_finally;
    } CATCH(STRINGIFY_EXCEPTION) {
        num += 20;
    } FINALLY() {
        num += 1;
    }
    assert(kexception_handler_deinit(&e) == 0);
    return num;
}

int main(int argc, char const* argv[])
{
    assert(check_try_catch(0) == 101);
    assert(check_try_catch(1) == 101);
    assert(check_try_catch(2) == 111);
    assert(check_try_catch(3) == 121);
    assert(check_try_catch(4) == 101);
    return 0;
}
