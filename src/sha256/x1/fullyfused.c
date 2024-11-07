#include "sha256x1.h"

#include "impl_common.h"

__attribute__((flatten)) void sha256_fullyfused(
    HashDigest hash,
    const HashInput data
) {
    uint32_t w[64];

    uint32_t a = 0x6a09e667;
    uint32_t b = 0xbb67ae85;
    uint32_t c = 0x3c6ef372;
    uint32_t d = 0xa54ff53a;
    uint32_t e = 0x510e527f;
    uint32_t f = 0x9b05688c;
    uint32_t g = 0x1f83d9ab;
    uint32_t h = 0x5be0cd19;

    // setting of initial block
    {
        const uint32_t w0 = __builtin_bswap32(((uint32_t*)data)[0]);
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[0] + w0;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
        w[0] = w0;
    }
    {
        const uint32_t w1 = __builtin_bswap32(((uint32_t*)data)[1]);
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[1] + w1;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
        w[1] = w1;
    }
    {
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[2] + 0x80000000;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
        w[2] = 0x80000000;
    }
    __builtin_memset(&w[3], 0, (15 - 3) * sizeof(uint32_t));
    for (uint32_t i = 3; i < 15; i++) {
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[i];
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    {
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[15] + 64;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
        w[15] = 64;
    }

    // optimization based on known
    uint32_t scra0 = w[0];
    uint32_t scra1 = w[1];

    {
        const uint32_t w0 = scra0;
        const uint32_t w1 = scra1;
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t w16 = w0 + s0(w1);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[16] + w16;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
        w[16] = w16;
        scra0 = w16;
    }
    {
        const uint32_t w1 = scra1;
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t w17 = w1 + s0(0x80000000) + s1(64);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[17] + w17;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
        w[17] = w17;
        scra1 = w17;
    }
    {
        const uint32_t w16 = scra0;
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t w18 = 0x80000000 + s1(w16);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[18] + w18;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
        w[18] = w18;
        scra0 = w18;
    }
    for (uint32_t i = 19; i < 22; i++) {
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t wi = s1(scra1);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[i] + wi;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;

        w[i] = wi;
        scra1 = scra0;
        scra0 = wi;
    }
    {
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t w22 = 64 + s1(scra1);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[22] + w22;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;

        w[22] = w22;
        scra1 = scra0;
        scra0 = w22;
    }
    for (uint32_t i = 23; i < 30; i++) {
        const uint32_t w7 = w[i - 7];
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t wi = s1(scra1) + w7;
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[i] + wi;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;

        w[i] = wi;
        scra1 = scra0;
        scra0 = wi;
    }
    {
        const uint32_t w23 = w[23];
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t w30 = s0(64) + s1(scra1) + w23;
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[30] + w30;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;

        w[30] = w30;
        scra1 = scra0;
        scra0 = w30;
    }
    {
        const uint32_t w16 = w[16];
        const uint32_t w24 = w[24];
        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t w31 = 64 + s1(scra1) + s0(w16) + w24;
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[31] + w31;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;

        w[31] = w31;
    }

    // loop rest
    for (uint32_t i = 32; i < 64; i++) {
        const uint32_t w16 = w[i - 16];
        const uint32_t w15 = w[i - 15];
        const uint32_t w7 = w[i - 7];
        const uint32_t w2 = w[i - 2];

        const uint32_t temp2 = S0(a) + maj(a, b, c);
        const uint32_t w0 = w16 + s0(w15) + w7 + s1(w2);
        const uint32_t temp1 = h + S1(e) + ch(e, f, g) + k[i] + w0;

        w[i] = w0;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    ((uint32_t*)hash)[0] = __builtin_bswap32(a + 0x6a09e667);
    ((uint32_t*)hash)[1] = __builtin_bswap32(b + 0xbb67ae85);
    ((uint32_t*)hash)[2] = __builtin_bswap32(c + 0x3c6ef372);
    ((uint32_t*)hash)[3] = __builtin_bswap32(d + 0xa54ff53a);
    ((uint32_t*)hash)[4] = __builtin_bswap32(e + 0x510e527f);
    ((uint32_t*)hash)[5] = __builtin_bswap32(f + 0x9b05688c);
    ((uint32_t*)hash)[6] = __builtin_bswap32(g + 0x1f83d9ab);
    ((uint32_t*)hash)[7] = __builtin_bswap32(h + 0x5be0cd19);
}