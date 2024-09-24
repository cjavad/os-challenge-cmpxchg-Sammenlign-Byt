#include "sha256x4.h"

static const uint32_t k[64] __attribute__((aligned(16))) = {
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

static const uint8_t byteswap_mask[16] __attribute__((aligned(16))) = {
	3, 2, 1, 0,
	7, 6, 5, 4,
	11, 10, 9, 8,
	15, 14, 13, 12
};

static const uint8_t punpack_mask[16] __attribute__((aligned(16))) = {
	6, 4, 2, 0,
	14, 12, 10, 8,
	7, 5, 3, 1,
	15, 13, 11, 9
};

static const uint32_t x80000000[4] __attribute__((aligned(16))) = {
    0x80000000, 0x80000000, 0x80000000, 0x80000000};

static const uint32_t d64[4] __attribute__((aligned(16))) = {
	64, 64, 64, 64
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

void sha256x4_asm(
	uint8_t hash[SHA256_DIGEST_LENGTH * 4],
	const uint8_t data[SHA256_INPUT_LENGTH * 4]
) {
	// cant clobber
	// rdi, rsp, rbp

	// currently 4 bytes inserted before

	__asm__ volatile (
		R"""(
			// in case we were previously using ymm registers
			// old intel CPUs die when using legacy xmm after ymm
			vzeroupper

			// set stack pointer (rsp aligned to 16 bytes by default)
			// 512 for w buffer
			sub rsp, 1024 + 8

			// load data and byteswap mask into registers
			vmovdqa xmm2, %[byteswap_mask]
			vmovdqa xmm0, [rsi]
			vmovdqa xmm1, [rsi + 16]

			// byteswap data, need big endian
			vpshufb xmm0, xmm0, xmm2
			vpshufb xmm1, xmm1, xmm2

			// move low sha256 word into xmm0, high into xmm1
			vpunpckldq xmm2, xmm0, xmm1
			vpunpckhdq xmm1, xmm0, xmm1
			vpunpckldq xmm0, xmm2, xmm1
			vpunpckhdq xmm1, xmm2, xmm1

			// w[0] = w0, w[1] = w1
			vmovdqa [rsp + (16 * 0)], xmm0
			vmovdqa [rsp + (16 * 1)], xmm1

			// load constants
			// set w[2] to 0x80000000
			// set [w[3] .. w[14]] to 0
			// set w[15] to 64
			vmovdqa xmm2, %[x80000000]
			vmovdqa xmm3, %[d64]
			// pxor with same register is recognized as special case for zeroing by CPU
			pxor xmm4, xmm4
			vmovdqa [rsp + (16 * 2)], xmm2
			vmovdqa [rsp + (16 * 3)], xmm4
			vmovdqa [rsp + (16 * 4)], xmm4
			vmovdqa [rsp + (16 * 5)], xmm4
			vmovdqa [rsp + (16 * 6)], xmm4
			vmovdqa [rsp + (16 * 7)], xmm4
			vmovdqa [rsp + (16 * 8)], xmm4
			vmovdqa [rsp + (16 * 9)], xmm4
			vmovdqa [rsp + (16 * 10)], xmm4
			vmovdqa [rsp + (16 * 11)], xmm4
			vmovdqa [rsp + (16 * 12)], xmm4
			vmovdqa [rsp + (16 * 13)], xmm4
			vmovdqa [rsp + (16 * 14)], xmm4
			vmovdqa [rsp + (16 * 15)], xmm3

			// ==========
			// xmm0 is w0
			// xmm1 is w1
			// xmm2 is 0x80000000
			// xmm3 is 64

			// w16 = w0 + s0(w1)
			// s0(w1)
			// rotr(w1, 18)
			vpsrld xmm4, xmm1, 18
			vpslld xmm5, xmm1, 32 - 18
			vpor xmm4, xmm4, xmm5
			// rotr(w1, 7)
			vpsrld xmm6, xmm1, 7
			vpslld xmm7, xmm1, 32 - 7
			vpor xmm6, xmm6, xmm7
			// shift right (w1, 3)
			vpsrld xmm5, xmm1, 3
			// xor all together
			vpxor xmm4, xmm4, xmm6
			vpxor xmm4, xmm4, xmm5

			// move w16 into xmm0 and store
			vpaddd xmm0, xmm0, xmm4
			vmovdqa [rsp + (16 * 16)], xmm0

			// xmm0 is w16
			// xmm1 is w1
			// xmm2 is 0x80000000
			// xmm3 is 64
			
			// w17 = w1 + s0(0x80000000) + s1(64)
			vpsrld xmm4, xmm2, 18
			vpslld xmm5, xmm2, 32 - 18
			vpor xmm4, xmm4, xmm5
			vpsrld xmm6, xmm2, 7
			vpslld xmm7, xmm2, 32 - 7
			vpor xmm6, xmm6, xmm7
			vpsrld xmm5, xmm2, 3
			vpxor xmm4, xmm4, xmm6
			vpxor xmm4, xmm4, xmm5

			vpaddd xmm1, xmm1, xmm4

			// s1 (64)
			vpsrld xmm4, xmm3, 19
			vpslld xmm5, xmm3, 32 - 19
			vpor xmm4, xmm4, xmm5
			vpsrld xmm6, xmm3, 17
			vpslld xmm7, xmm3, 32 - 17
			vpor xmm6, xmm6, xmm7
			vpsrld xmm5, xmm3, 10
			vpxor xmm4, xmm4, xmm6
			vpxor xmm4, xmm4, xmm5

			vpaddd xmm1, xmm1, xmm4
			vmovdqa [rsp + (16 * 17)], xmm1

			// xmm0 is w16
			// xmm1 is w17
			// xmm2 is 0x80000000
			// xmm3 is 64

			// w18 = 0x80000000 + s1(w16)
			vpsrld xmm4, xmm0, 19
			vpslld xmm5, xmm0, 32 - 19
			vpor xmm4, xmm4, xmm5
			vpsrld xmm6, xmm0, 17
			vpslld xmm7, xmm0, 32 - 17
			vpor xmm6, xmm6, xmm7
			vpsrld xmm5, xmm0, 10
			vpxor xmm4, xmm4, xmm6
			vpxor xmm4, xmm4, xmm5
			vpaddd xmm0, xmm4, xmm2
			vmovdqa [rsp + (16 * 18)], xmm0

			// xmm0 is w18
			// xmm1 is w17
			// xmm3 is 64

			// w19 = s1(w17)
			vpsrld xmm4, xmm1, 19
			vpslld xmm5, xmm1, 32 - 19
			vpor xmm4, xmm4, xmm5
			vpsrld xmm6, xmm1, 17
			vpslld xmm7, xmm1, 32 - 17
			vpor xmm6, xmm6, xmm7
			vpsrld xmm5, xmm1, 10
			vpxor xmm4, xmm4, xmm6
			vpxor xmm1, xmm4, xmm5

			vmovdqa [rsp + (16 * 19)], xmm1

			// xmm0 is w18
			// xmm1 is w19
			// xmm3 is 64

			// w20 = s1(w18)
			vpsrld xmm4, xmm0, 19
			vpslld xmm5, xmm0, 32 - 19
			vpor xmm4, xmm4, xmm5
			vpsrld xmm6, xmm0, 17
			vpslld xmm7, xmm0, 32 - 17
			vpor xmm6, xmm6, xmm7
			vpsrld xmm5, xmm0, 10
			vpxor xmm4, xmm4, xmm6
			vpxor xmm0, xmm4, xmm5

			vmovdqa [rsp + (16 * 20)], xmm0

			// xmm0 is w20
			// xmm1 is w19
			// xmm3 is 64

			// w21 = s1(w19)
			vpsrld xmm4, xmm1, 19
			vpslld xmm5, xmm1, 32 - 19
			vpor xmm4, xmm4, xmm5
			vpsrld xmm6, xmm1, 17
			vpslld xmm7, xmm1, 32 - 17
			vpor xmm6, xmm6, xmm7
			vpsrld xmm5, xmm1, 10
			vpxor xmm4, xmm4, xmm6
			vpxor xmm1, xmm4, xmm5

			vmovdqa [rsp + (16 * 21)], xmm1

			// xmm0 is w20
			// xmm1 is w21
			// xmm3 is 64

			// w22 = 64 + s1(w20)
			vpsrld xmm4, xmm0, 19
			vpslld xmm5, xmm0, 32 - 19
			vpor xmm4, xmm4, xmm5
			vpsrld xmm6, xmm0, 17
			vpslld xmm7, xmm0, 32 - 17
			vpor xmm6, xmm6, xmm7
			vpsrld xmm5, xmm0, 10
			vpxor xmm4, xmm4, xmm6
			vpxor xmm0, xmm4, xmm5
			vpaddd xmm0, xmm0, xmm3

			vmovdqa [rsp + (16 * 22)], xmm0

			// xmm0 is w22
			// xmm1 is w21
			// xmm3 is 64
			
			vmovdqa xmm2, [rsp + (16 * 16)]
			vmovdqa xmm4, [rsp + (17 * 16)]

			// xmm2 is w16
			// xmm4 is w17

			// w23 = w16 + s1(w21)
			vpsrld xmm5, xmm1, 19
			vpslld xmm6, xmm1, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm1, 17
			vpslld xmm8, xmm1, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm1, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm1, xmm2, xmm5
			vmovdqa xmm2, [rsp + (18 * 16)]
			vmovdqa [rsp + (23 * 16)], xmm1

			// xmm0 is w22
			// xmm1 is w23
			// xmm3 is 64
			// xmm2 is w18
			// xmm4 is w17

			// w24 = w17 + s1(w22)
			vpsrld xmm5, xmm0, 19
			vpslld xmm6, xmm0, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm0, 17
			vpslld xmm8, xmm0, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm0, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm0, xmm4, xmm5
			vmovdqa xmm4, [rsp + (19 * 16)]
			vmovdqa [rsp + (24 * 16)], xmm0

			// xmm0 is w24
			// xmm1 is w23
			// xmm3 is 64
			// xmm2 is w18
			// xmm4 is w19

			// w25 = w18 + s1(w23)
			vpsrld xmm5, xmm1, 19
			vpslld xmm6, xmm1, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm1, 17
			vpslld xmm8, xmm1, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm1, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm1, xmm2, xmm5
			vmovdqa xmm2, [rsp + (20 * 16)]
			vmovdqa [rsp + (25 * 16)], xmm1

			// xmm0 is w24
			// xmm1 is w25
			// xmm3 is 64
			// xmm2 is w20
			// xmm4 is w19

			// w26 = w19 + s1(w24)
			vpsrld xmm5, xmm0, 19
			vpslld xmm6, xmm0, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm0, 17
			vpslld xmm8, xmm0, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm0, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm0, xmm4, xmm5
			vmovdqa xmm4, [rsp + (21 * 16)]
			vmovdqa [rsp + (26 * 16)], xmm0

			// xmm0 is w26
			// xmm1 is w25
			// xmm3 is 64
			// xmm2 is w20
			// xmm4 is w21

			// w27 = w20 + s1(w25)
			vpsrld xmm5, xmm1, 19
			vpslld xmm6, xmm1, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm1, 17
			vpslld xmm8, xmm1, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm1, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm1, xmm2, xmm5
			vmovdqa xmm2, [rsp + (22 * 16)]
			vmovdqa [rsp + (27 * 16)], xmm1

			// xmm0 is w26
			// xmm1 is w27
			// xmm3 is 64
			// xmm2 is w22
			// xmm4 is w21

			// w28 = w21 + s1(w26)
			vpsrld xmm5, xmm0, 19
			vpslld xmm6, xmm0, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm0, 17
			vpslld xmm8, xmm0, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm0, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm0, xmm4, xmm5
			vmovdqa xmm4, [rsp + (23 * 16)]
			vmovdqa [rsp + (28 * 16)], xmm0

			// xmm0 is w28
			// xmm1 is w27
			// xmm3 is 64
			// xmm2 is w22
			// xmm4 is w23

			// w29 = w22 + s1(w27)
			vpsrld xmm5, xmm1, 19
			vpslld xmm6, xmm1, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm1, 17
			vpslld xmm8, xmm1, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm1, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm1, xmm2, xmm5
			vmovdqa xmm2, [rsp + (24 * 16)]
			vmovdqa [rsp + (29 * 16)], xmm1

			// xmm0 is w28
			// xmm1 is w29
			// xmm3 is 64
			// xmm2 is w24
			// xmm4 is w23

			// w30 = s0(64) + w23 + s1(w28)
			vpsrld xmm5, xmm0, 19
			vpslld xmm6, xmm0, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm0, 17
			vpslld xmm8, xmm0, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm0, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm0, xmm4, xmm5
			vmovdqa xmm4, [rsp + (16 * 16)]
			vpsrld xmm5, xmm3, 18
			vpslld xmm6, xmm3, 32 - 18
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm3, 7
			vpslld xmm8, xmm3, 32 - 7
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm3, 3
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm0, xmm0, xmm5
			vmovdqa [rsp + (30 * 16)], xmm0

			// xmm0 is w30
			// xmm1 is w29
			// xmm3 is 64
			// xmm2 is w24
			// xmm4 is w16

			// w31 = 64 + s0(w16) + w24 + s1(w29)
			vpsrld xmm5, xmm1, 19
			vpslld xmm6, xmm1, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm1, 17
			vpslld xmm8, xmm1, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm1, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm1, xmm2, xmm5
			vpsrld xmm5, xmm4, 18
			vpslld xmm6, xmm4, 32 - 18
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm4, 7
			vpslld xmm8, xmm4, 32 - 7
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm4, 3
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6
			vpaddd xmm3, xmm3, xmm5
			vpaddd xmm1, xmm1, xmm3
			vmovdqa [rsp + (31 * 16)], xmm1

			// xmm0 is w30
			// xmm1 is w31
			// xmm2 is w24
			// xmm4 is w16
			
			// loop setup

			vmovdqa xmm2, [rsp + (25 * 16)]
			vmovdqa xmm3, [rsp + (17 * 16)]

			// xmm0 is w30 (wi2)
			// xmm1 is w31
			// xmm2 is wi7
			// xmm3 is wi15
			// xmm4 is wi16

			lea rax, [rsp + (32 * 16)]
			lea rcx, [rsp + (63 * 16)]

			// loop begin
			%=:

			// ws1 = s1(wi2)
			vpsrld xmm5, xmm0, 19
			vpslld xmm6, xmm0, 32 - 19
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm0, 17
			vpslld xmm8, xmm0, 32 - 17
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm0, 10
			vpxor xmm5, xmm5, xmm7
			vpxor xmm0, xmm5, xmm6

			// ws0 = s0(wi15)
			vpsrld xmm5, xmm3, 18
			vpslld xmm6, xmm3, 32 - 18
			vpor xmm5, xmm5, xmm6
			vpsrld xmm7, xmm3, 7
			vpslld xmm8, xmm3, 32 - 7
			vpor xmm7, xmm7, xmm8
			vpsrld xmm6, xmm3, 3
			vpxor xmm5, xmm5, xmm7
			vpxor xmm5, xmm5, xmm6

			// wi = wi16 + wi7 + ws0 + ws1
			vpaddd xmm4, xmm4, xmm2
			vpaddd xmm0, xmm0, xmm5
			vpaddd xmm4, xmm0, xmm4
			vmovdqa [rax], xmm4

			// loop exit check
			cmp rax, rcx
			je %=f

			add rax, 16

			// loop end
			vmovdqa xmm2, [rax - 7 * 16]
			vmovdqa xmm0, xmm1
			vmovdqa xmm1, xmm4
			vmovdqa xmm4, xmm3
			vmovdqa xmm3, [rax - 15 * 16]

			jmp %=b
			%=:

			xor rax, rax
			lea rcx, %[k]

			// setup a, b, c, d, e, f, g, h
			vbroadcastss xmm0, %[H]
			vbroadcastss xmm1, %[H] + 4
			vbroadcastss xmm2, %[H] + 8
			vbroadcastss xmm3, %[H] + 12
			vbroadcastss xmm4, %[H] + 16
			vbroadcastss xmm5, %[H] + 20
			vbroadcastss xmm6, %[H] + 24
			vbroadcastss xmm7, %[H] + 28

			// a in xmm0
			// b in xmm1
			// c in xmm2
			// d in xmm3
			// e in xmm4
			// f in xmm5
			// g in xmm6
			// h in xmm7

			// loop begin
			%=:

			// kiwi time
			vbroadcastss xmm8, [rcx + rax]
			vpaddd xmm8, xmm8, [rsp + 4 * rax]
			
			// kiwi in xmm8

			// temp2 = S0(a) + maj(a, b, c)
			// S0(a)
			vpsrld xmm9, xmm0, 2
			vpslld xmm10, xmm0, 32 - 2
			vpor xmm9, xmm9, xmm10
			vpsrld xmm11, xmm0, 13
			vpslld xmm12, xmm0, 32 - 13
			vpor xmm11, xmm11, xmm12
			vpsrld xmm10, xmm0, 22
			vpslld xmm12, xmm0, 32 - 22
			vpor xmm10, xmm10, xmm12
			vpxor xmm9, xmm9, xmm11
			vpxor xmm9, xmm9, xmm10
			// maj(a, b, c)
			vpand xmm10, xmm0, xmm1
			vpand xmm11, xmm0, xmm2
			vpand xmm12, xmm1, xmm2
			vpxor xmm10, xmm10, xmm11
			vpxor xmm10, xmm10, xmm12

			vpaddd xmm9, xmm9, xmm10
			// temp2 in xmm9

			// temp1 = S1(e) + ch(e, f, g) + h + kiwi
			// h + kiwi
			vpaddd xmm8, xmm8, xmm7
			// S1(e)
			vpsrld xmm10, xmm4, 6
			vpslld xmm11, xmm4, 32 - 6
			vpor xmm10, xmm10, xmm11
			vpsrld xmm12, xmm4, 11
			vpslld xmm13, xmm4, 32 - 11
			vpor xmm12, xmm12, xmm13
			vpsrld xmm11, xmm4, 25
			vpslld xmm13, xmm4, 32 - 25
			vpor xmm11, xmm11, xmm13
			vpxor xmm10, xmm10, xmm12
			vpxor xmm10, xmm10, xmm11
			// ch(e, f, g)
			vpand xmm11, xmm4, xmm5
			vpandn xmm12, xmm4, xmm6
			vpxor xmm11, xmm11, xmm12

			vpaddd xmm8, xmm8, xmm10
			vpaddd xmm8, xmm8, xmm11

			// temp 1 in xmm8

			// h = g
			// g = f
			// f = e
			vmovdqa xmm7, xmm6
			vmovdqa xmm6, xmm5
			vmovdqa xmm5, xmm4

			// e = d + temp1
			vpaddd xmm4, xmm3, xmm8

			// d = c
			// c = b
			// b = a
			vmovdqa xmm3, xmm2
			vmovdqa xmm2, xmm1
			vmovdqa xmm1, xmm0

			// a = temp1 + temp2
			vpaddd xmm0, xmm8, xmm9

			// loop exit check
			cmp rax, 63 * 4
			je %=f
			add rax, 4

			// loop end
			jmp %=b
			%=:

			// a..h = a..h + H[0..7]
			vbroadcastss xmm8, %[H]
			vbroadcastss xmm9, %[H] + 4
			vbroadcastss xmm10, %[H] + 8
			vbroadcastss xmm11, %[H] + 12
			vbroadcastss xmm12, %[H] + 16
			vbroadcastss xmm13, %[H] + 20
			vbroadcastss xmm14, %[H] + 24
			vbroadcastss xmm15, %[H] + 28
			vpaddd xmm0, xmm0, xmm8
			vpaddd xmm1, xmm1, xmm9
			vpaddd xmm2, xmm2, xmm10
			vpaddd xmm3, xmm3, xmm11
			vpaddd xmm4, xmm4, xmm12
			vpaddd xmm5, xmm5, xmm13
			vpaddd xmm6, xmm6, xmm14
			vpaddd xmm7, xmm7, xmm15

			// sick trix (wonky matrix gets transposed, it's not 4x4 and that makes my brain hurt)
			vpunpckldq xmm8, xmm0, xmm1
			vpunpckhdq xmm9, xmm0, xmm1
			vpunpckldq xmm10, xmm2, xmm3
			vpunpckhdq xmm11, xmm2, xmm3

			vpunpcklbw xmm0, xmm8, xmm10
			vpunpckhbw xmm1, xmm8, xmm10
			vpunpcklbw xmm2, xmm9, xmm11
			vpunpckhbw xmm3, xmm9, xmm11

			vpunpckldq xmm8, xmm4, xmm5
			vpunpckhdq xmm9, xmm4, xmm5
			vpunpckldq xmm10, xmm6, xmm7
			vpunpckhdq xmm11, xmm6, xmm7

			vpunpcklbw xmm4, xmm8, xmm10
			vpunpckhbw xmm5, xmm8, xmm10
			vpunpcklbw xmm6, xmm9, xmm11
			vpunpckhbw xmm7, xmm9, xmm11

			vmovdqa xmm8, %[punpack_mask]

			vpshufb xmm0, xmm0, xmm8
			vpshufb xmm1, xmm1, xmm8
			vpshufb xmm2, xmm2, xmm8
			vpshufb xmm3, xmm3, xmm8
			vpshufb xmm4, xmm4, xmm8
			vpshufb xmm5, xmm5, xmm8
			vpshufb xmm6, xmm6, xmm8
			vpshufb xmm7, xmm7, xmm8

			// save hash
			vmovdqa [rdi], xmm0
			vmovdqa [rdi + 16], xmm4
			vmovdqa [rdi + 32], xmm1
			vmovdqa [rdi + 48], xmm5
			vmovdqa [rdi + 64], xmm2
			vmovdqa [rdi + 80], xmm6
			vmovdqa [rdi + 96], xmm3
			vmovdqa [rdi + 112], xmm7

			// restore stack pointer
			add rsp, 1024 + 8
		)"""
		// no output operands
		:
		// input operands (System V ABI), incase gcc decides to inline function
		: "rdi"(hash), "rsi"(data),
		// constants
		[byteswap_mask] "m" (byteswap_mask),
		[k] "m" (k),
		[x80000000] "m" (x80000000),
		[d64] "m" (d64),
		[H] "m" (H),
		[punpack_mask] "m" (punpack_mask),
		[format] "m" (format)
		: // TODO :: proper clobbers based off of final assembly
		"rax", "rcx",
		// "rdx", (not using)
		// "rsi", "rbx", // (callee saved)
		// "r8", "r9", "r10", "r11", (not using)
		// "r12", "r13", "r14", "r15", // (callee saved)
		"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
		"xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
		"memory"
	);

	return;
}