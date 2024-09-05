#include "hash.h"
#include <openssl/evp.h>

void debug_print_hash(hash_u hash) {
    for (size_t i = 0; i < 32; i++) {
        printf("%02x", hash.u8[i]);
    }
}
// SHA256
int sha256_hash(const uint8_t* data, const size_t len, hash_u hash) {
    unsigned int digest_length = -1;

    // Create a message digest context
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();

    if (mdctx == NULL) {
        printf("Error creating context\n");
        goto failed;
    }

    // Use SHA-256 as the digest type
    const EVP_MD *md = EVP_sha256();

    // Initialize the digest operation
    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        printf("Error initializing digest\n");
        EVP_MD_CTX_free(mdctx);
        goto failed;
    }

    // Provide the data to hash
    if (EVP_DigestUpdate(mdctx, data, len) != 1) {
        printf("Error updating digest\n");
        EVP_MD_CTX_free(mdctx);
        goto failed;
    }

    // Finalize the digest operation and obtain the result
    if (EVP_DigestFinal_ex(mdctx, hash.u8, &digest_length) != 1) {
        printf("Error finalizing digest\n");
        goto failed;
    }

    goto end;

    // Clean up the message digest context
    failed:
        if (mdctx != NULL) EVP_MD_CTX_free(mdctx);
        return -1;

    end:
    EVP_MD_CTX_free(mdctx);
    return digest_length;
}
