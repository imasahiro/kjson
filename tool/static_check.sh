#!/bin/sh

SOURCE=
SOURCE="${SOURCE} kjson.c"
SOURCE="${SOURCE} map.c"

CFLAGS="-I."
cppcheck -q --force ${CFLAGS} --enable=all ${SOURCE}
scan-build -o /tmp gcc -Wall ${SOURCE} ./test/test_kjson.c ${CFLAGS}
