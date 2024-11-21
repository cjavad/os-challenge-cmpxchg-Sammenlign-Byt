#include "sha256x1.h"

// Precompute SHA-256 constants and initialization vector, keep in memory
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const uint32_t H_INIT[8] = {
    0x6a09e667,
    0xbb67ae85,
    0x3c6ef372,
    0xa54ff53a,
    0x510e527f,
    0x9b05688c,
    0x1f83d9ab,
    0x5be0cd19
};

// Inline circular right shift
#define ROTR(x, n) ((x >> n) | (x << (32 - n)))

// Inline SHA-256 functions
#define Ch(x, y, z)  ((x & y) ^ (~x & z))
#define Maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define Sigma0(x)    (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define Sigma1(x)    (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sigma0(x)    (ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3))
#define sigma1(x)    (ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10))

// Optimized SHA-256 padding and preprocessing for 64-bit data (8 bytes)
// Optimized final hash calculation for fixed 64-bit input
void sha256_custom(
    HashDigest hash,
    const HashInput data
) {
    uint32_t W[64];

    uint32_t s[8]; // internal state

    for (int i = 0; i < 8; i++) {
        s[i] = H_INIT[i];
    }

    // Precompute W[0..15]
    W[0] = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
    W[1] = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | (data[7]);
    W[2] = 0x80000000; // Padding for fixed length
    for (int i = 3; i < 15; i++) {
        W[i] = 0;
    }
    W[15] = 64; // 64-bit message length

    // Extend W[16..63]
    for (int i = 16; i < 64; i++) {
        W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
    }

    // Main compression loop, use circular buffer to avoid shifts
    for (int i = 0; i < 64; i++) {
        const uint32_t T1 =
            s[7] + Sigma1(s[4]) + Ch(s[4], s[5], s[6]) + K[i] + W[i];
        const uint32_t T2 = Sigma0(s[0]) + Maj(s[0], s[1], s[2]);

        // Rotate the state variables in place
        s[7] = s[6];
        s[6] = s[5];
        s[5] = s[4];
        s[4] = s[3] + T1;
        s[3] = s[2];
        s[2] = s[1];
        s[1] = s[0];
        s[0] = T1 + T2;
    }

    // Output the final state (big endian)
    for (int i = 0; i < 8; i++) {
        *(uint32_t*)(hash + i * 4) = s[i];
    }
}