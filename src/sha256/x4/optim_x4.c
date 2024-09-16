#include "sha256x4.h"

#include <stdint.h>

#include <emmintrin.h>
#include <immintrin.h>
#include <tmmintrin.h>

#include <stdio.h>

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
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

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

static __m128i rotr(const __m128i x, const uint32_t n) {
	return _mm_or_si128(
		_mm_srli_epi32(x, n),
		 _mm_slli_epi32(x, 32 - n)
	);
}

static __m128i s0(const __m128i w) {
	__m128i t1 = rotr(w, 18);
	__m128i t2 = _mm_srli_epi32(w, 3);
	__m128i t0 = rotr(w, 7);

	return _mm_castps_si128(_mm_xor_ps
	(
		_mm_castsi128_ps(t0), 
		_mm_xor_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2))
	));
}

static __m128i s1(const __m128i w) {
	__m128i t1 = rotr(w, 19);
	__m128i t2 = _mm_srli_epi32(w, 10);
	__m128i t0 = rotr(w, 17);

	return _mm_castps_si128(_mm_xor_ps(
		_mm_castsi128_ps(t0), 
		_mm_xor_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2))
	));
}

static __m128i S0(const __m128i a) {
	return _mm_castps_si128(_mm_xor_ps(
		_mm_castsi128_ps(rotr(a, 2)),
		_mm_xor_ps(
			_mm_castsi128_ps(rotr(a, 13)), 
			_mm_castsi128_ps(rotr(a, 22))
		)
	));
}

static __m128i S1(const __m128i e) {
	return _mm_castps_si128(_mm_xor_ps(
		_mm_castsi128_ps(rotr(e, 6)),
		_mm_xor_ps(
			_mm_castsi128_ps(rotr(e, 11)), 
			_mm_castsi128_ps(rotr(e, 25))
		)
	));
}

static __m128i ch(const __m128i e, const __m128i f, const __m128i g) {
	return _mm_castps_si128(_mm_xor_ps(
		_mm_and_ps(_mm_castsi128_ps(e), _mm_castsi128_ps(f)),
		_mm_andnot_ps(_mm_castsi128_ps(e), _mm_castsi128_ps(g))		
	));
}

static __m128i maj(const __m128i a, const __m128i b, const __m128i c) {
	return _mm_castps_si128(_mm_xor_ps(
		_mm_and_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)),
		_mm_xor_ps(
			_mm_and_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(c)),
			_mm_and_ps(_mm_castsi128_ps(b), _mm_castsi128_ps(c))
		)
	));
}

static const uint8_t byteswap_mask[16] __attribute__((aligned(16))) = {
	3, 2, 1, 0,
	7, 6, 5, 4,
	11, 10, 9, 8,
	15, 14, 13, 12
};

static const uint32_t x80000000[4] __attribute__((aligned(16))) = {
	0x80000000, 0x80000000, 0x80000000, 0x80000000
};

static const uint32_t d64[4] __attribute__((aligned(16))) = {
	64, 64, 64, 64
};

__attribute__((flatten))
void sha256x4_optim(
	uint8_t hash[32 * 4], 
	const uint8_t data[8 * 4]
) {
	__m128i w[64] __attribute__((aligned(16)));

	// interleave input data words
	// register keyword doesn't actually do anything but i like writing it
	register __m128i t0;
	register __m128i t1;

	// TODO :: fix this shit using casts and blend
	{
		__m128i
			mask = _mm_load_si128((__m128i*)byteswap_mask),
			l0 = _mm_load_si128((__m128i*)&data[0]),
			l1 = _mm_load_si128((__m128i*)&data[16]);

		__m128i
			h0 = _mm_shuffle_epi8(l0, mask),
			h1 = _mm_shuffle_epi8(l1, mask);

		__m128i
			lo = _mm_unpacklo_epi32(h0, h1),
			hi = _mm_unpackhi_epi32(h0, h1);

		__m128i
			w0 = _mm_unpacklo_epi32(lo,  hi),
			w1 = _mm_unpackhi_epi32(lo, hi);

		_mm_store_si128(&w[0], w0);
		_mm_store_si128(&w[1], w1);

		t0 = w0;
		t1 = w1;
	}


	__m128i
		c80 = _mm_load_si128((__m128i*)x80000000),
		c64 = _mm_load_si128((__m128i*)d64);

	__builtin_memset(&w[3], 0, (15 - 3) * sizeof(__m128i));

	_mm_store_si128(&w[2], c80);
	_mm_store_si128(&w[15], c64);

	{
		__m128i w16 = _mm_add_epi32(t0, s0(t1));
		_mm_store_si128(&w[16], w16);
		t0 = w16;
	}
	{
		__m128i w17 = _mm_add_epi32(t1, _mm_add_epi32(s0(c80), s1(c64)));
		_mm_store_si128(&w[17], w17);
		t1 = w17;
	}
	{
		__m128i w18 = _mm_add_epi32(c80, s1(t0));
		_mm_store_si128(&w[18], w18);
		t0 = w18;
	}
	{
		__m128i w19 = s1(t1);
		_mm_store_si128(&w[19], w19);
		t1 = w19;

		__m128i w20 = s1(t0);
		_mm_store_si128(&w[20], w20);
		t0 = w20;

		__m128i w21 = s1(t1);
		_mm_store_si128(&w[21], w21);
		t1 = w21;
	}
	{
		__m128i w22 = _mm_add_epi32(c64, s1(t0));
		_mm_store_si128(&w[22], w22);
		t0 = w22;
	}
	
	
	{
		register __m128i l0 = _mm_load_si128(&w[16]);
		register __m128i l1 = _mm_load_si128(&w[17]);
		
		__m128i w23 = _mm_add_epi32(l0, s1(t1));
		_mm_store_si128(&w[23], w23);
		t1 = w23;
		l0 = _mm_load_si128(&w[18]);

		__m128i w24 = _mm_add_epi32(l1, s1(t0));
		_mm_store_si128(&w[24], w24);
		t0 = w24;
		l1 = _mm_load_si128(&w[19]);

		__m128i w25 = _mm_add_epi32(l0, s1(t1));
		_mm_store_si128(&w[25], w25);
		t1 = w25;
		l0 = _mm_load_si128(&w[20]);

		__m128i w26 = _mm_add_epi32(l1, s1(t0));
		_mm_store_si128(&w[26], w26);
		t0 = w26;
		l1 = _mm_load_si128(&w[21]);

		__m128i w27 = _mm_add_epi32(l0, s1(t1));
		_mm_store_si128(&w[27], w27);
		t1 = w27;
		l0 = _mm_load_si128(&w[22]);

		__m128i w28 = _mm_add_epi32(l1, s1(t0));
		_mm_store_si128(&w[28], w28);
		t0 = w28;
		l1 = _mm_load_si128(&w[23]);

		__m128i w29 = _mm_add_epi32(l0, s1(t1));
		_mm_store_si128(&w[29], w29);
		t1 = w29;
		l0 = _mm_load_si128(&w[24]);

		__m128i w30 = _mm_add_epi32(
			s0(c64), 
			_mm_add_epi32(l1, s1(t0))
		);
		_mm_store_si128(&w[30], w30);
		t0 = w30;
		l1 = _mm_load_si128(&w[16]);

		__m128i w31 = _mm_add_epi32(
			_mm_add_epi32(l0, s1(t1)),
			_mm_add_epi32(c64, s0(l1))
		);
		_mm_store_si128(&w[31], w31);
		t1 = w31;
	}

	for (uint32_t i = 32; i < 64; i++) {
		__m128i
			wi16 = _mm_load_si128(&w[i - 16]),
			wi15 = _mm_load_si128(&w[i - 15]),
			wi7 = _mm_load_si128(&w[i - 7]),
			wi2 = t0;
			t0 = t1;

		__m128i ws1 = s1(wi2);
		__m128i ws0 = s0(wi15);
		__m128i wi = _mm_add_epi32(
			_mm_add_epi32(wi16, wi7),
			_mm_add_epi32(ws0, ws1)
		);
		_mm_store_si128(&w[i], wi);
		t1 = wi;
	}

	// RIP 8 registers, at least no stack
	__m128i
		a = _mm_castps_si128(_mm_broadcast_ss((float*)&H[0])),
		b = _mm_castps_si128(_mm_broadcast_ss((float*)&H[1])),
		c = _mm_castps_si128(_mm_broadcast_ss((float*)&H[2])),
		d = _mm_castps_si128(_mm_broadcast_ss((float*)&H[3])),
		e = _mm_castps_si128(_mm_broadcast_ss((float*)&H[4])),
		f = _mm_castps_si128(_mm_broadcast_ss((float*)&H[5])),
		g = _mm_castps_si128(_mm_broadcast_ss((float*)&H[6])),
		h = _mm_castps_si128(_mm_broadcast_ss((float*)&H[7]));
	
	for (uint32_t i = 0; i < 64; i++) {
		__m128i 
			ki = _mm_castps_si128(_mm_broadcast_ss((float*)&k[i])),
			wi = _mm_load_si128(&w[i]);
		
		__m128i 
			temp2 = _mm_add_epi32(S0(a), maj(a, b, c)),
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

	// TODO :: single broadcast/load + shuffle
	// do after transpose for no shuffle ??? maybe, check perf

	a = _mm_add_epi32(a, _mm_castps_si128(_mm_broadcast_ss((float*)&H[0])));
	b = _mm_add_epi32(b, _mm_castps_si128(_mm_broadcast_ss((float*)&H[1])));
	c = _mm_add_epi32(c, _mm_castps_si128(_mm_broadcast_ss((float*)&H[2])));
	d = _mm_add_epi32(d, _mm_castps_si128(_mm_broadcast_ss((float*)&H[3])));
	e = _mm_add_epi32(e, _mm_castps_si128(_mm_broadcast_ss((float*)&H[4])));
	f = _mm_add_epi32(f, _mm_castps_si128(_mm_broadcast_ss((float*)&H[5])));
	g = _mm_add_epi32(g, _mm_castps_si128(_mm_broadcast_ss((float*)&H[6])));
	h = _mm_add_epi32(h, _mm_castps_si128(_mm_broadcast_ss((float*)&H[7])));

	// trans pose ma trix ????
	// MATH REFERECNE ?????????
	// OMG im losing it right now
	// unpackhilomovhilo time

	__m128i
		mask = _mm_load_si128((__m128i*)byteswap_mask);

	// first half
	{
		__m128i
			tmp0 = _mm_unpacklo_epi32(a, b),
			tmp1 = _mm_unpackhi_epi32(a, b),
			tmp2 = _mm_unpacklo_epi32(c, d),
			tmp3 = _mm_unpackhi_epi32(c, d);
		
		a = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(tmp0), _mm_castsi128_ps(tmp2)));
		b = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(tmp2), _mm_castsi128_ps(tmp0)));
		c = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(tmp1), _mm_castsi128_ps(tmp3)));
		d = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(tmp3), _mm_castsi128_ps(tmp1)));
	}
	// second half
	{
		__m128i
			tmp0 = _mm_unpacklo_epi32(e, f),
			tmp1 = _mm_unpackhi_epi32(e, f),
			tmp2 = _mm_unpacklo_epi32(g, h),
			tmp3 = _mm_unpackhi_epi32(g, h);
		
		e = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(tmp0), _mm_castsi128_ps(tmp2)));
		f = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(tmp2), _mm_castsi128_ps(tmp0)));
		g = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(tmp1), _mm_castsi128_ps(tmp3)));
		h = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(tmp3), _mm_castsi128_ps(tmp1)));
	}

	// byteswap and store
	a = _mm_shuffle_epi8(a, mask);
	b = _mm_shuffle_epi8(b, mask);
	c = _mm_shuffle_epi8(c, mask);
	d = _mm_shuffle_epi8(d, mask);
	e = _mm_shuffle_epi8(e, mask);
	f = _mm_shuffle_epi8(f, mask);
	g = _mm_shuffle_epi8(g, mask);
	h = _mm_shuffle_epi8(h, mask);

	_mm_store_si128((__m128i*)&hash[0], a);
	_mm_store_si128((__m128i*)&hash[16], e);
	_mm_store_si128((__m128i*)&hash[32], b);
	_mm_store_si128((__m128i*)&hash[48], f);
	_mm_store_si128((__m128i*)&hash[64], c);
	_mm_store_si128((__m128i*)&hash[80], g);
	_mm_store_si128((__m128i*)&hash[96], d);
	_mm_store_si128((__m128i*)&hash[112], h);
}