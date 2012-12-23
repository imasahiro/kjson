#include <stdio.h>
#include "kjson.h"

int main(int argc, char const* argv[])
{
#define SIZE_OF(T) do {\
    fprintf(stderr, "%s %d\n", #T, (int)sizeof(T));\
} while(0)
    SIZE_OF(JSON);
    SIZE_OF(JSONInt);
    SIZE_OF(JSONInt64);
    SIZE_OF(JSONDouble);
    SIZE_OF(JSONString);
    SIZE_OF(JSONArray);
    SIZE_OF(JSONObject);
    SIZE_OF(dictmap_t);
    SIZE_OF(hashmap_t);
    assert(sizeof(JSONInt)     <= sizeof(JSON));
    assert(sizeof(JSONInt64)   <= sizeof(JSON));
    assert(sizeof(JSONDouble)  <= sizeof(JSON));
    assert(sizeof(JSONString *) <= sizeof(JSON));
    return 0;
}
