#include "sha256x4x2.h"

#ifndef NO_AVX
#include "impl_common.h"

__attribute__((flatten)) void sha256x4x2_fused(
    uint8_t hash[SHA256_DIGEST_LENGTH * 8], const uint8_t data[SHA256_INPUT_LENGTH * 8]
) {
    __m128i w[64 * 2] __attribute__((aligned(32)));

    register __m128i t0l;
    register __m128i t0u;
    register __m128i t1l;
    register __m128i t1u;

    {
        // can't byte swap with m256, need to use m128
        __m128i mask0 = _mm_load_si128((__m128i*)byteswap_load0), mask1 = _mm_load_si128((__m128i*)&byteswap_load1),
                ll0 = _mm_load_si128((__m128i*)&data[0]), ll1 = _mm_load_si128((__m128i*)&data[16]),
                lu0 = _mm_load_si128((__m128i*)&data[32]), lu1 = _mm_load_si128((__m128i*)&data[48]);

        __m128i hl0 = _mm_shuffle_epi8(ll0, mask0), hl1 = _mm_shuffle_epi8(ll1, mask0),
                hu0 = _mm_shuffle_epi8(lu0, mask1), hu1 = _mm_shuffle_epi8(lu1, mask1);

        __m256 h0 = M128I_FUSE_PS(hl0, hl1), h1 = M128I_FUSE_PS(hu0, hu1);

        __m256 w0 = _mm256_blend_ps(h0, h1, 0b11001100), w1 = _mm256_blend_ps(h0, h1, 0b00110011);

        _mm256_store_ps((float*)&w[0 * 2], w0);
        _mm256_store_ps((float*)&w[1 * 2], w1);

        t0u = M256_UPPER_SI(w0);
        t0l = M256_LOWER_SI(w0);
        t1u = M256_UPPER_SI(w1);
        t1l = M256_LOWER_SI(w1);
    }

    __m128i c80 = _mm_load_si128((__m128i*)x80000000), c64 = _mm_load_si128((__m128i*)d64);

    __builtin_memset(&w[3 * 2], 0, (15 - 3) * sizeof(__m256));

    _mm256_store_ps((float*)&w[2 * 2], M128I_FUSE_PS(c80, c80));
    _mm256_store_ps((float*)&w[15 * 2], M128I_FUSE_PS(c64, c64));

    {
        __m128i w16l = _mm_add_epi32(t0l, m128_s0(t1l));
        __m128i w16u = _mm_add_epi32(t0u, m128_s0(t1u));
        _mm_store_si128(&w[16 * 2], w16l);
        _mm_store_si128(&w[16 * 2 + 1], w16u);
        t0l = w16l;
        t0u = w16u;
    }
    {
        __m128i w17l = _mm_add_epi32(t1l, _mm_add_epi32(m128_s0(c80), m128_s1(c64)));
        __m128i w17u = _mm_add_epi32(t1u, _mm_add_epi32(m128_s0(c80), m128_s1(c64)));
        _mm_store_si128(&w[17 * 2], w17l);
        _mm_store_si128(&w[17 * 2 + 1], w17u);
        t1l = w17l;
        t1u = w17u;
    }
    {
        __m128i w18l = _mm_add_epi32(c80, m128_s1(t0l));
        __m128i w18u = _mm_add_epi32(c80, m128_s1(t0u));
        _mm_store_si128(&w[18 * 2], w18l);
        _mm_store_si128(&w[18 * 2 + 1], w18u);
        t0l = w18l;
        t0u = w18u;
    }
    {
        __m128i w19l = m128_s1(t1l);
        __m128i w19u = m128_s1(t1u);
        _mm_store_si128(&w[19 * 2], w19l);
        _mm_store_si128(&w[19 * 2 + 1], w19u);
        t1l = w19l;
        t1u = w19u;

        __m128i w20l = m128_s1(t0l);
        __m128i w20u = m128_s1(t0u);
        _mm_store_si128(&w[20 * 2], w20l);
        _mm_store_si128(&w[20 * 2 + 1], w20u);
        t0l = w20l;
        t0u = w20u;

        __m128i w21l = m128_s1(t1l);
        __m128i w21u = m128_s1(t1u);
        _mm_store_si128(&w[21 * 2], w21l);
        _mm_store_si128(&w[21 * 2 + 1], w21u);
        t1l = w21l;
        t1u = w21u;
    }
    {
        __m128i w22l = _mm_add_epi32(c64, m128_s1(t0l));
        __m128i w22u = _mm_add_epi32(c64, m128_s1(t0u));
        _mm_store_si128(&w[22 * 2], w22l);
        _mm_store_si128(&w[22 * 2 + 1], w22u);
        t0l = w22l;
        t0u = w22u;
    }
    {
        register __m256 l0 = _mm256_load_ps((float*)&w[16 * 2]);
        register __m256 l1 = _mm256_load_ps((float*)&w[17 * 2]);

        register __m128i lu = M256_UPPER_SI(l0);
        register __m128i ll = M256_LOWER_SI(l0);

        __m128i w23l = _mm_add_epi32(ll, m128_s1(t1l));
        __m128i w23u = _mm_add_epi32(lu, m128_s1(t1u));
        _mm_store_si128(&w[23 * 2], w23l);
        _mm_store_si128(&w[23 * 2 + 1], w23u);
        t1l = w23l;
        t1u = w23u;
        l0 = _mm256_load_ps((float*)&w[18 * 2]);

        lu = M256_UPPER_SI(l1);
        ll = M256_LOWER_SI(l1);
        __m128i w24l = _mm_add_epi32(ll, m128_s1(t0l));
        __m128i w24u = _mm_add_epi32(lu, m128_s1(t0u));
        _mm_store_si128(&w[24 * 2], w24l);
        _mm_store_si128(&w[24 * 2 + 1], w24u);
        t0l = w24l;
        t0u = w24u;
        l1 = _mm256_load_ps((float*)&w[19 * 2]);

        lu = M256_UPPER_SI(l0);
        ll = M256_LOWER_SI(l0);
        __m128i w25l = _mm_add_epi32(ll, m128_s1(t1l));
        __m128i w25u = _mm_add_epi32(lu, m128_s1(t1u));
        _mm_store_si128(&w[25 * 2], w25l);
        _mm_store_si128(&w[25 * 2 + 1], w25u);
        t1l = w25l;
        t1u = w25u;
        l0 = _mm256_load_ps((float*)&w[20 * 2]);

        lu = M256_UPPER_SI(l1);
        ll = M256_LOWER_SI(l1);
        __m128i w26l = _mm_add_epi32(ll, m128_s1(t0l));
        __m128i w26u = _mm_add_epi32(lu, m128_s1(t0u));
        _mm_store_si128(&w[26 * 2], w26l);
        _mm_store_si128(&w[26 * 2 + 1], w26u);
        t0l = w26l;
        t0u = w26u;
        l1 = _mm256_load_ps((float*)&w[21 * 2]);

        lu = M256_UPPER_SI(l0);
        ll = M256_LOWER_SI(l0);
        __m128i w27l = _mm_add_epi32(ll, m128_s1(t1l));
        __m128i w27u = _mm_add_epi32(lu, m128_s1(t1u));
        _mm_store_si128(&w[27 * 2], w27l);
        _mm_store_si128(&w[27 * 2 + 1], w27u);
        t1l = w27l;
        t1u = w27u;
        l0 = _mm256_load_ps((float*)&w[22 * 2]);

        lu = M256_UPPER_SI(l1);
        ll = M256_LOWER_SI(l1);
        __m128i w28l = _mm_add_epi32(ll, m128_s1(t0l));
        __m128i w28u = _mm_add_epi32(lu, m128_s1(t0u));
        _mm_store_si128(&w[28 * 2], w28l);
        _mm_store_si128(&w[28 * 2 + 1], w28u);
        t0l = w28l;
        t0u = w28u;
        l1 = _mm256_load_ps((float*)&w[23 * 2]);

        lu = M256_UPPER_SI(l0);
        ll = M256_LOWER_SI(l0);
        __m128i w29l = _mm_add_epi32(ll, m128_s1(t1l));
        __m128i w29u = _mm_add_epi32(lu, m128_s1(t1u));
        _mm_store_si128(&w[29 * 2], w29l);
        _mm_store_si128(&w[29 * 2 + 1], w29u);
        t1l = w29l;
        t1u = w29u;
        l0 = _mm256_load_ps((float*)&w[24 * 2]);

        lu = M256_UPPER_SI(l1);
        ll = M256_LOWER_SI(l1);
        __m128i w30l = _mm_add_epi32(m128_s0(c64), _mm_add_epi32(ll, m128_s1(t0l)));
        __m128i w30u = _mm_add_epi32(m128_s0(c64), _mm_add_epi32(lu, m128_s1(t0u)));
        _mm_store_si128(&w[30 * 2], w30l);
        _mm_store_si128(&w[30 * 2 + 1], w30u);
        t0l = w30l;
        t0u = w30u;
        l1 = _mm256_load_ps((float*)&w[16 * 2]);

        lu = M256_UPPER_SI(l0);
        ll = M256_LOWER_SI(l0);
        __m128i w31l = _mm_add_epi32(_mm_add_epi32(ll, m128_s1(t1l)), _mm_add_epi32(c64, m128_s0(M256_LOWER_SI(l1))));
        __m128i w31u = _mm_add_epi32(_mm_add_epi32(lu, m128_s1(t1u)), _mm_add_epi32(c64, m128_s0(M256_UPPER_SI(l1))));
        _mm_store_si128(&w[31 * 2], w31l);
        _mm_store_si128(&w[31 * 2 + 1], w31u);
        t1l = w31l;
        t1u = w31u;
    }

    __m256 h0 = _mm256_broadcast_ps((__m128*)&H[0]), h1 = _mm256_broadcast_ps((__m128*)&H[4]);

    __m256 a = _mm256_permute_ps(h0, 0b00000000), b = _mm256_permute_ps(h0, 0b01010101),
           c = _mm256_permute_ps(h0, 0b10101010), d = _mm256_permute_ps(h0, 0b11111111),
           e = _mm256_permute_ps(h1, 0b00000000), f = _mm256_permute_ps(h1, 0b01010101),
           g = _mm256_permute_ps(h1, 0b10101010), h = _mm256_permute_ps(h1, 0b11111111);

    for (uint32_t i = 0; i < 32; i++) {
        __m128i ki = _mm_castps_si128(_mm_broadcast_ss((float*)&k[i])), wil = _mm_load_si128(&w[i * 2]),
                wiu = _mm_load_si128(&w[i * 2 + 1]);

        __m256 maj = m256_maj(a, b, c), ch = m256_ch(e, f, g);

        __m128i temp2u = _mm_add_epi32(m128_S0(M256_UPPER_SI(a)), M256_UPPER_SI(maj)),
                temp2l = _mm_add_epi32(m128_S0(M256_LOWER_SI(a)), M256_LOWER_SI(maj)),
                temp1u = _mm_add_epi32(
                    _mm_add_epi32(m128_S1(M256_UPPER_SI(e)), M256_UPPER_SI(ch)),
                    _mm_add_epi32(M256_UPPER_SI(h), _mm_add_epi32(ki, wiu))
                ),
                temp1l = _mm_add_epi32(
                    _mm_add_epi32(m128_S1(M256_LOWER_SI(e)), M256_LOWER_SI(ch)),
                    _mm_add_epi32(M256_LOWER_SI(h), _mm_add_epi32(ki, wil))
                );

        h = g;
        g = f;
        f = e;
        e = M128I_FUSE_PS(_mm_add_epi32(M256_LOWER_SI(d), temp1l), _mm_add_epi32(M256_UPPER_SI(d), temp1u));
        d = c;
        c = b;
        b = a;
        a = M128I_FUSE_PS(_mm_add_epi32(temp1l, temp2l), _mm_add_epi32(temp1u, temp2u));
    }

    for (uint32_t i = 32; i < 64; i++) {
        __m128i wi16l = _mm_load_si128(&w[i * 2 - 32]), wi16u = _mm_load_si128(&w[i * 2 - 31]),
                wi15l = _mm_load_si128(&w[i * 2 - 30]), wi15u = _mm_load_si128(&w[i * 2 - 29]),
                wi7l = _mm_load_si128(&w[i * 2 - 14]), wi7u = _mm_load_si128(&w[i * 2 - 13]), wi2l = t0l, wi2u = t0u;

        t0l = t1l;
        t0u = t1u;

        __m128i ws1l = m128_s1(wi2l), ws1u = m128_s1(wi2u), ws0l = m128_s0(wi15l), ws0u = m128_s0(wi15u);

        __m128i wil = _mm_add_epi32(_mm_add_epi32(wi16l, wi7l), _mm_add_epi32(ws0l, ws1l)),
                wiu = _mm_add_epi32(_mm_add_epi32(wi16u, wi7u), _mm_add_epi32(ws0u, ws1u));

        _mm_store_si128(&w[i * 2], wil);
        _mm_store_si128(&w[i * 2 + 1], wiu);

        t1l = wil;
        t1u = wiu;

        __m128i ki = _mm_castps_si128(_mm_broadcast_ss((float*)&k[i]));

        __m256 maj = m256_maj(a, b, c), ch = m256_ch(e, f, g);

        __m128i temp2u = _mm_add_epi32(m128_S0(M256_UPPER_SI(a)), M256_UPPER_SI(maj)),
                temp2l = _mm_add_epi32(m128_S0(M256_LOWER_SI(a)), M256_LOWER_SI(maj)),
                temp1u = _mm_add_epi32(
                    _mm_add_epi32(m128_S1(M256_UPPER_SI(e)), M256_UPPER_SI(ch)),
                    _mm_add_epi32(M256_UPPER_SI(h), _mm_add_epi32(ki, wiu))
                ),
                temp1l = _mm_add_epi32(
                    _mm_add_epi32(m128_S1(M256_LOWER_SI(e)), M256_LOWER_SI(ch)),
                    _mm_add_epi32(M256_LOWER_SI(h), _mm_add_epi32(ki, wil))
                );

        h = g;
        g = f;
        f = e;
        e = M128I_FUSE_PS(_mm_add_epi32(M256_LOWER_SI(d), temp1l), _mm_add_epi32(M256_UPPER_SI(d), temp1u));
        d = c;
        c = b;
        b = a;
        a = M128I_FUSE_PS(_mm_add_epi32(temp1l, temp2l), _mm_add_epi32(temp1u, temp2u));
    }

    a = M128I_FUSE_PS(
        _mm_add_epi32(M256_LOWER_SI(a), _mm_castps_si128(_mm_broadcast_ss((float*)&H[0]))),
        _mm_add_epi32(M256_UPPER_SI(a), _mm_castps_si128(_mm_broadcast_ss((float*)&H[0])))
    );

    b = M128I_FUSE_PS(
        _mm_add_epi32(M256_LOWER_SI(b), _mm_castps_si128(_mm_broadcast_ss((float*)&H[1]))),
        _mm_add_epi32(M256_UPPER_SI(b), _mm_castps_si128(_mm_broadcast_ss((float*)&H[1])))
    );

    c = M128I_FUSE_PS(
        _mm_add_epi32(M256_LOWER_SI(c), _mm_castps_si128(_mm_broadcast_ss((float*)&H[2]))),
        _mm_add_epi32(M256_UPPER_SI(c), _mm_castps_si128(_mm_broadcast_ss((float*)&H[2])))
    );

    d = M128I_FUSE_PS(
        _mm_add_epi32(M256_LOWER_SI(d), _mm_castps_si128(_mm_broadcast_ss((float*)&H[3]))),
        _mm_add_epi32(M256_UPPER_SI(d), _mm_castps_si128(_mm_broadcast_ss((float*)&H[3])))
    );

    e = M128I_FUSE_PS(
        _mm_add_epi32(M256_LOWER_SI(e), _mm_castps_si128(_mm_broadcast_ss((float*)&H[4]))),
        _mm_add_epi32(M256_UPPER_SI(e), _mm_castps_si128(_mm_broadcast_ss((float*)&H[4])))
    );

    f = M128I_FUSE_PS(
        _mm_add_epi32(M256_LOWER_SI(f), _mm_castps_si128(_mm_broadcast_ss((float*)&H[5]))),
        _mm_add_epi32(M256_UPPER_SI(f), _mm_castps_si128(_mm_broadcast_ss((float*)&H[5])))
    );

    g = M128I_FUSE_PS(
        _mm_add_epi32(M256_LOWER_SI(g), _mm_castps_si128(_mm_broadcast_ss((float*)&H[6]))),
        _mm_add_epi32(M256_UPPER_SI(g), _mm_castps_si128(_mm_broadcast_ss((float*)&H[6])))
    );

    h = M128I_FUSE_PS(
        _mm_add_epi32(M256_LOWER_SI(h), _mm_castps_si128(_mm_broadcast_ss((float*)&H[7]))),
        _mm_add_epi32(M256_UPPER_SI(h), _mm_castps_si128(_mm_broadcast_ss((float*)&H[7])))
    );

    __m256 tmp00 = _mm256_unpacklo_ps(a, b), tmp01 = _mm256_unpackhi_ps(a, b), tmp02 = _mm256_unpacklo_ps(c, d),
           tmp03 = _mm256_unpackhi_ps(c, d), tmp10 = _mm256_unpacklo_ps(e, f), tmp11 = _mm256_unpackhi_ps(e, f),
           tmp12 = _mm256_unpacklo_ps(g, h), tmp13 = _mm256_unpackhi_ps(g, h);

    // rows without swap
    __m256i r0 = _mm256_castps_si256(_mm256_shuffle_ps(tmp00, tmp02, 0b01000100)),
            r1 = _mm256_castps_si256(_mm256_shuffle_ps(tmp00, tmp02, 0b11101110)),
            r2 = _mm256_castps_si256(_mm256_shuffle_ps(tmp01, tmp03, 0b01000100)),
            r3 = _mm256_castps_si256(_mm256_shuffle_ps(tmp01, tmp03, 0b11101110)),
            r4 = _mm256_castps_si256(_mm256_shuffle_ps(tmp10, tmp12, 0b01000100)),
            r5 = _mm256_castps_si256(_mm256_shuffle_ps(tmp10, tmp12, 0b11101110)),
            r6 = _mm256_castps_si256(_mm256_shuffle_ps(tmp11, tmp13, 0b01000100)),
            r7 = _mm256_castps_si256(_mm256_shuffle_ps(tmp11, tmp13, 0b11101110));

    __m128i mask = _mm_load_si128((__m128i*)byteswap_mask);

    // breaking stores up from 256 to 128 probably helps memory bandwith, especially on 2012 CPU
    // also, free swap
    _mm_store_si128((__m128i*)&hash[64], _mm_shuffle_epi8(_mm256_extractf128_si256(r0, 1), mask));
    _mm_store_si128((__m128i*)&hash[0], _mm_shuffle_epi8(_mm256_castsi256_si128(r0), mask));

    _mm_store_si128((__m128i*)&hash[80], _mm_shuffle_epi8(_mm256_extractf128_si256(r4, 1), mask));
    _mm_store_si128((__m128i*)&hash[16], _mm_shuffle_epi8(_mm256_castsi256_si128(r4), mask));

    _mm_store_si128((__m128i*)&hash[96], _mm_shuffle_epi8(_mm256_extractf128_si256(r1, 1), mask));
    _mm_store_si128((__m128i*)&hash[32], _mm_shuffle_epi8(_mm256_castsi256_si128(r1), mask));

    _mm_store_si128((__m128i*)&hash[112], _mm_shuffle_epi8(_mm256_extractf128_si256(r5, 1), mask));
    _mm_store_si128((__m128i*)&hash[48], _mm_shuffle_epi8(_mm256_castsi256_si128(r5), mask));

    _mm_store_si128((__m128i*)&hash[192], _mm_shuffle_epi8(_mm256_extractf128_si256(r2, 1), mask));
    _mm_store_si128((__m128i*)&hash[128], _mm_shuffle_epi8(_mm256_castsi256_si128(r2), mask));

    _mm_store_si128((__m128i*)&hash[208], _mm_shuffle_epi8(_mm256_extractf128_si256(r6, 1), mask));
    _mm_store_si128((__m128i*)&hash[144], _mm_shuffle_epi8(_mm256_castsi256_si128(r6), mask));

    _mm_store_si128((__m128i*)&hash[224], _mm_shuffle_epi8(_mm256_extractf128_si256(r3, 1), mask));
    _mm_store_si128((__m128i*)&hash[160], _mm_shuffle_epi8(_mm256_castsi256_si128(r3), mask));

    _mm_store_si128((__m128i*)&hash[240], _mm_shuffle_epi8(_mm256_extractf128_si256(r7, 1), mask));
    _mm_store_si128((__m128i*)&hash[176], _mm_shuffle_epi8(_mm256_castsi256_si128(r7), mask));
}
#else
void sha256x4x2_fused(uint8_t hash[SHA256_DIGEST_LENGTH * 8], const uint8_t data[SHA256_INPUT_LENGTH * 8]) {
    (void)hash;
    (void)data;
}
#endif