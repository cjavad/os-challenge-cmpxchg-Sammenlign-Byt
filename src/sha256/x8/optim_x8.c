#include "sha256x8.h"

#ifndef NO_AVX
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>

// TODO :: this file is a mess i need to clean it

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
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const uint32_t H[8] __attribute__((aligned(16))) = {
    0x6a09e667,
    0xbb67ae85,
    0x3c6ef372,
    0xa54ff53a,
    0x510e527f,
    0x9b05688c,
    0x1f83d9ab,
    0x5be0cd19
};

static __m256i rotr(
    const __m128i lower,
    const __m128i upper,
    const uint32_t n
) {
    return _mm256_insertf128_si256(
        _mm256_castsi128_si256(_mm_or_si128(
            _mm_srli_epi32(lower, n),
            _mm_slli_epi32(lower, 32 - n)
        )),
        _mm_or_si128(_mm_srli_epi32(upper, n), _mm_slli_epi32(upper, 32 - n)),
        1
    );
}

static __m256i s0(
    const __m256i w
) {
    __m128i upper = _mm256_extractf128_si256(w, 1);
    __m128i lower = _mm256_castsi256_si128(w);

    __m256i t1 = rotr(lower, upper, 18);
    __m256i t2 = _mm256_insertf128_si256(
        _mm256_castsi128_si256(_mm_srli_epi32(lower, 3)),
        _mm_srli_epi32(upper, 3),
        1
    );
    __m256i t0 = rotr(lower, upper, 7);

    return _mm256_castps_si256(_mm256_xor_ps(
        _mm256_castsi256_ps(t0),
        _mm256_xor_ps(_mm256_castsi256_ps(t1), _mm256_castsi256_ps(t2))
    ));
}

static __m256i s1(
    const __m256i w
) {
    __m128i upper = _mm256_extractf128_si256(w, 1);
    __m128i lower = _mm256_castsi256_si128(w);

    __m256i t1 = rotr(lower, upper, 19);
    __m256i t2 = _mm256_insertf128_si256(
        _mm256_castsi128_si256(_mm_srli_epi32(lower, 10)),
        _mm_srli_epi32(upper, 10),
        1
    );
    __m256i t0 = rotr(lower, upper, 17);

    return _mm256_castps_si256(_mm256_xor_ps(
        _mm256_castsi256_ps(t0),
        _mm256_xor_ps(_mm256_castsi256_ps(t1), _mm256_castsi256_ps(t2))
    ));
}

static __m256i S0(
    const __m256i a
) {
    __m128i upper = _mm256_extractf128_si256(a, 1);
    __m128i lower = _mm256_castsi256_si128(a);

    return _mm256_castps_si256(_mm256_xor_ps(
        _mm256_castsi256_ps(rotr(lower, upper, 2)),
        _mm256_xor_ps(
            _mm256_castsi256_ps(rotr(lower, upper, 13)),
            _mm256_castsi256_ps(rotr(lower, upper, 22))
        )
    ));
}

static __m256i S1(
    const __m256i e
) {
    __m128i upper = _mm256_extractf128_si256(e, 1);
    __m128i lower = _mm256_castsi256_si128(e);

    return _mm256_castps_si256(_mm256_xor_ps(
        _mm256_castsi256_ps(rotr(lower, upper, 6)),
        _mm256_xor_ps(
            _mm256_castsi256_ps(rotr(lower, upper, 11)),
            _mm256_castsi256_ps(rotr(lower, upper, 25))
        )
    ));
}

static __m256i ch(
    const __m256i e,
    const __m256i f,
    const __m256i g
) {
    return _mm256_castps_si256(_mm256_xor_ps(
        _mm256_and_ps(_mm256_castsi256_ps(e), _mm256_castsi256_ps(f)),
        _mm256_andnot_ps(_mm256_castsi256_ps(e), _mm256_castsi256_ps(g))
    ));
}

static __m256i maj(
    const __m256i a,
    const __m256i b,
    const __m256i c
) {
    return _mm256_castps_si256(_mm256_xor_ps(
        _mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)),
        _mm256_xor_ps(
            _mm256_and_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(c)),
            _mm256_and_ps(_mm256_castsi256_ps(b), _mm256_castsi256_ps(c))
        )
    ));
}

static __m256i _mm256_fake_add_epi32(
    const __m256i x,
    const __m256i y
) {
    return _mm256_insertf128_si256(
        _mm256_castsi128_si256(
            _mm_add_epi32(_mm256_castsi256_si128(x), _mm256_castsi256_si128(y))
        ),
        _mm_add_epi32(
            _mm256_extractf128_si256(x, 1),
            _mm256_extractf128_si256(y, 1)
        ),
        1
    );
}

static const uint8_t byteswap_mask0[16] __attribute__((aligned(16))
) = {3, 2, 1, 0, 11, 10, 9, 8, 7, 6, 5, 4, 15, 14, 13, 12};

static const uint8_t byteswap_mask1[16] __attribute__((aligned(16))
) = {7, 6, 5, 4, 15, 14, 13, 12, 3, 2, 1, 0, 11, 10, 9, 8};

static const uint8_t byteswap_mask[16] __attribute__((aligned(16))
) = {3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12};

static const uint32_t x80000000[4] __attribute__((aligned(16))
) = {0x80000000, 0x80000000, 0x80000000, 0x80000000};

static const uint32_t d64[4] __attribute__((aligned(16))) = {64, 64, 64, 64};

__attribute__((flatten)) void sha256x8_optim(
    uint8_t hash[SHA256_DIGEST_LENGTH * 8],
    const uint8_t data[SHA256_INPUT_LENGTH * 8]
) {
    __m256i w[64] __attribute__((aligned(32)));

    register __m256i t0;
    register __m256i t1;

    {
        // can't byte swap with m256, need to use m128
        __m128i mask0 = _mm_load_si128((__m128i*)byteswap_mask0),
                mask1 = _mm_load_si128((__m128i*)&byteswap_mask1),
                ll0 = _mm_load_si128((__m128i*)&data[0]),
                ll1 = _mm_load_si128((__m128i*)&data[16]),
                lu0 = _mm_load_si128((__m128i*)&data[32]),
                lu1 = _mm_load_si128((__m128i*)&data[48]);

        __m128i hl0 = _mm_shuffle_epi8(ll0, mask0),
                hl1 = _mm_shuffle_epi8(ll1, mask1),
                hu0 = _mm_shuffle_epi8(lu0, mask0),
                hu1 = _mm_shuffle_epi8(lu1, mask1);

        __m256i
            h0 = _mm256_insertf128_si256(_mm256_castsi128_si256(hl0), hl1, 1),
            h1 = _mm256_insertf128_si256(_mm256_castsi128_si256(hu0), hu1, 1);

        __m256i w0 = _mm256_castps_si256(_mm256_blend_ps(
                    _mm256_castsi256_ps(h0),
                    _mm256_castsi256_ps(h1),
                    0b11001100
                )),
                w1 = _mm256_castps_si256(_mm256_blend_ps(
                    _mm256_castsi256_ps(h0),
                    _mm256_castsi256_ps(h1),
                    0b00110011
                ));

        _mm256_store_si256(&w[0], w0);
        _mm256_store_si256(&w[1], w1);

        t0 = w0;
        t1 = w1;
    }

    __m256i c80, c64;
    {
        __m128i l0 = _mm_load_si128((__m128i*)x80000000),
                l1 = _mm_load_si128((__m128i*)d64);

        c80 = _mm256_insertf128_si256(_mm256_castsi128_si256(l0), l0, 1);
        c64 = _mm256_insertf128_si256(_mm256_castsi128_si256(l1), l1, 1);
    }

    __builtin_memset(&w[3], 0, (15 - 3) * sizeof(__m256i));

    _mm256_store_si256(&w[2], c80);
    _mm256_store_si256(&w[15], c64);

    {
        __m256i w16 = _mm256_fake_add_epi32(t0, s0(t1));
        _mm256_store_si256(&w[16], w16);
        t0 = w16;
    }
    {
        __m256i w17 =
            _mm256_fake_add_epi32(t1, _mm256_fake_add_epi32(s0(c80), s1(c64)));
        _mm256_store_si256(&w[17], w17);
        t1 = w17;
    }
    {
        __m256i w18 = _mm256_fake_add_epi32(c80, s1(t0));
        _mm256_store_si256(&w[18], w18);
        t0 = w18;
    }
    {
        __m256i w19 = s1(t1);
        _mm256_store_si256(&w[19], w19);
        t1 = w19;

        __m256i w20 = s1(t0);
        _mm256_store_si256(&w[20], w20);
        t0 = w20;

        __m256i w21 = s1(t1);
        _mm256_store_si256(&w[21], w21);
        t1 = w21;
    }
    {
        __m256i w22 = _mm256_fake_add_epi32(c64, s1(t0));
        _mm256_store_si256(&w[22], w22);
        t0 = w22;
    }

    {
        register __m256i l0 = _mm256_load_si256(&w[16]);
        register __m256i l1 = _mm256_load_si256(&w[17]);

        __m256i w23 = _mm256_fake_add_epi32(l0, s1(t1));
        _mm256_store_si256(&w[23], w23);
        t1 = w23;
        l0 = _mm256_load_si256(&w[18]);

        __m256i w24 = _mm256_fake_add_epi32(l1, s1(t0));
        _mm256_store_si256(&w[24], w24);
        t0 = w24;
        l1 = _mm256_load_si256(&w[19]);

        __m256i w25 = _mm256_fake_add_epi32(l0, s1(t1));
        _mm256_store_si256(&w[25], w25);
        t1 = w25;
        l0 = _mm256_load_si256(&w[20]);

        __m256i w26 = _mm256_fake_add_epi32(l1, s1(t0));
        _mm256_store_si256(&w[26], w26);
        t0 = w26;
        l1 = _mm256_load_si256(&w[21]);

        __m256i w27 = _mm256_fake_add_epi32(l0, s1(t1));
        _mm256_store_si256(&w[27], w27);
        t1 = w27;
        l0 = _mm256_load_si256(&w[22]);

        __m256i w28 = _mm256_fake_add_epi32(l1, s1(t0));
        _mm256_store_si256(&w[28], w28);
        t0 = w28;
        l1 = _mm256_load_si256(&w[23]);

        __m256i w29 = _mm256_fake_add_epi32(l0, s1(t1));
        _mm256_store_si256(&w[29], w29);
        t1 = w29;
        l0 = _mm256_load_si256(&w[24]);

        __m256i w30 =
            _mm256_fake_add_epi32(s0(c64), _mm256_fake_add_epi32(l1, s1(t0)));
        _mm256_store_si256(&w[30], w30);
        t0 = w30;
        l1 = _mm256_load_si256(&w[16]);

        __m256i w31 = _mm256_fake_add_epi32(
            _mm256_fake_add_epi32(l0, s1(t1)),
            _mm256_fake_add_epi32(c64, s0(l1))
        );
        _mm256_store_si256(&w[31], w31);
        t1 = w31;
    }

    for (uint32_t i = 32; i < 64; i++) {
        __m256i wi16 = _mm256_load_si256(&w[i - 16]),
                wi15 = _mm256_load_si256(&w[i - 15]),
                wi7 = _mm256_load_si256(&w[i - 7]), wi2 = t0;
        t0 = t1;

        __m256i ws1 = s1(wi2);
        __m256i ws0 = s0(wi15);
        __m256i wi = _mm256_fake_add_epi32(
            _mm256_fake_add_epi32(wi16, wi7),
            _mm256_fake_add_epi32(ws0, ws1)
        );
        _mm256_store_si256(&w[i], wi);
        t1 = wi;
    }

    __m256 h0 = _mm256_broadcast_ps((__m128*)&H[0]),
           h1 = _mm256_broadcast_ps((__m128*)&H[4]);

    __m256i a = _mm256_castps_si256(_mm256_permute_ps(h0, 0b00000000)),
            b = _mm256_castps_si256(_mm256_permute_ps(h0, 0b01010101)),
            c = _mm256_castps_si256(_mm256_permute_ps(h0, 0b10101010)),
            d = _mm256_castps_si256(_mm256_permute_ps(h0, 0b11111111)),
            e = _mm256_castps_si256(_mm256_permute_ps(h1, 0b00000000)),
            f = _mm256_castps_si256(_mm256_permute_ps(h1, 0b01010101)),
            g = _mm256_castps_si256(_mm256_permute_ps(h1, 0b10101010)),
            h = _mm256_castps_si256(_mm256_permute_ps(h1, 0b11111111));

    for (uint32_t i = 0; i < 64; i++) {
        __m256i ki = _mm256_castps_si256(_mm256_broadcast_ss((float*)&k[i])),
                wi = _mm256_load_si256(&w[i]);

        __m256i temp2 = _mm256_fake_add_epi32(S0(a), maj(a, b, c)),
                temp1 = _mm256_fake_add_epi32(
                    _mm256_fake_add_epi32(S1(e), ch(e, f, g)),
                    _mm256_fake_add_epi32(h, _mm256_fake_add_epi32(ki, wi))
                );

        h = g;
        g = f;
        f = e;
        e = _mm256_fake_add_epi32(d, temp1);
        d = c;
        c = b;
        b = a;
        a = _mm256_fake_add_epi32(temp1, temp2);
    }

    a = _mm256_fake_add_epi32(
        a,
        _mm256_castps_si256(_mm256_broadcast_ss((float*)&H[0]))
    );
    b = _mm256_fake_add_epi32(
        b,
        _mm256_castps_si256(_mm256_broadcast_ss((float*)&H[1]))
    );
    c = _mm256_fake_add_epi32(
        c,
        _mm256_castps_si256(_mm256_broadcast_ss((float*)&H[2]))
    );
    d = _mm256_fake_add_epi32(
        d,
        _mm256_castps_si256(_mm256_broadcast_ss((float*)&H[3]))
    );
    e = _mm256_fake_add_epi32(
        e,
        _mm256_castps_si256(_mm256_broadcast_ss((float*)&H[4]))
    );
    f = _mm256_fake_add_epi32(
        f,
        _mm256_castps_si256(_mm256_broadcast_ss((float*)&H[5]))
    );
    g = _mm256_fake_add_epi32(
        g,
        _mm256_castps_si256(_mm256_broadcast_ss((float*)&H[6]))
    );
    h = _mm256_fake_add_epi32(
        h,
        _mm256_castps_si256(_mm256_broadcast_ss((float*)&H[7]))
    );

    /*
        1 2
        3 4

        trans(1) trans(2)
        trans(3) trans(4)

        need swap for (unpacks work on 128 bit lanes)

        trans(1) trans(3)
        trans(2) trans(4)

        same as

        trans(
            1 2
            3 4
        )
    */

    __m256
        tmp00 =
            _mm256_unpacklo_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)),
        tmp01 =
            _mm256_unpackhi_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b)),
        tmp02 =
            _mm256_unpacklo_ps(_mm256_castsi256_ps(c), _mm256_castsi256_ps(d)),
        tmp03 =
            _mm256_unpackhi_ps(_mm256_castsi256_ps(c), _mm256_castsi256_ps(d)),
        tmp10 =
            _mm256_unpacklo_ps(_mm256_castsi256_ps(e), _mm256_castsi256_ps(f)),
        tmp11 =
            _mm256_unpackhi_ps(_mm256_castsi256_ps(e), _mm256_castsi256_ps(f)),
        tmp12 =
            _mm256_unpacklo_ps(_mm256_castsi256_ps(g), _mm256_castsi256_ps(h)),
        tmp13 =
            _mm256_unpackhi_ps(_mm256_castsi256_ps(g), _mm256_castsi256_ps(h));

    // rows without swap
    __m256i
        r0 = _mm256_castps_si256(_mm256_shuffle_ps(tmp00, tmp01, 0b01000100)),
        r1 = _mm256_castps_si256(_mm256_shuffle_ps(tmp00, tmp01, 0b11101110)),
        r2 = _mm256_castps_si256(_mm256_shuffle_ps(tmp02, tmp03, 0b01000100)),
        r3 = _mm256_castps_si256(_mm256_shuffle_ps(tmp02, tmp03, 0b11101110)),
        r4 = _mm256_castps_si256(_mm256_shuffle_ps(tmp10, tmp11, 0b01000100)),
        r5 = _mm256_castps_si256(_mm256_shuffle_ps(tmp10, tmp11, 0b11101110)),
        r6 = _mm256_castps_si256(_mm256_shuffle_ps(tmp12, tmp13, 0b01000100)),
        r7 = _mm256_castps_si256(_mm256_shuffle_ps(tmp12, tmp13, 0b11101110));

    __m128i mask = _mm_load_si128((__m128i*)byteswap_mask);

    // breaking stores up from 256 to 128 probably helps memory bandwith,
    // especially on 2012 CPU
    _mm_store_si128(
        (__m128i*)&hash[0],
        _mm_shuffle_epi8(_mm256_extractf128_si256(r0, 1), mask)
    );
    _mm_store_si128(
        (__m128i*)&hash[16],
        _mm_shuffle_epi8(_mm256_castsi256_si128(r0), mask)
    );

    _mm_store_si128(
        (__m128i*)&hash[32],
        _mm_shuffle_epi8(_mm256_extractf128_si256(r1, 1), mask)
    );
    _mm_store_si128(
        (__m128i*)&hash[48],
        _mm_shuffle_epi8(_mm256_castsi256_si128(r1), mask)
    );

    _mm_store_si128(
        (__m128i*)&hash[64],
        _mm_shuffle_epi8(_mm256_extractf128_si256(r2, 1), mask)
    );
    _mm_store_si128(
        (__m128i*)&hash[80],
        _mm_shuffle_epi8(_mm256_castsi256_si128(r2), mask)
    );

    _mm_store_si128(
        (__m128i*)&hash[96],
        _mm_shuffle_epi8(_mm256_extractf128_si256(r3, 1), mask)
    );
    _mm_store_si128(
        (__m128i*)&hash[112],
        _mm_shuffle_epi8(_mm256_castsi256_si128(r3), mask)
    );

    _mm_store_si128(
        (__m128i*)&hash[128],
        _mm_shuffle_epi8(_mm256_extractf128_si256(r4, 1), mask)
    );
    _mm_store_si128(
        (__m128i*)&hash[144],
        _mm_shuffle_epi8(_mm256_castsi256_si128(r4), mask)
    );

    _mm_store_si128(
        (__m128i*)&hash[160],
        _mm_shuffle_epi8(_mm256_extractf128_si256(r5, 1), mask)
    );
    _mm_store_si128(
        (__m128i*)&hash[176],
        _mm_shuffle_epi8(_mm256_castsi256_si128(r5), mask)
    );

    _mm_store_si128(
        (__m128i*)&hash[192],
        _mm_shuffle_epi8(_mm256_extractf128_si256(r6, 1), mask)
    );
    _mm_store_si128(
        (__m128i*)&hash[208],
        _mm_shuffle_epi8(_mm256_castsi256_si128(r6), mask)
    );

    _mm_store_si128(
        (__m128i*)&hash[224],
        _mm_shuffle_epi8(_mm256_extractf128_si256(r7, 1), mask)
    );
    _mm_store_si128(
        (__m128i*)&hash[240],
        _mm_shuffle_epi8(_mm256_castsi256_si128(r7), mask)
    );
}
#else
void sha256x8_optim(
    uint8_t hash[SHA256_DIGEST_LENGTH * 8],
    const uint8_t data[SHA256_INPUT_LENGTH * 8]
) {
    (void)hash;
    (void)data;
}
#endif