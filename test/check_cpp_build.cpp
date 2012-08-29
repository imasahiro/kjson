#include "../kjson.c"
#include "../map.c"
#include "../stream.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const* argv[])
{
    FILE *fp = fopen("test/sample.json", "rb");
    if (!fp) 
        fp = fopen("../test/sample.json", "rb");
    assert(fp != 0);

    fseek(fp, 0, SEEK_END);
    size_t len = (size_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *json = (char*)calloc(1, len + 1);
    assert(len == fread(json, 1, len, fp));
    json[len] = '\0';
    fclose(fp);

    JSON o = parseJSON(json, json + len);
    assert(o);
    if (o == NULL) {
        fprintf(stderr, "Error\n");
        exit(EXIT_FAILURE);
    }
    JSON a = JSON_get(o, "a");
    assert(a);
    JSON_free(o);
    free(json);
    return 0;
}
