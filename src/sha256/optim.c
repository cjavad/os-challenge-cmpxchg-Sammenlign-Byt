#include "sha256.h"

static const uint32_t k[64] = {
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
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static uint32_t rotr(const uint32_t x, const uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static uint32_t s0(const uint32_t w) { return rotr(w, 7) ^ rotr(w, 18) ^ (w >> 3); }

static uint32_t s1(const uint32_t w) { return rotr(w, 17) ^ rotr(w, 19) ^ (w >> 10); }

__attribute__((flatten))
void sha256_optim(HashDigest hash, const HashInput data) {
    uint32_t w[64];

    // setting of initial block
    w[0] = __builtin_bswap32(((uint32_t *)data)[0]);
    w[1] = __builtin_bswap32(((uint32_t *)data)[1]);
    w[2] = 0x80000000;
    __builtin_memset(&w[3], 0, (15 - 3) * sizeof(uint32_t));
    w[15] = 64;

    // optimization based on known
    w[16] = w[0] + s0(w[1]);

    w[17] = w[1] + s0(0x80000000) + s1(64);

    w[18] = 0x80000000 + s1(w[16]);

    w[19] = s1(w[17]);
    w[20] = s1(w[18]);
    w[21] = s1(w[19]);

    w[22] = 64 + s1(w[20]);

    w[23] = w[16] + s1(w[21]);
    w[24] = w[17] + s1(w[22]);
    w[25] = w[18] + s1(w[23]);
    w[26] = w[19] + s1(w[24]);
    w[27] = w[20] + s1(w[25]);
    w[28] = w[21] + s1(w[26]);
    w[29] = w[22] + s1(w[27]);

    w[30] = s0(64) + w[23] + s1(w[28]);

    w[31] = 64 + s0(w[16]) + w[24] + s1(w[29]);


    // loop rest
    for (uint32_t i = 32; i < 64; i++) {
        w[i] = w[i - 16] + s0(w[i - 15]) + w[i - 7] + s1(w[i - 2]);
    }

    uint32_t a = 0x6a09e667;
    uint32_t b = 0xbb67ae85;
    uint32_t c = 0x3c6ef372;
    uint32_t d = 0xa54ff53a;
    uint32_t e = 0x510e527f;
    uint32_t f = 0x9b05688c;
    uint32_t g = 0x1f83d9ab;
    uint32_t h = 0x5be0cd19;

    // compression loop
    for (uint32_t i = 0; i < 64; i++) {
        const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const uint32_t ch = (e & f) ^ ((~e) & g);
        const uint32_t temp1 = h + S1 + ch + k[i] + w[i];
        const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    ((uint32_t *)hash)[0] = __builtin_bswap32(a + 0x6a09e667);
    ((uint32_t *)hash)[1] = __builtin_bswap32(b + 0xbb67ae85);
    ((uint32_t *)hash)[2] = __builtin_bswap32(c + 0x3c6ef372);
    ((uint32_t *)hash)[3] = __builtin_bswap32(d + 0xa54ff53a);
    ((uint32_t *)hash)[4] = __builtin_bswap32(e + 0x510e527f);
    ((uint32_t *)hash)[5] = __builtin_bswap32(f + 0x9b05688c);
    ((uint32_t *)hash)[6] = __builtin_bswap32(g + 0x1f83d9ab);
    ((uint32_t *)hash)[7] = __builtin_bswap32(h + 0x5be0cd19);
}