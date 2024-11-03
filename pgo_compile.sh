#!/bin/sh

CC=gcc
BIN=server_pgo
CFLAGS="-O3 -g -masm=intel -march=native -std=gnu11 -DRELEASE -lpthread -lrt -lm -I./src"
FILES=$(find src -name "*.c")

$CC $CFLAGS $FILES -o $BIN

# Run profiling
#./$BIN benchmark
#$CC $CFLAGS $FILES -fprofile-use -Werror=missing-profile -o $BIN

# Cleanup
#rm -f *.gcda
