#include <stdio.h>
#include <stdlib.h>

#ifndef KJSON_TEST_H
#define KJSON_TEST_H

static char *load_file(const char *path, size_t *size)
{
    FILE *fp = fopen(path, "r");
    assert(fp != 0);

    fseek(fp, 0, SEEK_END);
    size_t len = (size_t) ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *json = (char *) calloc(1, len + 1);
    size_t readed = fread(json, 1, len, fp);
    assert(len == readed);
    json[len] = '\0';
    fclose(fp);
    *size = len;
    return json;
    (void)readed;
}

#endif /* end of include guard */
