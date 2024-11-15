#include "sha256x4.h"

#include <stdio.h>

static const uint8_t BYTES[] __attribute__((aligned(16))
) = {3, 2, 1, 0, 7,  6,  5,  4, 11, 10, 9, 8, 15, 14, 13, 12, /* 0 */
     6, 4, 2, 0, 14, 12, 10, 8, 7,  5,  3, 1, 15, 13, 11, 9 /* 39 */};

// 1 - 38
static const uint32_t WORDS[] __attribute__((aligned(16))
) = {0x98c7e2a2, 0x98c7e2a2, 0x98c7e2a2, 0x98c7e2a2, /* 1 */
     0xfc08884d, 0xfc08884d, 0xfc08884d, 0xfc08884d, /* 2 */
     0xd16e48e2, 0xd16e48e2, 0xd16e48e2, 0xd16e48e2, /* 3 */
     0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f, /* 4 */
     0x67381d5d, 0x67381d5d, 0x67381d5d, 0x67381d5d, /* 5 */
     0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c, /* 6 */
     0xcd2a11ae, 0xcd2a11ae, 0xcd2a11ae, 0xcd2a11ae, /* 7 */
     0xbabcc441, 0xbabcc441, 0xbabcc441, 0xbabcc441, /* 8 */
     0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667, /* 9 */
     0xb2d5ee51, 0xb2d5ee51, 0xb2d5ee51, 0xb2d5ee51, /* 10 */
     0x8c2e12e0, 0x8c2e12e0, 0x8c2e12e0, 0x8c2e12e0, /* 11 */
     0xd0c6645b, 0xd0c6645b, 0xd0c6645b, 0xd0c6645b, /* 12 */
     0xc19bf1b4, 0xc19bf1b4, 0xc19bf1b4, 0xc19bf1b4, /* 13 */
     0xe49b69c1, 0xe49b69c1, 0xe49b69c1, 0xe49b69c1, /* 14 */
     0x11282000, 0x11282000, 0x11282000, 0x11282000, /* 15 */
     0xe66786,   0xe66786,   0xe66786,   0xe66786,   /* 16 */
     0x80000000, 0x80000000, 0x80000000, 0x80000000, /* 17 */
     0x8fc19dc6, 0x8fc19dc6, 0x8fc19dc6, 0x8fc19dc6, /* 18 */
     0x240ca1cc, 0x240ca1cc, 0x240ca1cc, 0x240ca1cc, /* 19 */
     0x2de92c6f, 0x2de92c6f, 0x2de92c6f, 0x2de92c6f, /* 20 */
     0x4a7484aa, 0x4a7484aa, 0x4a7484aa, 0x4a7484aa, /* 21 */
     0x40,       0x40,       0x40,       0x40,       /* 22 */
     0x5cb0aa1c, 0x5cb0aa1c, 0x5cb0aa1c, 0x5cb0aa1c, /* 23 */
     0x76f988da, 0x76f988da, 0x76f988da, 0x76f988da, /* 24 */
     0x983e5152, 0x983e5152, 0x983e5152, 0x983e5152, /* 25 */
     0xa831c66d, 0xa831c66d, 0xa831c66d, 0xa831c66d, /* 26 */
     0xb00327c8, 0xb00327c8, 0xb00327c8, 0xb00327c8, /* 27 */
     0xbf597fc7, 0xbf597fc7, 0xbf597fc7, 0xbf597fc7, /* 28 */
     0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, /* 29 */
     0xd5a79147, 0xd5a79147, 0xd5a79147, 0xd5a79147, /* 30 */
     0x80100008, 0x80100008, 0x80100008, 0x80100008, /* 31 */
     0x86da6359, 0x86da6359, 0x86da6359, 0x86da6359, /* 32 */
     0x14292967, 0x14292967, 0x14292967, 0x14292967, /* 33 */
     0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85, /* 34 */
     0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372, /* 35 */
     0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a, /* 36 */
     0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, /* 37 */
     0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19 /* 38 */};

static const uint32_t SINGLE_WORD_0[]
    __attribute__((aligned(8))) = {0x510e527f};
static const uint32_t SINGLE_WORD_1[] = {0x67381d5d};
static const uint32_t SINGLE_WORD_2[] = {0x6a09e667};
static const uint32_t SINGLE_WORD_3[] = {0xb2d5ee51};

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

// Align stack to 8 bytes
__attribute__((always_inline)) inline void sha256x4_cyclic_asm(
    uint8_t hash[SHA256_DIGEST_LENGTH * 4],
    const uint8_t data[SHA256_INPUT_LENGTH * 4]
) {
    // print current stack pointer
    __asm__ volatile(
        R"""(
    .p2align 4, 0x90
    sub	rsp, 392

	vmovdqa	xmm0, xmmword ptr [rsi]
	vmovdqa	xmm1, xmmword ptr [rsi + 16]
	vmovdqa	xmm2, xmmword ptr %[BYTES] # xmm2 = [3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12]
	vpshufb	xmm0, xmm0, xmm2
	vpshufb	xmm1, xmm1, xmm2
	vpunpckldq	xmm2, xmm0, xmm1        # xmm2 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
	vpunpckhdq	xmm1, xmm0, xmm1        # xmm1 = xmm0[2],xmm1[2],xmm0[3],xmm1[3]
	vpunpckldq	xmm0, xmm2, xmm1        # xmm0 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
	vpunpckhdq	xmm1, xmm2, xmm1        # xmm1 = xmm2[2],xmm1[2],xmm2[3],xmm1[3]
	vpaddd	xmm6, xmm0, xmmword ptr %[WORDS] + ((1 - 1) * 0x10)
	vpaddd	xmm3, xmm0, xmmword ptr %[WORDS] + ((2 - 1) * 0x10)
	vpsrld	xmm2, xmm3, 2
	vpslld	xmm4, xmm3, 30
	vpor	xmm2, xmm4, xmm2
	vpsrld	xmm4, xmm3, 13
	vpslld	xmm5, xmm3, 19
	vpor	xmm4, xmm5, xmm4
	vpsrld	xmm5, xmm3, 22
	vpslld	xmm7, xmm3, 10
	vpor	xmm5, xmm7, xmm5
	vpxor	xmm4, xmm4, xmm5
	vpand	xmm5, xmm3, xmmword ptr %[WORDS] + ((3 - 1) * 0x10)
	vpxor	xmm2, xmm4, xmm2
	vpaddd	xmm2, xmm2, xmm5
	vpsrld	xmm4, xmm6, 6
	vpslld	xmm5, xmm6, 26
	vpor	xmm4, xmm5, xmm4
	vpsrld	xmm5, xmm6, 11
	vpslld	xmm7, xmm6, 21
	vpor	xmm5, xmm7, xmm5
	vpsrld	xmm7, xmm6, 25
	vpslld	xmm8, xmm6, 7
	vpor	xmm7, xmm8, xmm7
	vpxor	xmm5, xmm5, xmm7
	vpxor	xmm4, xmm5, xmm4
	vbroadcastss	xmm8, dword ptr %[SINGLE_WORD_0] # xmm8 = [1359893119,1359893119,1359893119,1359893119]
	vpand	xmm5, xmm8, xmm6
	vbroadcastss	xmm7, dword ptr %[SINGLE_WORD_1] # xmm7 = [1731730781,1731730781,1731730781,1731730781]
	vpsubd	xmm7, xmm7, xmm0
	vpand	xmm7, xmm7, xmmword ptr %[WORDS] + ((6 - 1) * 0x10)
	vpxor	xmm5, xmm5, xmm7
	vpaddd	xmm5, xmm1, xmm5
	vpaddd	xmm4, xmm5, xmm4
	vpaddd	xmm5, xmm4, xmmword ptr %[WORDS] + ((7 - 1) * 0x10)
	vpaddd	xmm2, xmm4, xmm2
	vpaddd	xmm2, xmm2, xmmword ptr %[WORDS] + ((8 - 1) * 0x10)
	vpsrld	xmm7, xmm2, 2
	vpslld	xmm9, xmm2, 30
	vpor	xmm7, xmm9, xmm7
	vpsrld	xmm9, xmm2, 13
	vpslld	xmm10, xmm2, 19
	vpor	xmm9, xmm10, xmm9
	vpsrld	xmm10, xmm2, 22
	vpslld	xmm11, xmm2, 10
	vpor	xmm10, xmm11, xmm10
	vpxor	xmm9, xmm9, xmm10
	vpxor	xmm7, xmm9, xmm7
	vpand	xmm10, xmm2, xmm3
	vpxor	xmm11, xmm2, xmm3
	vbroadcastss	xmm9, dword ptr %[SINGLE_WORD_2] # xmm9 = [1779033703,1779033703,1779033703,1779033703]
	vpand	xmm11, xmm11, xmm9
	vpxor	xmm10, xmm11, xmm10
	vpaddd	xmm10, xmm10, xmm7
	vpsrld	xmm7, xmm5, 6
	vpslld	xmm11, xmm5, 26
	vpor	xmm7, xmm11, xmm7
	vpsrld	xmm11, xmm5, 11
	vpslld	xmm12, xmm5, 21
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm5, 25
	vpslld	xmm13, xmm5, 7
	vpor	xmm12, xmm13, xmm12
	vpxor	xmm11, xmm11, xmm12
	vpxor	xmm7, xmm11, xmm7
	vpand	xmm11, xmm5, xmm6
	vbroadcastss	xmm12, dword ptr %[SINGLE_WORD_3] # xmm12 = [3000364625,3000364625,3000364625,3000364625]
	vpsubd	xmm4, xmm12, xmm4
	vpand	xmm4, xmm8, xmm4
	vpxor	xmm4, xmm11, xmm4
	vpaddd	xmm4, xmm7, xmm4
	vpaddd	xmm7, xmm4, xmmword ptr %[WORDS] + ((11 - 1) * 0x10)
	vpaddd	xmm4, xmm10, xmm4
	vpaddd	xmm4, xmm4, xmmword ptr %[WORDS] + ((12 - 1) * 0x10)
	mov	ecx, 3
	lea	rax, %[k]
	.p2align	4, 0x90
// .LBB0_1:                                # =>This Inner Loop Header: Depth=1
%=:
	vmovdqa	xmm10, xmm8
	vmovdqa	xmm8, xmm6
	vmovdqa	xmm6, xmm5
	vmovdqa	xmm5, xmm7
	vpsrld	xmm7, xmm7, 6
	vpslld	xmm11, xmm5, 26
	vpor	xmm7, xmm11, xmm7
	vpsrld	xmm11, xmm5, 11
	vpslld	xmm12, xmm5, 21
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm5, 25
	vpslld	xmm13, xmm5, 7
	vpor	xmm12, xmm13, xmm12
	vpxor	xmm11, xmm11, xmm12
	vpxor	xmm7, xmm11, xmm7
	vpand	xmm11, xmm5, xmm6
	vpandn	xmm12, xmm5, xmm8
	vpaddd	xmm10, xmm12, xmm10
	vpaddd	xmm10, xmm10, xmm11
	vpaddd	xmm7, xmm10, xmm7
	vbroadcastss	xmm10, dword ptr [rax + 4*rcx]
	vpaddd	xmm10, xmm10, xmm7
	vpaddd	xmm7, xmm10, xmm9
	vmovdqa	xmm9, xmm3
	vmovdqa	xmm3, xmm2
	vmovdqa	xmm2, xmm4
	vpsrld	xmm4, xmm4, 2
	vpslld	xmm11, xmm2, 30
	vpor	xmm4, xmm11, xmm4
	vpsrld	xmm11, xmm2, 13
	vpslld	xmm12, xmm2, 19
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm2, 22
	vpslld	xmm13, xmm2, 10
	vpor	xmm12, xmm13, xmm12
	vpxor	xmm11, xmm11, xmm12
	vpxor	xmm4, xmm11, xmm4
	vpand	xmm11, xmm2, xmm3
	vpxor	xmm12, xmm2, xmm3
	vpand	xmm12, xmm12, xmm9
	vpxor	xmm11, xmm12, xmm11
	vpaddd	xmm4, xmm11, xmm4
	vpaddd	xmm4, xmm10, xmm4
	inc	rcx
	cmp	rcx, 15
	jne	%=b

    vpsrld	xmm10, xmm4, 2
	vpslld	xmm11, xmm4, 30
	vpor	xmm10, xmm11, xmm10
	vpsrld	xmm11, xmm4, 13
	vpslld	xmm12, xmm4, 19
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm4, 22
	vpslld	xmm13, xmm4, 10
	vpor	xmm12, xmm13, xmm12
	vpxor	xmm11, xmm11, xmm12
	vpxor	xmm10, xmm11, xmm10
	vpand	xmm11, xmm4, xmm2
	vpxor	xmm12, xmm4, xmm2
	vpand	xmm12, xmm12, xmm3
	vpxor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm7, 6
	vpslld	xmm13, xmm7, 26
	vpor	xmm12, xmm13, xmm12
	vpsrld	xmm13, xmm7, 11
	vpslld	xmm14, xmm7, 21
	vpor	xmm13, xmm14, xmm13
	vpsrld	xmm14, xmm7, 25
	vpslld	xmm15, xmm7, 7
	vpor	xmm14, xmm15, xmm14
	vpxor	xmm13, xmm13, xmm14
	vpxor	xmm12, xmm13, xmm12
	vpand	xmm13, xmm7, xmm5
	vpandn	xmm14, xmm7, xmm6
	vpaddd	xmm8, xmm8, xmm14
	vpaddd	xmm8, xmm8, xmm13
	vpaddd	xmm8, xmm8, xmm12
	vpaddd	xmm12, xmm8, xmmword ptr %[WORDS] + ((13 - 1) * 0x10)
	vpaddd	xmm10, xmm11, xmm10
	vpaddd	xmm8, xmm12, xmm9
	vpaddd	xmm10, xmm10, xmm12
	vpsrld	xmm9, xmm1, 18
	vpslld	xmm11, xmm1, 14
	vpor	xmm9, xmm11, xmm9
	vpsrld	xmm11, xmm1, 3
	vpxor	xmm9, xmm9, xmm11
	vpsrld	xmm11, xmm1, 7
	vpslld	xmm12, xmm1, 25
	vpor	xmm11, xmm12, xmm11
	vpxor	xmm9, xmm9, xmm11
	vpaddd	xmm15, xmm9, xmm0
	vpsrld	xmm0, xmm10, 2
	vpslld	xmm9, xmm10, 30
	vpor	xmm0, xmm9, xmm0
	vpsrld	xmm9, xmm10, 13
	vpslld	xmm11, xmm10, 19
	vpor	xmm9, xmm11, xmm9
	vpsrld	xmm11, xmm10, 22
	vpslld	xmm12, xmm10, 10
	vpor	xmm11, xmm12, xmm11
	vpxor	xmm9, xmm9, xmm11
	vpxor	xmm0, xmm9, xmm0
	vpand	xmm9, xmm10, xmm4
	vpxor	xmm11, xmm10, xmm4
	vpand	xmm11, xmm11, xmm2
	vpxor	xmm9, xmm11, xmm9
	vpaddd	xmm0, xmm9, xmm0
	vpsrld	xmm9, xmm8, 6
	vpslld	xmm11, xmm8, 26
	vpor	xmm9, xmm11, xmm9
	vpsrld	xmm11, xmm8, 11
	vpslld	xmm12, xmm8, 21
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm8, 25
	vpslld	xmm13, xmm8, 7
	vpor	xmm12, xmm13, xmm12
	vpxor	xmm11, xmm11, xmm12
	vpxor	xmm9, xmm11, xmm9
	vpand	xmm11, xmm8, xmm7
	vpandn	xmm12, xmm8, xmm5
	vpaddd	xmm6, xmm15, xmm6
	vpaddd	xmm6, xmm12, xmm6
	vpaddd	xmm6, xmm11, xmm6
	vpaddd	xmm6, xmm9, xmm6
	vpaddd	xmm6, xmm6, xmmword ptr %[WORDS] + ((14 - 1) * 0x10)
	vpaddd	xmm12, xmm6, xmm3
	vpaddd	xmm3, xmm0, xmm6
	vpsrld	xmm0, xmm3, 2
	vpslld	xmm6, xmm3, 30
	vpor	xmm0, xmm6, xmm0
	vpsrld	xmm6, xmm3, 13
	vpslld	xmm9, xmm3, 19
	vpor	xmm6, xmm9, xmm6
	vpsrld	xmm9, xmm3, 22
	vpslld	xmm11, xmm3, 10
	vpor	xmm9, xmm11, xmm9
	vpxor	xmm6, xmm9, xmm6
	vpxor	xmm0, xmm6, xmm0
	vpand	xmm6, xmm10, xmm3
	vpxor	xmm9, xmm10, xmm3
	vpand	xmm9, xmm9, xmm4
	vpxor	xmm6, xmm9, xmm6
	vpsrld	xmm9, xmm12, 6
	vpslld	xmm11, xmm12, 26
	vpor	xmm9, xmm11, xmm9
	vpsrld	xmm11, xmm12, 11
	vpslld	xmm13, xmm12, 21
	vpor	xmm11, xmm13, xmm11
	vpsrld	xmm13, xmm12, 25
	vpslld	xmm14, xmm12, 7
	vpor	xmm13, xmm14, xmm13
	vpxor	xmm11, xmm11, xmm13
	vpxor	xmm9, xmm11, xmm9
	vpand	xmm11, xmm12, xmm8
	vpandn	xmm13, xmm12, xmm7
	vpaddd	xmm5, xmm1, xmm5
	vpaddd	xmm5, xmm13, xmm5
	vpaddd	xmm5, xmm11, xmm5
	vpaddd	xmm5, xmm9, xmm5
	vpaddd	xmm5, xmm5, xmmword ptr %[WORDS] + ((16 - 1) * 0x10)
	vpaddd	xmm0, xmm6, xmm0
	vpaddd	xmm11, xmm5, xmm2
	vpaddd	xmm6, xmm0, xmm5
	vmovdqa	xmmword ptr [rsp + 64], xmm15   # 16-byte Spill
	vpsrld	xmm0, xmm15, 19
	vpslld	xmm2, xmm15, 13
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm15, 10
	vpxor	xmm0, xmm0, xmm2
	vpsrld	xmm2, xmm15, 17
	vpslld	xmm5, xmm15, 15
	vpor	xmm2, xmm5, xmm2
	vpxor	xmm13, xmm0, xmm2
	vpsrld	xmm0, xmm6, 2
	vpslld	xmm2, xmm6, 30
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm6, 13
	vpslld	xmm5, xmm6, 19
	vpor	xmm2, xmm5, xmm2
	vpsrld	xmm5, xmm6, 22
	vpslld	xmm9, xmm6, 10
	vpor	xmm5, xmm9, xmm5
	vpxor	xmm2, xmm2, xmm5
	vpxor	xmm0, xmm2, xmm0
	vpand	xmm2, xmm6, xmm3
	vpxor	xmm5, xmm6, xmm3
	vpand	xmm5, xmm10, xmm5
	vpxor	xmm5, xmm5, xmm2
	vpsrld	xmm2, xmm11, 6
	vpslld	xmm9, xmm11, 26
	vpor	xmm9, xmm9, xmm2
	vpsrld	xmm2, xmm11, 11
	vpslld	xmm14, xmm11, 21
	vpor	xmm14, xmm14, xmm2
	vpsrld	xmm2, xmm11, 25
	vpslld	xmm15, xmm11, 7
	vpor	xmm15, xmm15, xmm2
	vpaddd	xmm2, xmm1, xmmword ptr %[WORDS] + ((15 - 1) * 0x10)
	vpxor	xmm1, xmm14, xmm15
	vpxor	xmm1, xmm9, xmm1
	vpand	xmm9, xmm11, xmm12
	vpandn	xmm14, xmm11, xmm8
	vpaddd	xmm7, xmm13, xmm7
	vpaddd	xmm7, xmm14, xmm7
	vpaddd	xmm7, xmm9, xmm7
	vpaddd	xmm1, xmm7, xmm1
	vpaddd	xmm1, xmm1, xmmword ptr %[WORDS] + ((18 - 1) * 0x10)
	vpaddd	xmm0, xmm5, xmm0
	vpaddd	xmm9, xmm1, xmm4
	vpaddd	xmm0, xmm0, xmm1
	vmovdqa	xmmword ptr [rsp], xmm2         # 16-byte Spill
	vpsrld	xmm1, xmm2, 19
	vpslld	xmm4, xmm2, 13
	vpor	xmm1, xmm4, xmm1
	vpsrld	xmm4, xmm2, 10
	vpxor	xmm1, xmm1, xmm4
	vpsrld	xmm4, xmm2, 17
	vpslld	xmm5, xmm2, 15
	vpor	xmm4, xmm5, xmm4
	vpxor	xmm2, xmm1, xmm4
	vpsrld	xmm1, xmm0, 2
	vpslld	xmm4, xmm0, 30
	vpor	xmm1, xmm4, xmm1
	vpsrld	xmm4, xmm0, 13
	vpslld	xmm7, xmm0, 19
	vpor	xmm4, xmm7, xmm4
	vpsrld	xmm7, xmm0, 22
	vpslld	xmm14, xmm0, 10
	vpor	xmm7, xmm14, xmm7
	vpxor	xmm4, xmm4, xmm7
	vpxor	xmm1, xmm4, xmm1
	vpand	xmm4, xmm0, xmm6
	vpxor	xmm7, xmm0, xmm6
	vpand	xmm7, xmm7, xmm3
	vpxor	xmm4, xmm7, xmm4
	vpsrld	xmm7, xmm9, 6
	vpslld	xmm14, xmm9, 26
	vpor	xmm7, xmm14, xmm7
	vpsrld	xmm14, xmm9, 11
	vpslld	xmm15, xmm9, 21
	vpor	xmm14, xmm15, xmm14
	vpsrld	xmm15, xmm9, 25
	vpslld	xmm5, xmm9, 7
	vpor	xmm5, xmm15, xmm5
	vpxor	xmm5, xmm14, xmm5
	vpxor	xmm5, xmm5, xmm7
	vpandn	xmm7, xmm9, xmm12
	vpaddd	xmm8, xmm8, xmm2
	vpaddd	xmm7, xmm8, xmm7
	vpxor	xmm8, xmm13, xmmword ptr %[WORDS] + ((17 - 1) * 0x10)
	vpand	xmm14, xmm9, xmm11
	vpaddd	xmm7, xmm14, xmm7
	vpaddd	xmm5, xmm7, xmm5
	vpaddd	xmm5, xmm5, xmmword ptr %[WORDS] + ((19 - 1) * 0x10)
	vpaddd	xmm1, xmm4, xmm1
	vpaddd	xmm7, xmm10, xmm5
	vpaddd	xmm1, xmm1, xmm5
	vpslld	xmm4, xmm13, 13
	vmovdqa	xmmword ptr [rsp + 80], xmm8    # 16-byte Spill
	vpsrld	xmm5, xmm8, 19
	vpor	xmm4, xmm4, xmm5
	vpsrld	xmm5, xmm8, 10
	vpxor	xmm4, xmm4, xmm5
	vpslld	xmm5, xmm13, 15
	vpsrld	xmm10, xmm8, 17
	vpor	xmm5, xmm10, xmm5
	vpxor	xmm4, xmm4, xmm5
	vpsrld	xmm5, xmm1, 2
	vpslld	xmm10, xmm1, 30
	vpor	xmm5, xmm10, xmm5
	vpsrld	xmm10, xmm1, 13
	vpslld	xmm13, xmm1, 19
	vpor	xmm10, xmm13, xmm10
	vpsrld	xmm13, xmm1, 22
	vpslld	xmm14, xmm1, 10
	vpor	xmm13, xmm14, xmm13
	vpxor	xmm10, xmm10, xmm13
	vpxor	xmm5, xmm10, xmm5
	vpand	xmm10, xmm1, xmm0
	vpxor	xmm13, xmm1, xmm0
	vpand	xmm13, xmm13, xmm6
	vpxor	xmm10, xmm13, xmm10
	vpaddd	xmm5, xmm10, xmm5
	vpsrld	xmm10, xmm7, 6
	vpslld	xmm13, xmm7, 26
	vpor	xmm10, xmm13, xmm10
	vpsrld	xmm13, xmm7, 11
	vpslld	xmm14, xmm7, 21
	vpor	xmm13, xmm14, xmm13
	vpsrld	xmm14, xmm7, 25
	vpslld	xmm15, xmm7, 7
	vpor	xmm14, xmm15, xmm14
	vpxor	xmm13, xmm13, xmm14
	vpxor	xmm10, xmm13, xmm10
	vpandn	xmm13, xmm7, xmm11
	vpaddd	xmm12, xmm12, xmm4
	vpaddd	xmm12, xmm12, xmm13
	vpand	xmm13, xmm9, xmm7
	vpaddd	xmm12, xmm12, xmm13
	vpaddd	xmm10, xmm12, xmm10
	vpaddd	xmm10, xmm10, xmmword ptr %[WORDS] + ((20 - 1) * 0x10)
	vpaddd	xmm14, xmm10, xmm3
	vpaddd	xmm10, xmm10, xmm5
	vmovdqa	xmmword ptr [rsp + 112], xmm2   # 16-byte Spill
	vpsrld	xmm3, xmm2, 19
	vpslld	xmm5, xmm2, 13
	vpor	xmm3, xmm5, xmm3
	vpsrld	xmm5, xmm2, 10
	vpxor	xmm3, xmm3, xmm5
	vpsrld	xmm5, xmm2, 17
	vpslld	xmm12, xmm2, 15
	vpor	xmm5, xmm12, xmm5
	vpxor	xmm2, xmm3, xmm5
	vpsrld	xmm3, xmm10, 2
	vpslld	xmm5, xmm10, 30
	vpor	xmm3, xmm5, xmm3
	vpsrld	xmm5, xmm10, 13
	vpslld	xmm12, xmm10, 19
	vpor	xmm5, xmm12, xmm5
	vpsrld	xmm12, xmm10, 22
	vpslld	xmm13, xmm10, 10
	vpor	xmm12, xmm13, xmm12
	vpxor	xmm5, xmm12, xmm5
	vpxor	xmm3, xmm5, xmm3
	vpxor	xmm5, xmm10, xmm1
	vpand	xmm5, xmm5, xmm0
	vpand	xmm12, xmm10, xmm1
	vpxor	xmm5, xmm12, xmm5
	vpsrld	xmm12, xmm14, 6
	vpslld	xmm13, xmm14, 26
	vpor	xmm12, xmm13, xmm12
	vpsrld	xmm13, xmm14, 11
	vpslld	xmm15, xmm14, 21
	vpor	xmm13, xmm15, xmm13
	vpsrld	xmm15, xmm14, 25
	vpslld	xmm8, xmm14, 7
	vpor	xmm8, xmm8, xmm15
	vpxor	xmm8, xmm13, xmm8
	vpxor	xmm8, xmm8, xmm12
	vpaddd	xmm11, xmm11, xmm2
	vpandn	xmm12, xmm14, xmm9
	vpaddd	xmm11, xmm11, xmm12
	vpand	xmm12, xmm14, xmm7
	vpaddd	xmm11, xmm11, xmm12
	vpaddd	xmm8, xmm11, xmm8
	vpaddd	xmm8, xmm8, xmmword ptr %[WORDS] + ((21 - 1) * 0x10)
	vpaddd	xmm3, xmm5, xmm3
	vpaddd	xmm15, xmm8, xmm6
	vpaddd	xmm11, xmm8, xmm3
	vmovdqa	xmmword ptr [rsp + 96], xmm4    # 16-byte Spill
	vpsrld	xmm3, xmm4, 19
	vpslld	xmm5, xmm4, 13
	vpor	xmm3, xmm5, xmm3
	vpsrld	xmm5, xmm4, 10
	vpxor	xmm3, xmm3, xmm5
	vpsrld	xmm5, xmm4, 17
	vpslld	xmm6, xmm4, 15
	vpor	xmm5, xmm6, xmm5
	vpxor	xmm3, xmm3, xmm5
	vpsrld	xmm5, xmm11, 2
	vpslld	xmm6, xmm11, 30
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm11, 13
	vpslld	xmm8, xmm11, 19
	vpor	xmm6, xmm8, xmm6
	vpsrld	xmm8, xmm11, 22
	vpslld	xmm12, xmm11, 10
	vpor	xmm8, xmm12, xmm8
	vpxor	xmm6, xmm8, xmm6
	vpxor	xmm5, xmm6, xmm5
	vpxor	xmm6, xmm11, xmm10
	vpand	xmm6, xmm6, xmm1
	vpand	xmm8, xmm11, xmm10
	vpxor	xmm6, xmm8, xmm6
	vpaddd	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm15, 6
	vpslld	xmm8, xmm15, 26
	vpor	xmm6, xmm8, xmm6
	vpsrld	xmm8, xmm15, 11
	vpslld	xmm12, xmm15, 21
	vpor	xmm8, xmm12, xmm8
	vpsrld	xmm12, xmm15, 25
	vpslld	xmm13, xmm15, 7
	vpor	xmm12, xmm13, xmm12
	vpxor	xmm8, xmm8, xmm12
	vpxor	xmm6, xmm8, xmm6
	vpaddd	xmm8, xmm9, xmm3
	vpandn	xmm9, xmm15, xmm7
	vpaddd	xmm8, xmm8, xmm9
	vpand	xmm9, xmm15, xmm14
	vpaddd	xmm8, xmm8, xmm9
	vpaddd	xmm6, xmm8, xmm6
	vpaddd	xmm6, xmm6, xmmword ptr %[WORDS] + ((23 - 1) * 0x10)
	vpaddd	xmm0, xmm6, xmm0
	vpaddd	xmm12, xmm5, xmm6
	vmovdqa	xmmword ptr [rsp + 48], xmm2    # 16-byte Spill
	vpsrld	xmm5, xmm2, 19
	vpslld	xmm6, xmm2, 13
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm2, 10
	vpxor	xmm5, xmm5, xmm6
	vpsrld	xmm6, xmm2, 17
	vpslld	xmm8, xmm2, 15
	vpor	xmm6, xmm8, xmm6
	vpxor	xmm5, xmm5, xmm6
	vpsrld	xmm6, xmm12, 2
	vpslld	xmm8, xmm12, 30
	vpor	xmm6, xmm8, xmm6
	vpsrld	xmm8, xmm12, 13
	vpslld	xmm9, xmm12, 19
	vpor	xmm8, xmm9, xmm8
	vpsrld	xmm9, xmm12, 22
	vpslld	xmm13, xmm12, 10
	vpor	xmm9, xmm13, xmm9
	vpxor	xmm8, xmm8, xmm9
	vpxor	xmm6, xmm8, xmm6
	vpxor	xmm8, xmm12, xmm11
	vpand	xmm8, xmm8, xmm10
	vpand	xmm9, xmm12, xmm11
	vpxor	xmm8, xmm8, xmm9
	vpaddd	xmm8, xmm8, xmm6
	vpsrld	xmm6, xmm0, 6
	vpslld	xmm9, xmm0, 26
	vpor	xmm6, xmm9, xmm6
	vpsrld	xmm9, xmm0, 11
	vpslld	xmm13, xmm0, 21
	vpor	xmm9, xmm13, xmm9
	vpsrld	xmm13, xmm0, 25
	vpslld	xmm4, xmm0, 7
	vpor	xmm4, xmm13, xmm4
	vpxor	xmm4, xmm9, xmm4
	vpxor	xmm4, xmm4, xmm6
	vmovdqa	xmm6, xmmword ptr [rsp + 64]    # 16-byte Reload
	vpaddd	xmm2, xmm5, xmm6
	vmovdqa	xmmword ptr [rsp + 16], xmm2    # 16-byte Spill
	vpaddd	xmm5, xmm2, xmm7
	vpandn	xmm7, xmm0, xmm14
	vpaddd	xmm5, xmm5, xmm7
	vpand	xmm7, xmm15, xmm0
	vpaddd	xmm5, xmm5, xmm7
	vpaddd	xmm4, xmm5, xmm4
	vpaddd	xmm4, xmm4, xmmword ptr %[WORDS] + ((24 - 1) * 0x10)
	vpaddd	xmm13, xmm4, xmm1
	vpaddd	xmm9, xmm8, xmm4
	vpaddd	xmm2, xmm3, xmmword ptr %[WORDS] + ((22 - 1) * 0x10)
	vmovdqa	xmmword ptr [rsp + 32], xmm2    # 16-byte Spill
	vpsrld	xmm1, xmm2, 19
	vpslld	xmm3, xmm2, 13
	vpor	xmm1, xmm3, xmm1
	vpsrld	xmm3, xmm2, 10
	vpxor	xmm1, xmm1, xmm3
	vpsrld	xmm3, xmm2, 17
	vpslld	xmm4, xmm2, 15
	vpor	xmm3, xmm4, xmm3
	vpxor	xmm1, xmm1, xmm3
	vmovdqa	xmmword ptr [rsp + 128], xmm6
	vmovdqa	xmm2, xmmword ptr [rsp]         # 16-byte Reload
	vmovdqa	xmmword ptr [rsp + 144], xmm2
	vpaddd	xmm7, xmm1, xmm2
	vpsrld	xmm1, xmm9, 2
	vpslld	xmm3, xmm9, 30
	vpor	xmm1, xmm3, xmm1
	vpsrld	xmm3, xmm9, 13
	vpslld	xmm4, xmm9, 19
	vpor	xmm3, xmm4, xmm3
	vpsrld	xmm4, xmm9, 22
	vpslld	xmm5, xmm9, 10
	vpor	xmm4, xmm5, xmm4
	vpxor	xmm3, xmm3, xmm4
	vpxor	xmm1, xmm3, xmm1
	vpxor	xmm3, xmm9, xmm12
	vpand	xmm3, xmm11, xmm3
	vpand	xmm4, xmm9, xmm12
	vpxor	xmm3, xmm3, xmm4
	vpsrld	xmm4, xmm13, 6
	vpslld	xmm5, xmm13, 26
	vpor	xmm4, xmm5, xmm4
	vpsrld	xmm5, xmm13, 11
	vpslld	xmm8, xmm13, 21
	vpor	xmm5, xmm8, xmm5
	vpsrld	xmm8, xmm13, 25
	vpslld	xmm2, xmm13, 7
	vpor	xmm2, xmm8, xmm2
	vpxor	xmm2, xmm5, xmm2
	vpxor	xmm2, xmm2, xmm4
	vpaddd	xmm4, xmm14, xmm7
	vpandn	xmm5, xmm13, xmm15
	vpaddd	xmm4, xmm4, xmm5
	vpand	xmm5, xmm13, xmm0
	vpaddd	xmm4, xmm4, xmm5
	vpaddd	xmm2, xmm4, xmm2
	vpaddd	xmm2, xmm2, xmmword ptr %[WORDS] + ((25 - 1) * 0x10)
	vpaddd	xmm1, xmm3, xmm1
	vpaddd	xmm14, xmm10, xmm2
	vpaddd	xmm10, xmm1, xmm2
	vmovdqa	xmm3, xmmword ptr [rsp + 16]    # 16-byte Reload
	vpsrld	xmm1, xmm3, 19
	vpslld	xmm2, xmm3, 13
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm3, 10
	vpxor	xmm1, xmm1, xmm2
	vpsrld	xmm2, xmm3, 17
	vpslld	xmm3, xmm3, 15
	vpor	xmm2, xmm3, xmm2
	vpxor	xmm1, xmm1, xmm2
	vmovdqa	xmm2, xmmword ptr [rsp + 80]    # 16-byte Reload
	vmovdqa	xmmword ptr [rsp + 160], xmm2
	vpaddd	xmm8, xmm1, xmm2
	vpsrld	xmm1, xmm10, 2
	vpslld	xmm2, xmm10, 30
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm10, 13
	vpslld	xmm3, xmm10, 19
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm10, 22
	vpslld	xmm4, xmm10, 10
	vpor	xmm3, xmm4, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpxor	xmm1, xmm2, xmm1
	vpxor	xmm2, xmm10, xmm9
	vpand	xmm2, xmm12, xmm2
	vpand	xmm3, xmm10, xmm9
	vpxor	xmm2, xmm2, xmm3
	vpsrld	xmm3, xmm14, 6
	vpslld	xmm4, xmm14, 26
	vpor	xmm3, xmm4, xmm3
	vpsrld	xmm4, xmm14, 11
	vpslld	xmm5, xmm14, 21
	vpor	xmm4, xmm5, xmm4
	vpsrld	xmm5, xmm14, 25
	vpslld	xmm6, xmm14, 7
	vpor	xmm5, xmm6, xmm5
	vpxor	xmm4, xmm4, xmm5
	vpxor	xmm3, xmm4, xmm3
	vpaddd	xmm4, xmm8, xmm15
	vpandn	xmm5, xmm14, xmm0
	vpaddd	xmm4, xmm4, xmm5
	vpand	xmm5, xmm14, xmm13
	vpaddd	xmm4, xmm4, xmm5
	vpaddd	xmm3, xmm4, xmm3
	vpaddd	xmm3, xmm3, xmmword ptr %[WORDS] + ((26 - 1) * 0x10)
	vpaddd	xmm1, xmm2, xmm1
	vpaddd	xmm15, xmm11, xmm3
	vpaddd	xmm11, xmm1, xmm3
	vmovdqa	xmmword ptr [rsp], xmm7         # 16-byte Spill
	vpsrld	xmm1, xmm7, 19
	vpslld	xmm2, xmm7, 13
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm7, 10
	vpxor	xmm1, xmm1, xmm2
	vpsrld	xmm2, xmm7, 17
	vpslld	xmm3, xmm7, 15
	vpor	xmm2, xmm3, xmm2
	vpxor	xmm1, xmm1, xmm2
	vmovdqa	xmm2, xmmword ptr [rsp + 112]   # 16-byte Reload
	vmovdqa	xmmword ptr [rsp + 176], xmm2
	vpaddd	xmm5, xmm1, xmm2
	vpsrld	xmm1, xmm11, 2
	vpslld	xmm2, xmm11, 30
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm11, 13
	vpslld	xmm3, xmm11, 19
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm11, 22
	vpslld	xmm4, xmm11, 10
	vpor	xmm3, xmm4, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpxor	xmm1, xmm2, xmm1
	vpxor	xmm2, xmm11, xmm10
	vpand	xmm2, xmm9, xmm2
	vpand	xmm3, xmm11, xmm10
	vpxor	xmm2, xmm2, xmm3
	vpsrld	xmm3, xmm15, 6
	vpslld	xmm4, xmm15, 26
	vpor	xmm3, xmm4, xmm3
	vpsrld	xmm4, xmm15, 11
	vpslld	xmm6, xmm15, 21
	vpor	xmm4, xmm6, xmm4
	vpsrld	xmm6, xmm15, 25
	vpslld	xmm7, xmm15, 7
	vpor	xmm6, xmm7, xmm6
	vpxor	xmm4, xmm4, xmm6
	vpxor	xmm3, xmm4, xmm3
	vpaddd	xmm0, xmm5, xmm0
	vpandn	xmm4, xmm15, xmm13
	vpaddd	xmm0, xmm0, xmm4
	vpand	xmm4, xmm15, xmm14
	vpaddd	xmm0, xmm0, xmm4
	vpaddd	xmm0, xmm0, xmm3
	vpaddd	xmm0, xmm0, xmmword ptr %[WORDS] + ((27 - 1) * 0x10)
	vpaddd	xmm2, xmm2, xmm1
	vpaddd	xmm1, xmm12, xmm0
	vpaddd	xmm12, xmm2, xmm0
	vpsrld	xmm0, xmm8, 19
	vpslld	xmm2, xmm8, 13
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm8, 10
	vpxor	xmm0, xmm0, xmm2
	vmovdqa	xmm4, xmmword ptr [rsp + 96]    # 16-byte Reload
	vmovdqa	xmmword ptr [rsp + 192], xmm4
	vmovaps	xmm2, xmmword ptr [rsp + 48]    # 16-byte Reload
	vmovaps	xmmword ptr [rsp + 208], xmm2
	vmovaps	xmm2, xmmword ptr [rsp + 32]    # 16-byte Reload
	vmovaps	xmmword ptr [rsp + 224], xmm2
	vmovaps	xmm2, xmmword ptr [rsp + 16]    # 16-byte Reload
	vmovaps	xmmword ptr [rsp + 240], xmm2
	vmovaps	xmm2, xmmword ptr [rsp]         # 16-byte Reload
	vmovaps	xmmword ptr [rsp + 256], xmm2
	vmovdqa	xmmword ptr [rsp + 272], xmm8
	vpsrld	xmm2, xmm8, 17
	vpslld	xmm3, xmm8, 15
	vpor	xmm2, xmm3, xmm2
	vpxor	xmm0, xmm0, xmm2
	vpaddd	xmm4, xmm0, xmm4
	vpsrld	xmm0, xmm12, 2
	vpslld	xmm2, xmm12, 30
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm12, 13
	vpslld	xmm3, xmm12, 19
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm12, 22
	vpslld	xmm6, xmm12, 10
	vpor	xmm3, xmm6, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpxor	xmm0, xmm2, xmm0
	vpxor	xmm2, xmm12, xmm11
	vpand	xmm2, xmm10, xmm2
	vpand	xmm3, xmm12, xmm11
	vpxor	xmm2, xmm2, xmm3
	vpaddd	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm1, 6
	vpslld	xmm3, xmm1, 26
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm1, 11
	vpslld	xmm6, xmm1, 21
	vpor	xmm3, xmm6, xmm3
	vpsrld	xmm6, xmm1, 25
	vpslld	xmm7, xmm1, 7
	vpor	xmm6, xmm7, xmm6
	vpxor	xmm3, xmm3, xmm6
	vpxor	xmm2, xmm3, xmm2
	vpaddd	xmm3, xmm13, xmm4
	vpandn	xmm6, xmm1, xmm14
	vpaddd	xmm3, xmm3, xmm6
	vpand	xmm6, xmm15, xmm1
	vpaddd	xmm3, xmm3, xmm6
	vpaddd	xmm2, xmm3, xmm2
	vpaddd	xmm2, xmm2, xmmword ptr %[WORDS] + ((28 - 1) * 0x10)
	vpaddd	xmm13, xmm9, xmm2
	vpaddd	xmm9, xmm0, xmm2
	vpsrld	xmm0, xmm5, 19
	vpslld	xmm2, xmm5, 13
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm5, 10
	vpxor	xmm0, xmm0, xmm2
	vmovdqa	xmmword ptr [rsp + 288], xmm5
	vpsrld	xmm2, xmm5, 17
	vpslld	xmm3, xmm5, 15
	vpor	xmm2, xmm3, xmm2
	vpxor	xmm0, xmm0, xmm2
	vpaddd	xmm0, xmm0, xmmword ptr [rsp + 48] # 16-byte Folded Reload
	vpsrld	xmm2, xmm9, 2
	vpslld	xmm3, xmm9, 30
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm9, 13
	vpslld	xmm5, xmm9, 19
	vpor	xmm3, xmm5, xmm3
	vpsrld	xmm5, xmm9, 22
	vpslld	xmm6, xmm9, 10
	vpor	xmm5, xmm6, xmm5
	vpxor	xmm3, xmm3, xmm5
	vpxor	xmm2, xmm3, xmm2
	vpxor	xmm3, xmm9, xmm12
	vpand	xmm3, xmm11, xmm3
	vpand	xmm5, xmm9, xmm12
	vpxor	xmm3, xmm3, xmm5
	vpaddd	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm13, 6
	vpslld	xmm5, xmm13, 26
	vpor	xmm3, xmm5, xmm3
	vpsrld	xmm5, xmm13, 11
	vpslld	xmm6, xmm13, 21
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm13, 25
	vpslld	xmm7, xmm13, 7
	vpor	xmm6, xmm7, xmm6
	vpxor	xmm5, xmm5, xmm6
	vpxor	xmm3, xmm5, xmm3
	vpaddd	xmm5, xmm14, xmm0
	vpandn	xmm6, xmm13, xmm15
	vpaddd	xmm5, xmm5, xmm6
	vpand	xmm6, xmm13, xmm1
	vpaddd	xmm5, xmm5, xmm6
	vpaddd	xmm3, xmm5, xmm3
	vpaddd	xmm5, xmm3, xmmword ptr %[WORDS] + ((29 - 1) * 0x10)
	vpaddd	xmm3, xmm10, xmm5
	vpaddd	xmm8, xmm2, xmm5
	vpsrld	xmm2, xmm4, 19
	vpslld	xmm5, xmm4, 13
	vpor	xmm2, xmm5, xmm2
	vpsrld	xmm5, xmm4, 10
	vpxor	xmm2, xmm2, xmm5
	vmovdqa	xmmword ptr [rsp + 304], xmm4
	vpsrld	xmm5, xmm4, 17
	vpslld	xmm4, xmm4, 15
	vpor	xmm4, xmm4, xmm5
	vpxor	xmm2, xmm2, xmm4
	vpaddd	xmm10, xmm2, xmmword ptr [rsp + 32] # 16-byte Folded Reload
	vpsrld	xmm2, xmm8, 2
	vpslld	xmm4, xmm8, 30
	vpor	xmm2, xmm4, xmm2
	vpsrld	xmm4, xmm8, 13
	vpslld	xmm5, xmm8, 19
	vpor	xmm4, xmm5, xmm4
	vpsrld	xmm5, xmm8, 22
	vpslld	xmm6, xmm8, 10
	vpor	xmm5, xmm6, xmm5
	vpxor	xmm4, xmm4, xmm5
	vpxor	xmm2, xmm4, xmm2
	vpxor	xmm4, xmm8, xmm9
	vpand	xmm4, xmm12, xmm4
	vpand	xmm5, xmm8, xmm9
	vpxor	xmm4, xmm4, xmm5
	vpaddd	xmm2, xmm4, xmm2
	vpsrld	xmm4, xmm3, 6
	vpslld	xmm5, xmm3, 26
	vpor	xmm4, xmm5, xmm4
	vpsrld	xmm5, xmm3, 11
	vpslld	xmm6, xmm3, 21
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm3, 25
	vpslld	xmm7, xmm3, 7
	vpor	xmm6, xmm7, xmm6
	vpxor	xmm5, xmm5, xmm6
	vpxor	xmm4, xmm5, xmm4
	vpaddd	xmm5, xmm10, xmm15
	vpandn	xmm6, xmm3, xmm1
	vpaddd	xmm5, xmm5, xmm6
	vpand	xmm6, xmm13, xmm3
	vpaddd	xmm5, xmm5, xmm6
	vmovdqa	xmmword ptr [rsp + 320], xmm0
	vpaddd	xmm4, xmm5, xmm4
	vpaddd	xmm5, xmm4, xmmword ptr %[WORDS] + ((30 - 1) * 0x10)
	vpaddd	xmm4, xmm11, xmm5
	vpaddd	xmm5, xmm2, xmm5
	vpsrld	xmm2, xmm0, 19
	vpslld	xmm6, xmm0, 13
	vpor	xmm2, xmm6, xmm2
	vpsrld	xmm6, xmm0, 10
	vpxor	xmm2, xmm2, xmm6
	vpsrld	xmm6, xmm0, 17
	vpslld	xmm0, xmm0, 15
	vpor	xmm0, xmm0, xmm6
	vpxor	xmm0, xmm2, xmm0
	vpaddd	xmm11, xmm0, xmmword ptr [rsp + 16] # 16-byte Folded Reload
	vpsrld	xmm0, xmm5, 2
	vpslld	xmm2, xmm5, 30
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm5, 13
	vpslld	xmm6, xmm5, 19
	vpor	xmm2, xmm6, xmm2
	vpsrld	xmm6, xmm5, 22
	vpslld	xmm7, xmm5, 10
	vpor	xmm6, xmm7, xmm6
	vpxor	xmm2, xmm2, xmm6
	vpxor	xmm0, xmm2, xmm0
	vpand	xmm2, xmm8, xmm5
	vpxor	xmm6, xmm8, xmm5
	vpand	xmm6, xmm9, xmm6
	vpxor	xmm2, xmm6, xmm2
	vpaddd	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm4, 6
	vpslld	xmm6, xmm4, 26
	vpor	xmm2, xmm6, xmm2
	vpsrld	xmm6, xmm4, 11
	vpslld	xmm7, xmm4, 21
	vpor	xmm6, xmm7, xmm6
	vpsrld	xmm7, xmm4, 25
	vpslld	xmm14, xmm4, 7
	vpor	xmm7, xmm14, xmm7
	vpxor	xmm6, xmm6, xmm7
	vpxor	xmm2, xmm6, xmm2
	vpandn	xmm6, xmm4, xmm13
	vpaddd	xmm1, xmm11, xmm1
	vpaddd	xmm1, xmm1, xmm6
	vpand	xmm6, xmm4, xmm3
	vpaddd	xmm1, xmm1, xmm6
	vmovdqa	xmmword ptr [rsp + 336], xmm10
	vpaddd	xmm1, xmm1, xmm2
	vpaddd	xmm1, xmm1, xmmword ptr %[WORDS] + ((32 - 1) * 0x10)
	vpaddd	xmm6, xmm12, xmm1
	vpaddd	xmm7, xmm0, xmm1
	vpsrld	xmm0, xmm10, 19
	vpslld	xmm1, xmm10, 13
	vpor	xmm0, xmm1, xmm0
	vpsrld	xmm1, xmm10, 10
	vpxor	xmm0, xmm0, xmm1
	vpsrld	xmm1, xmm10, 17
	vpslld	xmm2, xmm10, 15
	vpor	xmm1, xmm2, xmm1
	vpxor	xmm0, xmm0, xmm1
	vmovdqa	xmm10, xmmword ptr [rsp + 64]   # 16-byte Reload
	vpsrld	xmm1, xmm10, 18
	vpslld	xmm2, xmm10, 14
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm10, 3
	vpxor	xmm1, xmm1, xmm2
	vpsrld	xmm2, xmm10, 7
	vpslld	xmm10, xmm10, 25
	vpor	xmm2, xmm10, xmm2
	vpxor	xmm1, xmm1, xmm2
	vpaddd	xmm1, xmm1, xmmword ptr [rsp]   # 16-byte Folded Reload
	vpaddd	xmm1, xmm1, xmmword ptr %[WORDS] + ((22 - 1) * 0x10)
	vpaddd	xmm0, xmm1, xmm0
	vpsrld	xmm1, xmm7, 2
	vpslld	xmm2, xmm7, 30
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm7, 13
	vpslld	xmm10, xmm7, 19
	vpor	xmm2, xmm10, xmm2
	vpsrld	xmm10, xmm7, 22
	vpslld	xmm12, xmm7, 10
	vpor	xmm10, xmm12, xmm10
	vpxor	xmm2, xmm10, xmm2
	vpxor	xmm1, xmm2, xmm1
	vpand	xmm2, xmm7, xmm5
	vpxor	xmm10, xmm7, xmm5
	vpand	xmm10, xmm10, xmm8
	vpxor	xmm2, xmm10, xmm2
	vpaddd	xmm2, xmm2, xmm1
	vpsrld	xmm1, xmm6, 6
	vpslld	xmm10, xmm6, 26
	vpor	xmm1, xmm10, xmm1
	vpsrld	xmm10, xmm6, 11
	vpslld	xmm12, xmm6, 21
	vpor	xmm10, xmm12, xmm10
	vpsrld	xmm12, xmm6, 25
	vpslld	xmm14, xmm6, 7
	vpor	xmm12, xmm14, xmm12
	vpxor	xmm10, xmm10, xmm12
	vpxor	xmm10, xmm10, xmm1
	vpandn	xmm1, xmm6, xmm3
	vpaddd	xmm12, xmm13, xmm0
	vpaddd	xmm1, xmm12, xmm1
	vpand	xmm12, xmm6, xmm4
	vpaddd	xmm12, xmm12, xmm1
	vpaddd	xmm1, xmm11, xmmword ptr %[WORDS] + ((31 - 1) * 0x10)
	vpaddd	xmm10, xmm12, xmm10
	vpaddd	xmm11, xmm10, xmmword ptr %[WORDS] + ((33 - 1) * 0x10)
	vpaddd	xmm10, xmm11, xmm9
	vpaddd	xmm2, xmm11, xmm2
	vmovdqa	xmmword ptr [rsp + 352], xmm1
	vmovdqa	xmmword ptr [rsp + 368], xmm0
	mov	edx, 32
	.p2align	4, 0x90
// .LBB0_3:                                # =>This Inner Loop Header: Depth=1
%=:
	vmovdqa	xmm9, xmm3
	vmovdqa	xmm3, xmm4
	vmovdqa	xmm4, xmm6
	vmovdqa	xmm6, xmm10
	lea	rcx, [rdx + 1]
	lea	esi, [rdx + 9]
	vbroadcastss	xmm10, dword ptr [rax + 4*rdx]
                                        # kill: def $edx killed $edx killed $rdx def $rdx
	and	edx, 15
	shl	edx, 4
	mov	r8d, ecx
	and	r8d, 15
	shl	r8d, 4
	vmovdqa	xmm11, xmmword ptr [rsp + r8 + 128]
	and	esi, 15
	shl	esi, 4
	vpsrld	xmm12, xmm1, 19
	vpslld	xmm13, xmm1, 13
	vpor	xmm12, xmm13, xmm12
	vpsrld	xmm13, xmm1, 10
	vpxor	xmm12, xmm12, xmm13
	vpsrld	xmm13, xmm1, 17
	vpslld	xmm1, xmm1, 15
	vpor	xmm1, xmm13, xmm1
	vpxor	xmm1, xmm12, xmm1
	vpsrld	xmm12, xmm11, 18
	vpslld	xmm13, xmm11, 14
	vpor	xmm12, xmm13, xmm12
	vpsrld	xmm13, xmm11, 3
	vpxor	xmm12, xmm12, xmm13
	vpsrld	xmm13, xmm11, 7
	vpslld	xmm11, xmm11, 25
	vpor	xmm11, xmm11, xmm13
	vpaddd	xmm1, xmm1, xmmword ptr [rsp + rdx + 128]
	vpxor	xmm11, xmm12, xmm11
	vpaddd	xmm12, xmm1, xmmword ptr [rsp + rsi + 128]
	vmovdqa	xmm1, xmm0
	vpaddd	xmm0, xmm12, xmm11
	vpsrld	xmm11, xmm6, 6
	vpslld	xmm12, xmm6, 26
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm6, 11
	vpslld	xmm13, xmm6, 21
	vpor	xmm12, xmm13, xmm12
	vpsrld	xmm13, xmm6, 25
	vpslld	xmm14, xmm6, 7
	vpor	xmm13, xmm14, xmm13
	vpxor	xmm12, xmm12, xmm13
	vpxor	xmm11, xmm12, xmm11
	vpand	xmm12, xmm6, xmm4
	vpandn	xmm13, xmm6, xmm3
	vpaddd	xmm9, xmm13, xmm9
	vpaddd	xmm9, xmm9, xmm12
	vpaddd	xmm9, xmm9, xmm11
	vpaddd	xmm9, xmm9, xmm10
	vpaddd	xmm9, xmm9, xmm0
	vpaddd	xmm10, xmm9, xmm8
	vmovdqa	xmm8, xmm5
	vmovdqa	xmm5, xmm7
	vmovdqa	xmm7, xmm2
	vmovdqa	xmmword ptr [rsp + rdx + 128], xmm0
	vpsrld	xmm2, xmm2, 2
	vpslld	xmm11, xmm7, 30
	vpor	xmm2, xmm11, xmm2
	vpsrld	xmm11, xmm7, 13
	vpslld	xmm12, xmm7, 19
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm7, 22
	vpslld	xmm13, xmm7, 10
	vpor	xmm12, xmm13, xmm12
	vpxor	xmm11, xmm11, xmm12
	vpxor	xmm2, xmm11, xmm2
	vpand	xmm11, xmm7, xmm5
	vpxor	xmm12, xmm7, xmm5
	vpand	xmm12, xmm12, xmm8
	vpxor	xmm11, xmm12, xmm11
	vpaddd	xmm2, xmm11, xmm2
	vpaddd	xmm2, xmm9, xmm2
	mov	rdx, rcx
	cmp	rcx, 64
	jne	%=b

    vpaddd	xmm0, xmm2, xmmword ptr %[WORDS] + ((9 - 1) * 0x10)
	vpaddd	xmm1, xmm7, xmmword ptr %[WORDS] + ((34 - 1) * 0x10)
	vpaddd	xmm2, xmm5, xmmword ptr %[WORDS] + ((35 - 1) * 0x10)
	vpaddd	xmm5, xmm8, xmmword ptr %[WORDS] + ((36 - 1) * 0x10)
	vpaddd	xmm7, xmm10, xmmword ptr %[WORDS] + ((4 - 1) * 0x10)
	vpaddd	xmm6, xmm6, xmmword ptr %[WORDS] + ((6 - 1) * 0x10)
	vpaddd	xmm4, xmm4, xmmword ptr %[WORDS] + ((37 - 1) * 0x10)
	vpaddd	xmm3, xmm3, xmmword ptr %[WORDS] + ((38 - 1) * 0x10)
	vpunpckldq	xmm8, xmm0, xmm1        # xmm8 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
	vpunpckhdq	xmm0, xmm0, xmm1        # xmm0 = xmm0[2],xmm1[2],xmm0[3],xmm1[3]
	vpunpckldq	xmm1, xmm2, xmm5        # xmm1 = xmm2[0],xmm5[0],xmm2[1],xmm5[1]
	vpunpckhdq	xmm2, xmm2, xmm5        # xmm2 = xmm2[2],xmm5[2],xmm2[3],xmm5[3]
	vpunpckldq	xmm5, xmm7, xmm6        # xmm5 = xmm7[0],xmm6[0],xmm7[1],xmm6[1]
	vpunpckhdq	xmm6, xmm7, xmm6        # xmm6 = xmm7[2],xmm6[2],xmm7[3],xmm6[3]
	vpunpckldq	xmm7, xmm4, xmm3        # xmm7 = xmm4[0],xmm3[0],xmm4[1],xmm3[1]
	vpunpckhdq	xmm3, xmm4, xmm3        # xmm3 = xmm4[2],xmm3[2],xmm4[3],xmm3[3]
	vpunpcklbw	xmm4, xmm8, xmm1        # xmm4 = xmm8[0],xmm1[0],xmm8[1],xmm1[1],xmm8[2],xmm1[2],xmm8[3],xmm1[3],xmm8[4],xmm1[4],xmm8[5],xmm1[5],xmm8[6],xmm1[6],xmm8[7],xmm1[7]
	vmovdqa	xmm9, xmmword ptr %[BYTES]+0x10 # xmm9 = [6,4,2,0,14,12,10,8,7,5,3,1,15,13,11,9]
	vpshufb	xmm4, xmm4, xmm9
	vpunpckhbw	xmm1, xmm8, xmm1        # xmm1 = xmm8[8],xmm1[8],xmm8[9],xmm1[9],xmm8[10],xmm1[10],xmm8[11],xmm1[11],xmm8[12],xmm1[12],xmm8[13],xmm1[13],xmm8[14],xmm1[14],xmm8[15],xmm1[15]
	vpshufb	xmm1, xmm1, xmm9
	vpunpcklbw	xmm8, xmm0, xmm2        # xmm8 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3],xmm0[4],xmm2[4],xmm0[5],xmm2[5],xmm0[6],xmm2[6],xmm0[7],xmm2[7]
	vpshufb	xmm8, xmm8, xmm9
	vpunpckhbw	xmm0, xmm0, xmm2        # xmm0 = xmm0[8],xmm2[8],xmm0[9],xmm2[9],xmm0[10],xmm2[10],xmm0[11],xmm2[11],xmm0[12],xmm2[12],xmm0[13],xmm2[13],xmm0[14],xmm2[14],xmm0[15],xmm2[15]
	vpshufb	xmm0, xmm0, xmm9
	vpunpcklbw	xmm2, xmm5, xmm7        # xmm2 = xmm5[0],xmm7[0],xmm5[1],xmm7[1],xmm5[2],xmm7[2],xmm5[3],xmm7[3],xmm5[4],xmm7[4],xmm5[5],xmm7[5],xmm5[6],xmm7[6],xmm5[7],xmm7[7]
	vpshufb	xmm2, xmm2, xmm9
	vpunpckhbw	xmm5, xmm5, xmm7        # xmm5 = xmm5[8],xmm7[8],xmm5[9],xmm7[9],xmm5[10],xmm7[10],xmm5[11],xmm7[11],xmm5[12],xmm7[12],xmm5[13],xmm7[13],xmm5[14],xmm7[14],xmm5[15],xmm7[15]
	vpshufb	xmm5, xmm5, xmm9
	vpunpcklbw	xmm7, xmm6, xmm3        # xmm7 = xmm6[0],xmm3[0],xmm6[1],xmm3[1],xmm6[2],xmm3[2],xmm6[3],xmm3[3],xmm6[4],xmm3[4],xmm6[5],xmm3[5],xmm6[6],xmm3[6],xmm6[7],xmm3[7]
	vpshufb	xmm7, xmm7, xmm9
	vpunpckhbw	xmm3, xmm6, xmm3        # xmm3 = xmm6[8],xmm3[8],xmm6[9],xmm3[9],xmm6[10],xmm3[10],xmm6[11],xmm3[11],xmm6[12],xmm3[12],xmm6[13],xmm3[13],xmm6[14],xmm3[14],xmm6[15],xmm3[15]
	vpshufb	xmm3, xmm3, xmm9
	vmovdqa	xmmword ptr [rdi], xmm4
	vmovdqa	xmmword ptr [rdi + 16], xmm2
	vmovdqa	xmmword ptr [rdi + 32], xmm1
	vmovdqa	xmmword ptr [rdi + 48], xmm5
	vmovdqa	xmmword ptr [rdi + 64], xmm8
	vmovdqa	xmmword ptr [rdi + 80], xmm7
	vmovdqa	xmmword ptr [rdi + 96], xmm0
	vmovdqa	xmmword ptr [rdi + 112], xmm3

    // restore stack
	add	rsp, 392
	ret
        )"""
        :
        : "rdi"(hash),
          "rsi"(data),
          // constants
          [k] "m"(k),
          [BYTES] "m"(BYTES),
          [WORDS] "m"(WORDS),
          [SINGLE_WORD_0] "m"(SINGLE_WORD_0),
          [SINGLE_WORD_1] "m"(SINGLE_WORD_1),
          [SINGLE_WORD_2] "m"(SINGLE_WORD_2),
          [SINGLE_WORD_3] "m"(SINGLE_WORD_3)
        : // TODO :: proper clobbers based off of final assembly
        "rax",
        "rcx",
        "rdx",
        "r8",
        "edx",
        // "rsi", "rbx", // (callee saved)
        // "r8", "r9", "r10", "r11", (not using)
        // "r12", "r13", "r14", "r15", // (callee saved)
        "xmm0",
        "xmm1",
        "xmm2",
        "xmm3",
        "xmm4",
        "xmm5",
        "xmm6",
        "xmm7",
        "xmm8",
        "xmm9",
        "xmm10",
        "xmm11",
        "xmm12",
        "xmm13",
        "xmm14",
        "xmm15",
        "memory"
    );
}