#include <stdio.h>
#include "kjson.h"

int main(int argc, char const* argv[])
{
#define SIZE_OF(T) "%s %d\n", #T, (int)sizeof(T)
    fprintf(stderr, SIZE_OF(JSON));
    fprintf(stderr, SIZE_OF(JSONInt));
    fprintf(stderr, SIZE_OF(JSONInt64));
    fprintf(stderr, SIZE_OF(JSONDouble));
    fprintf(stderr, SIZE_OF(JSONString));
    fprintf(stderr, SIZE_OF(JSONArray));
    fprintf(stderr, SIZE_OF(JSONObject));
    fprintf(stderr, SIZE_OF(dictmap_t));
    fprintf(stderr, SIZE_OF(hashmap_t));
    return 0;
}
