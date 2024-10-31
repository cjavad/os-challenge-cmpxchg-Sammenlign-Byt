#pragma once

#include <emmintrin.h>
#include <stdint.h>

#ifdef NO_AVX
#define BROADCAST_SS(val) _mm_castsi128_ps(_mm_set1_epi32(val))
#else
#define BROADCAST_SS(val) _mm_broadcast_ss((float*)&val)
#endif

static const uint32_t k[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
                               0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
                               0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
                               0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
                               0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
                               0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
                               0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
                               0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                               0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
                               0xc67178f2};

static const uint32_t H[8] __attribute__((aligned(16))) = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                                           0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

static __m128i rotr(const __m128i x, const uint32_t n) {
    return _mm_or_si128(_mm_srli_epi32(x, n), _mm_slli_epi32(x, 32 - n));
}

static __m128i s0(const __m128i w) {
    __m128i t1 = rotr(w, 18);
    __m128i t2 = _mm_srli_epi32(w, 3);
    __m128i t0 = rotr(w, 7);

    return _mm_castps_si128(_mm_xor_ps(_mm_castsi128_ps(t0), _mm_xor_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2))));
}

static __m128i s1(const __m128i w) {
    __m128i t1 = rotr(w, 19);
    __m128i t2 = _mm_srli_epi32(w, 10);
    __m128i t0 = rotr(w, 17);

    return _mm_castps_si128(_mm_xor_ps(_mm_castsi128_ps(t0), _mm_xor_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2))));
}

static __m128i S0(const __m128i a) {
    return _mm_castps_si128(_mm_xor_ps(
        _mm_castsi128_ps(rotr(a, 2)), _mm_xor_ps(_mm_castsi128_ps(rotr(a, 13)), _mm_castsi128_ps(rotr(a, 22)))
    ));
}

static __m128i S1(const __m128i e) {
    return _mm_castps_si128(_mm_xor_ps(
        _mm_castsi128_ps(rotr(e, 6)), _mm_xor_ps(_mm_castsi128_ps(rotr(e, 11)), _mm_castsi128_ps(rotr(e, 25)))
    ));
}

static __m128i ch(const __m128i e, const __m128i f, const __m128i g) {
    return _mm_castps_si128(_mm_xor_ps(
        _mm_and_ps(_mm_castsi128_ps(e), _mm_castsi128_ps(f)), _mm_andnot_ps(_mm_castsi128_ps(e), _mm_castsi128_ps(g))
    ));
}

static __m128i maj(const __m128i a, const __m128i b, const __m128i c) {
    return _mm_castps_si128(_mm_xor_ps(
        _mm_and_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)),
        _mm_xor_ps(
            _mm_and_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(c)), _mm_and_ps(_mm_castsi128_ps(b), _mm_castsi128_ps(c))
        )
    ));
}

static const uint8_t byteswap_mask[16]
    __attribute__((aligned(16))) = {3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12};

static const uint32_t x80000000[4] __attribute__((aligned(16))) = {0x80000000, 0x80000000, 0x80000000, 0x80000000};

static const uint32_t d64[4] __attribute__((aligned(16))) = {64, 64, 64, 64};

#define COMPRESS_ROUND(w, i)                                                                                           \
    {                                                                                                                  \
        __m128i ki = _mm_castps_si128(BROADCAST_SS(k[i]));                                                             \
                                                                                                                       \
        __m128i temp2 = _mm_add_epi32(S0(a), maj(a, b, c)),                                                            \
                temp1 = _mm_add_epi32(_mm_add_epi32(S1(e), ch(e, f, g)), _mm_add_epi32(h, _mm_add_epi32(ki, w)));      \
                                                                                                                       \
        h = g;                                                                                                         \
        g = f;                                                                                                         \
        f = e;                                                                                                         \
        e = _mm_add_epi32(d, temp1);                                                                                   \
        d = c;                                                                                                         \
        c = b;                                                                                                         \
        b = a;                                                                                                         \
        a = _mm_add_epi32(temp1, temp2);                                                                               \
    }
