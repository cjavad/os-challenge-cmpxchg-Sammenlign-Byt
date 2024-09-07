#include "hash.h"

#include <openssl/evp.h>
#include <string.h>

void debug_print_hash(hash_u hash) {
    for (size_t i = 0; i < 32; i++) {
        printf("%02x", hash.u8[i]);
    }
}
// SHA256
int sha256_hash(const uint8_t *data, const size_t len, hash_u *hash) {
    unsigned long hash_len; // Length of the hash output
    unsigned char h[EVP_MAX_MD_SIZE];
    EVP_Q_digest(NULL, "SHA256", NULL, data, len, h, &hash_len);
    memcpy(hash->u8, h, 32);
    return 0;
}

uint64_t reverse_hash(uint64_t start, uint64_t end, hash_u target) {

    for (uint64_t i = start; i < end; i++) {
        hash_u out;

        printf("i: %lu\n", i);

        // convert uint64_t to uint8_t

        uint8_t data[8];

        for (size_t j = 0; j < 8; j++) {
            data[j] = (i >> (j * 8)) & 0xff;
        }

        sha256_hash(data, 8, &out);

        printf("Input: ");

        for (size_t j = 0; j < 8; j++) {
            printf(" %02x ", data[j]);
        }

        printf(" comparison ");
        debug_print_hash(target);
        printf(" <-> ");
        debug_print_hash(out);
        printf("\n");

        if (memcmp(out.u8, target.u8, 32) == 0) {
            return i;
        }
    }

    return 0;
}