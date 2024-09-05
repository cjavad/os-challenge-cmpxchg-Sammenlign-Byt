#include "hash.h"
#include <openssl/evp.h>

void debug_print_hash(const hash_t hash) {
    for (size_t i = 0; i < sizeof(hash_t); i++) {
        printf("%02x", hash[i]);
    }
}
// SHA256
int sha256_hash(const uint8_t* data, size_t len, hash_t hash) {
    return EVP_Q_digest(NULL, "SHA256", NULL, data, len, hash, NULL);
}
