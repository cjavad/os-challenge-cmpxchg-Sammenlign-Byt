#include "sha256.h"

#include <openssl/evp.h>

SHA256_GCC_OPT_ATTR
void sha256_openssl(HashDigest hash, const HashInput data) {
    EVP_Q_digest(NULL, "SHA256", NULL, data, SHA256_INPUT_LENGTH, hash, NULL);
}