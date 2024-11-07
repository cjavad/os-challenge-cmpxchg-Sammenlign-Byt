#!/bin/sh

CC=gcc
BIN=pgo_server
CFLAGS="-O1 -g -masm=intel -march=sandybridge -mtune=sandybridge -std=gnu11 -DRELEASE -lpthread -lrt -lm -I./src"
FILES=$(find src -name "*.c")

$CC $CFLAGS $FILES -DPROFILE_GENERATION -fprofile-generate -o $BIN

./$BIN 8080 &
SERVER_PID=$!

./run-client.sh midway

kill -2 $SERVER_PID
wait $SERVER_PID

$CC $CFLAGS $FILES -fprofile-use -Werror=missing-profile -Wno-error=coverage-mismatch -o $BIN

rm -f *.gcda
