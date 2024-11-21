#include "sha256x4.h"
#include <stdint.h>

#include <emmintrin.h>
#include <immintrin.h>
#include <tmmintrin.h>

#include "impl_common.h"

#include <stdio.h>

uint64_t reverse_sha256x4_fullyfused(
    const uint64_t start,
    const uint64_t end,
    const HashDigest target
) {
    // byte of interest.
    const __m128i h64e61 = _mm_set1_epi32(((uint32_t*)target)[7]);

    uint64_t data[4]
        __attribute__((aligned(64))) = {start, start + 1, start + 2, start + 3};

    do {
        __m128i w[48] __attribute__((aligned(16)));

        // interleave input data words
        // register keyword doesn't actually do anything, but I like writing it
        register __m128i t0;
        register __m128i t1;

        __m128i h0 = _mm_load_si128((__m128i*)&H[0]),
                h1 = _mm_load_si128((__m128i*)&H[4]);

        __m128i a = _mm_shuffle_epi32(h0, 0b00000000),
                b = _mm_shuffle_epi32(h0, 0b01010101),
                c = _mm_shuffle_epi32(h0, 0b10101010),
                d = _mm_shuffle_epi32(h0, 0b11111111),
                e = _mm_shuffle_epi32(h1, 0b00000000),
                f = _mm_shuffle_epi32(h1, 0b01010101),
                g = _mm_shuffle_epi32(h1, 0b10101010),
                h = _mm_shuffle_epi32(h1, 0b11111111);

        {
            __m128i mask = _mm_load_si128((__m128i*)byteswap_mask),
                    l0 = _mm_load_si128((__m128i*)&data[0]),
                    l1 = _mm_load_si128((__m128i*)&data[2]);

            __m128i h0 = _mm_shuffle_epi8(l0, mask),
                    h1 = _mm_shuffle_epi8(l1, mask);

            __m128i lo = _mm_unpacklo_epi32(h0, h1),
                    hi = _mm_unpackhi_epi32(h0, h1);

            __m128i w0 = _mm_unpacklo_epi32(lo, hi),
                    w1 = _mm_unpackhi_epi32(lo, hi);

            t0 = w0;
            t1 = w1;

            COMPRESS_ROUND(t0, 0)
            COMPRESS_ROUND(t1, 1)
        }

        __m128i c80 = _mm_load_si128((__m128i*)x80000000),
                c64 = _mm_load_si128((__m128i*)d64);

        COMPRESS_ROUND(c80, 2)

        for (uint32_t i = 3; i < 15; i++) {
            __m128i ki = _mm_castps_si128(BROADCAST_SS(k[i]));

            __m128i temp2 = _mm_add_epi32(S0(a), maj(a, b, c)),
                    temp1 = _mm_add_epi32(
                        _mm_add_epi32(S1(e), ch(e, f, g)),
                        _mm_add_epi32(h, ki)
                    );

            h = g;
            g = f;
            f = e;
            e = _mm_add_epi32(d, temp1);
            d = c;
            c = b;
            b = a;
            a = _mm_add_epi32(temp1, temp2);
        }

        COMPRESS_ROUND(c64, 15)

        {
            __m128i w16 = _mm_add_epi32(t0, s0(t1));
            _mm_store_si128(&w[0], w16);
            t0 = w16;
            COMPRESS_ROUND(t0, 16)
        }
        {
            __m128i w17 = _mm_add_epi32(t1, _mm_add_epi32(s0(c80), s1(c64)));
            _mm_store_si128(&w[1], w17);
            t1 = w17;
            COMPRESS_ROUND(t1, 17)
        }
        {
            __m128i w18 = _mm_add_epi32(c80, s1(t0));
            _mm_store_si128(&w[2], w18);
            t0 = w18;
            COMPRESS_ROUND(t0, 18)
        }
        {
            __m128i w19 = s1(t1);
            _mm_store_si128(&w[3], w19);
            t1 = w19;
            COMPRESS_ROUND(t1, 19)

            __m128i w20 = s1(t0);
            _mm_store_si128(&w[4], w20);
            t0 = w20;
            COMPRESS_ROUND(t0, 20)

            __m128i w21 = s1(t1);
            _mm_store_si128(&w[5], w21);
            t1 = w21;
            COMPRESS_ROUND(t1, 21)
        }
        {
            __m128i w22 = _mm_add_epi32(c64, s1(t0));
            _mm_store_si128(&w[6], w22);
            t0 = w22;
            COMPRESS_ROUND(t0, 22)
        }

        {
            register __m128i l0 = _mm_load_si128(&w[0]);
            register __m128i l1 = _mm_load_si128(&w[1]);

            __m128i w23 = _mm_add_epi32(l0, s1(t1));
            _mm_store_si128(&w[7], w23);
            t1 = w23;
            COMPRESS_ROUND(t1, 23)
            l0 = _mm_load_si128(&w[2]);

            __m128i w24 = _mm_add_epi32(l1, s1(t0));
            _mm_store_si128(&w[8], w24);
            t0 = w24;
            COMPRESS_ROUND(t0, 24)
            l1 = _mm_load_si128(&w[3]);

            __m128i w25 = _mm_add_epi32(l0, s1(t1));
            _mm_store_si128(&w[9], w25);
            t1 = w25;
            COMPRESS_ROUND(t1, 25)
            l0 = _mm_load_si128(&w[4]);

            __m128i w26 = _mm_add_epi32(l1, s1(t0));
            _mm_store_si128(&w[10], w26);
            t0 = w26;
            COMPRESS_ROUND(t0, 26)
            l1 = _mm_load_si128(&w[5]);

            __m128i w27 = _mm_add_epi32(l0, s1(t1));
            _mm_store_si128(&w[11], w27);
            t1 = w27;
            COMPRESS_ROUND(t1, 27)
            l0 = _mm_load_si128(&w[6]);

            __m128i w28 = _mm_add_epi32(l1, s1(t0));
            _mm_store_si128(&w[12], w28);
            t0 = w28;
            COMPRESS_ROUND(t0, 28)
            l1 = _mm_load_si128(&w[7]);

            __m128i w29 = _mm_add_epi32(l0, s1(t1));
            _mm_store_si128(&w[13], w29);
            t1 = w29;
            COMPRESS_ROUND(t1, 29)
            l0 = _mm_load_si128(&w[8]);

            __m128i w30 = _mm_add_epi32(s0(c64), _mm_add_epi32(l1, s1(t0)));
            _mm_store_si128(&w[14], w30);
            t0 = w30;
            COMPRESS_ROUND(t0, 30)
            l1 = _mm_load_si128(&w[0]);

            __m128i w31 = _mm_add_epi32(
                _mm_add_epi32(l0, s1(t1)),
                _mm_add_epi32(c64, s0(l1))
            );
            _mm_store_si128(&w[15], w31);
            t1 = w31;
            COMPRESS_ROUND(t1, 31)
        }

        // Round 32-60
        for (uint32_t i = 16; i < 45; i++) {
            __m128i wi16 = _mm_load_si128(&w[i - 16]),
                    wi15 = _mm_load_si128(&w[i - 15]),
                    wi7 = _mm_load_si128(&w[i - 7]), wi2 = t0;
            t0 = t1;

            __m128i ws1 = s1(wi2);
            __m128i ws0 = s0(wi15);
            __m128i wi = _mm_add_epi32(
                _mm_add_epi32(wi16, wi7),
                _mm_add_epi32(ws0, ws1)
            );
            _mm_store_si128(&w[i], wi);
            t1 = wi;

            __m128i ki = _mm_castps_si128(BROADCAST_SS(k[i + 16]));

            __m128i temp2 = _mm_add_epi32(S0(a), maj(a, b, c)),
                    temp1 = _mm_add_epi32(
                        _mm_add_epi32(S1(e), ch(e, f, g)),
                        _mm_add_epi32(h, _mm_add_epi32(ki, wi))
                    );

            h = g;
            g = f;
            f = e;
            e = _mm_add_epi32(d, temp1);
            d = c;
            c = b;
            b = a;
            a = _mm_add_epi32(temp1, temp2);
        }

        __m128i test = _mm_cmpeq_epi32(e, h64e61);

        if (_mm_testz_si128(test,test)) {
            goto next;
        }

        // Round 32-61
        for (uint32_t i = 45; i < 48; i++) {
            __m128i wi16 = _mm_load_si128(&w[i - 16]),
                    wi15 = _mm_load_si128(&w[i - 15]),
                    wi7 = _mm_load_si128(&w[i - 7]), wi2 = t0;
            t0 = t1;

            __m128i ws1 = s1(wi2);
            __m128i ws0 = s0(wi15);
            __m128i wi = _mm_add_epi32(
                _mm_add_epi32(wi16, wi7),
                _mm_add_epi32(ws0, ws1)
            );
            _mm_store_si128(&w[i], wi);
            t1 = wi;

            __m128i ki = _mm_castps_si128(BROADCAST_SS(k[i + 16]));

            __m128i temp2 = _mm_add_epi32(S0(a), maj(a, b, c)),
                    temp1 = _mm_add_epi32(
                        _mm_add_epi32(S1(e), ch(e, f, g)),
                        _mm_add_epi32(h, _mm_add_epi32(ki, wi))
                    );

            h = g;
            g = f;
            f = e;
            e = _mm_add_epi32(d, temp1);
            d = c;
            c = b;
            b = a;
            a = _mm_add_epi32(temp1, temp2);
        }

        // RIP 8 registers, at least no stack

        // first half
        {
            __m128i tmp0 = _mm_unpacklo_epi32(a, b),
                    tmp1 = _mm_unpackhi_epi32(a, b),
                    tmp2 = _mm_unpacklo_epi32(c, d),
                    tmp3 = _mm_unpackhi_epi32(c, d);

            a = _mm_castps_si128(
                _mm_movelh_ps(_mm_castsi128_ps(tmp0), _mm_castsi128_ps(tmp2))
            );
            b = _mm_castps_si128(
                _mm_movehl_ps(_mm_castsi128_ps(tmp2), _mm_castsi128_ps(tmp0))
            );
            c = _mm_castps_si128(
                _mm_movelh_ps(_mm_castsi128_ps(tmp1), _mm_castsi128_ps(tmp3))
            );
            d = _mm_castps_si128(
                _mm_movehl_ps(_mm_castsi128_ps(tmp3), _mm_castsi128_ps(tmp1))
            );
        }
        // second half
        {
            __m128i tmp0 = _mm_unpacklo_epi32(e, f),
                    tmp1 = _mm_unpackhi_epi32(e, f),
                    tmp2 = _mm_unpacklo_epi32(g, h),
                    tmp3 = _mm_unpackhi_epi32(g, h);

            e = _mm_castps_si128(
                _mm_movelh_ps(_mm_castsi128_ps(tmp0), _mm_castsi128_ps(tmp2))
            );
            f = _mm_castps_si128(
                _mm_movehl_ps(_mm_castsi128_ps(tmp2), _mm_castsi128_ps(tmp0))
            );
            g = _mm_castps_si128(
                _mm_movelh_ps(_mm_castsi128_ps(tmp1), _mm_castsi128_ps(tmp3))
            );
            h = _mm_castps_si128(
                _mm_movehl_ps(_mm_castsi128_ps(tmp3), _mm_castsi128_ps(tmp1))
            );
        }

        // TODO :: Optimize this.

        // Compare hash 1: (a, e)
        if (((uint64_t*)&a)[0] == ((uint64_t*)target)[0] &&
            ((uint64_t*)&a)[1] == ((uint64_t*)target)[1] &&
            ((uint64_t*)&e)[0] == ((uint64_t*)target)[2] &&
            ((uint64_t*)&e)[1] == ((uint64_t*)target)[3]) {
            return data[0];
        }

        // Compare hash 2: (b, f)
        if (((uint64_t*)&b)[0] == ((uint64_t*)target)[0] &&
            ((uint64_t*)&b)[1] == ((uint64_t*)target)[1] &&
            ((uint64_t*)&f)[0] == ((uint64_t*)target)[2] &&
            ((uint64_t*)&f)[1] == ((uint64_t*)target)[3]) {
            return data[1];
        }

        // Compare hash 3: (c, g)
        if (((uint64_t*)&c)[0] == ((uint64_t*)target)[0] &&
            ((uint64_t*)&c)[1] == ((uint64_t*)target)[1] &&
            ((uint64_t*)&g)[0] == ((uint64_t*)target)[2] &&
            ((uint64_t*)&g)[1] == ((uint64_t*)target)[3]) {
            return data[2];
        }

        // Compare hash 4: (d, h)
        if (((uint64_t*)&d)[0] == ((uint64_t*)target)[0] &&
            ((uint64_t*)&d)[1] == ((uint64_t*)target)[1] &&
            ((uint64_t*)&h)[0] == ((uint64_t*)target)[2] &&
            ((uint64_t*)&h)[1] == ((uint64_t*)target)[3]) {
            return data[3];
        }

    next:
        // Increment data.
        data[0] += 4;
        data[1] += 4;
        data[2] += 4;
        data[3] += 4;
    } while (data[0] <= end);

    return 0;
}