#include "sha256.h"

#include "impl_common.h"

__attribute__((flatten))
void sha256_optim(HashDigest hash, const HashInput data) {
    uint32_t w[64]  __attribute__((aligned(64)));

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