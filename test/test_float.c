#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../kjson.c"

static double gen_float_plus()
{
    double f = rand() / (double)rand();
    return f * ((double)rand() / 1000);
}

static double gen_float_minus()
{
    double f = rand() / (double)rand();
    return f / ((double)rand() / 1000);
}

int main(int argc, char const* argv[])
{
    int i;
    for(i = 0; i < 4096*4096; i++) {
        double f = (rand() % 2 == 0 ? -1 : 1) *
            (rand() % 2 == 0 ? gen_float_plus() : gen_float_minus());
        JSON json = JSONDouble_new(f);
        assert(toDouble(json.val) - f <= 1e-200);
        JSON_free(json);
        char *str = JSON_toString(json);
        free(str);
    }
    return 0;
}
