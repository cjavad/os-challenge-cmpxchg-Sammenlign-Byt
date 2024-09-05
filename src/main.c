#include <stdio.h>
#include "hash.h"
#include <string.h>

int main(int argc, char** argv) {
    hash_t h;

    int res = sha256_hash((uint8_t*) argv[1], strlen(argv[1]), h);
    // Print hash as hex
    debug_print_hash(h);

	return res;
}
