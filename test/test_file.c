#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../kjson/kjson.c"

int main(int argc, char const* argv[])
{
    if(argc != 3)
        return 1;
    FILE *fp = fopen(argv[1], "r");
    assert(fp != 0);

    fseek(fp, 0, SEEK_END);
    size_t len = (size_t) ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *json = (char *) calloc(1, len + 1);
    size_t readed = fread(json, 1, len, fp);
    assert(len == readed);
    fprintf(stderr, "len=%d, json='%s'\n", (int)readed, json);
    fclose(fp);

    JSONMemoryPool jm;
    JSONMemoryPool_Init(&jm);
    JSON o = parseJSON(&jm, json, json + len);
    if(argv[2][0] == 'F') {
        assert(IsError(o.val));
        fprintf(stderr, "Checking Error:%s\n", JSONError_get(o));
    } else {
        assert(o.bits != 0);
        if(!JSON_isValid(o)) {
            fprintf(stderr, "Error\n");
            exit(EXIT_FAILURE);
        }
        else {
            size_t length;
            const char *text = JSON_toStringWithLength(o, &length);
            fprintf(stderr, "%s\n", text);
            free((char *)text);
        }
        JSON_free(o);
    }
    JSONMemoryPool_Delete(&jm);
    free(json);
    return 0;
}
