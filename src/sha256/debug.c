#include "sha256.h"

#include <stdio.h>

void sprintf_hash(char *str, const HashDigest hash) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(str + i * 2, "%02x", hash[i]);
    }
}

void print_hash(const HashDigest hash) {
    char str[SHA256_DIGEST_LENGTH * 2 + 1];
    sprintf_hash(str, hash);
    str[SHA256_DIGEST_LENGTH * 2] = '\0';
    printf("%s\n", str);
}