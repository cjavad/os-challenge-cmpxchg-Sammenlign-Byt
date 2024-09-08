#include "sha256.h"

#include <openssl/evp.h>

void sha256_openssl(HashDigest hash, const HashInput data) {
    EVP_Q_digest(NULL, "SHA256", NULL, data, 8, hash, NULL);
}