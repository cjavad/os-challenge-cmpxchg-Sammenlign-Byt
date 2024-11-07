#pragma once

#include <stdint.h>

#include <emmintrin.h>
#include <immintrin.h>
#include <tmmintrin.h>

#define M256_LOWER_SI(x) _mm_castps_si128(_mm256_castps256_ps128(x))
#define M256_UPPER_SI(x) _mm_castps_si128(_mm256_extractf128_ps(x, 1))
#define M128I_FUSE_PS(l, u)                                                    \
    _mm256_castsi256_ps(                                                       \
        _mm256_insertf128_si256(_mm256_castsi128_si256(l), u, 1)               \
    )

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

static const uint8_t byteswap_load0[16] __attribute__((aligned(16))
) = {3, 2, 1, 0, 11, 10, 9, 8, 7, 6, 5, 4, 15, 14, 13, 12};

static const uint8_t byteswap_load1[16] __attribute__((aligned(16))
) = {7, 6, 5, 4, 15, 14, 13, 12, 3, 2, 1, 0, 11, 10, 9, 8};

static const uint8_t byteswap_mask[16] __attribute__((aligned(16))
) = {3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12};

static const uint32_t x80000000[4] __attribute__((aligned(16))
) = {0x80000000, 0x80000000, 0x80000000, 0x80000000};

static const uint32_t d64[4] __attribute__((aligned(16))) = {64, 64, 64, 64};

static __m128i m128_rotr(
    const __m128i x,
    const uint32_t n
) {
    return _mm_or_si128(_mm_srli_epi32(x, n), _mm_slli_epi32(x, 32 - n));
}

static __m128i m128_s0(
    const __m128i w
) {
    __m128i t1 = m128_rotr(w, 18);
    __m128i t2 = _mm_srli_epi32(w, 3);
    __m128i t0 = m128_rotr(w, 7);

    return _mm_castps_si128(_mm_xor_ps(
        _mm_castsi128_ps(t0),
        _mm_xor_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2))
    ));
}

static __m128i m128_s1(
    const __m128i w
) {
    __m128i t1 = m128_rotr(w, 19);
    __m128i t2 = _mm_srli_epi32(w, 10);
    __m128i t0 = m128_rotr(w, 17);

    return _mm_castps_si128(_mm_xor_ps(
        _mm_castsi128_ps(t0),
        _mm_xor_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2))
    ));
}

static __m128i m128_S0(
    const __m128i a
) {
    return _mm_castps_si128(_mm_xor_ps(
        _mm_castsi128_ps(m128_rotr(a, 2)),
        _mm_xor_ps(
            _mm_castsi128_ps(m128_rotr(a, 13)),
            _mm_castsi128_ps(m128_rotr(a, 22))
        )
    ));
}

static __m128i m128_S1(
    const __m128i e
) {
    return _mm_castps_si128(_mm_xor_ps(
        _mm_castsi128_ps(m128_rotr(e, 6)),
        _mm_xor_ps(
            _mm_castsi128_ps(m128_rotr(e, 11)),
            _mm_castsi128_ps(m128_rotr(e, 25))
        )
    ));
}

static __m256 m256_ch(
    const __m256 e,
    const __m256 f,
    const __m256 g
) {
    return _mm256_xor_ps(_mm256_and_ps(e, f), _mm256_andnot_ps(e, g));
}

static __m256 m256_maj(
    const __m256 a,
    const __m256 b,
    const __m256 c
) {
    return _mm256_xor_ps(
        _mm256_and_ps(a, b),
        _mm256_xor_ps(_mm256_and_ps(a, c), _mm256_and_ps(b, c))
    );
}