#include <stdio.h>
#include "kjson.h"

void case0()
{
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    const char *text = "{ \"a\" : {\"b\" : 100}}";
    JSON obj = JSON_parse_(&jm, text, text + strlen(text));
    JSON child = JSON_get(obj, "a", 1);
    JSON_Retain(child);
    assert(JSON_isValid(child));
    JSON_free(child);
    JSON_free(obj);
    JSONMemoryPool_Delete(&jm);
}

void case1()
{
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    const char *text = "{ \"a\" : {\"b\" : 100}}";
    JSON obj = JSON_parse_(&jm, text, text + strlen(text));
    JSON child = JSON_get(obj, "a", 1);
    JSON_Retain(child);
    JSONObject_set(&jm, obj, "a", 1, JSONInt_new(&jm, 100));
    assert(JSON_isValid(child));
    JSON_free(child);
    JSON_free(obj);
    JSONMemoryPool_Delete(&jm);
}

void case2()
{
    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    const char *text = "[ 100, {\"b\" : 100}]";
    JSON obj = JSON_parse_(&jm, text, text + strlen(text));
    JSON child = JSONArray_get(obj, 1);
    JSON_Retain(child);
    assert(JSON_isValid(child));
    JSON_free(child);
    JSON_free(obj);
    JSONMemoryPool_Delete(&jm);
}

int main(int argc, char const* argv[])
{
    case0();
    case1();
    case2();
    return 0;
}
