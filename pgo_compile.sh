#!/bin/sh

CC=gcc
BIN=server_pgo
CFLAGS="-O3 -flto=auto -march=native -masm=intel -std=gnu11 -D_GNU_SOURCE -DRELEASE -lpthread -lrt -lm -Wl,-rpath,lib -I./src"
FILES=$(find src -name "*.c")

$CC $CFLAGS $FILES -flto -fprofile-generate -o $BIN

# Run profiling
./$BIN benchmark

$CC $CFLAGS $FILES -fprofile-use -Werror=missing-profile -o $BIN

# Cleanup
rm -f *.gcda
