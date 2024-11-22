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

static const uint32_t WORDS[] __attribute__((aligned(16))) = {
    0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f, /* BYTES */
    0x98c7e2a2, 0x98c7e2a2, 0x98c7e2a2, 0x98c7e2a2, /*  1 */
    0xfc08884d, 0xfc08884d, 0xfc08884d, 0xfc08884d, /*  2 */
    0xd16e48e2, 0xd16e48e2, 0xd16e48e2, 0xd16e48e2, /*  3 */
    0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f, /*  4 */
    0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c, /*  5 */
    0xcd2a11ae, 0xcd2a11ae, 0xcd2a11ae, 0xcd2a11ae, /*  6 */
    0xbabcc441, 0xbabcc441, 0xbabcc441, 0xbabcc441, /*  7 */
    0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667, /*  8 */
    0x8c2e12e0, 0x8c2e12e0, 0x8c2e12e0, 0x8c2e12e0, /*  9 */
    0xd0c6645b, 0xd0c6645b, 0xd0c6645b, 0xd0c6645b, /* 10 */
    0xc19bf1b4, 0xc19bf1b4, 0xc19bf1b4, 0xc19bf1b4, /* 11 */
    0xe49b69c1, 0xe49b69c1, 0xe49b69c1, 0xe49b69c1, /* 12 */
    0x11282000, 0x11282000, 0x11282000, 0x11282000, /* 13 */
    0xe66786,   0xe66786,   0xe66786,   0xe66786,   /* 14 */
    0x80000000, 0x80000000, 0x80000000, 0x80000000, /* 15 */
    0x8fc19dc6, 0x8fc19dc6, 0x8fc19dc6, 0x8fc19dc6, /* 16 */
    0x240ca1cc, 0x240ca1cc, 0x240ca1cc, 0x240ca1cc, /* 17 */
    0x2de92c6f, 0x2de92c6f, 0x2de92c6f, 0x2de92c6f, /* 18 */
    0x4a7484aa, 0x4a7484aa, 0x4a7484aa, 0x4a7484aa, /* 19 */
    0x40,       0x40,       0x40,       0x40,       /* 20 */
    0x5cb0aa1c, 0x5cb0aa1c, 0x5cb0aa1c, 0x5cb0aa1c, /* 21 */
    0x76f988da, 0x76f988da, 0x76f988da, 0x76f988da, /* 22 */
    0x983e5152, 0x983e5152, 0x983e5152, 0x983e5152, /* 23 */
    0xa831c66d, 0xa831c66d, 0xa831c66d, 0xa831c66d, /* 24 */
    0xb00327c8, 0xb00327c8, 0xb00327c8, 0xb00327c8, /* 25 */
    0xbf597fc7, 0xbf597fc7, 0xbf597fc7, 0xbf597fc7, /* 26 */
    0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, /* 27 */
    0xd5a79147, 0xd5a79147, 0xd5a79147, 0xd5a79147, /* 28 */
    0x80100008, 0x80100008, 0x80100008, 0x80100008, /* 29 */
    0x86da6359, 0x86da6359, 0x86da6359, 0x86da6359, /* 30 */
    0x14292967, 0x14292967, 0x14292967, 0x14292967, /* 31 */
    0x00000004, 0x00000000, 0x00000004, 0x00000000, /* 32 */
    0x00000004, 0x00000000, 0x00000000, 0x00000000, /* 33 */
    0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f, /* 34 */
    0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667, /* 35 */
    0x40,       0x40,       0x40,       0x40,       /* 36 */
};

uint64_t reverse_sha256x4_fullyfused_asm_2(
    uint64_t start,
    uint64_t end,
    const HashDigest target
) {
    __asm__ volatile (
        R"""(
	.p2align	4, 0x90

	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	push	r14
	push	rbx
	and	rsp, -64
	sub	rsp, 1152
	.cfi_offset rbx, -32
	.cfi_offset r14, -24
	mov	rax, qword ptr fs:[40]
	mov	qword ptr [rsp + 1112], rax
	mov	r8d, dword ptr [rdx + 16]
	mov	r11d, dword ptr [rdx + 4]
	mov	r10d, r11d
	mov	r9d, r11d
	mov	ecx, dword ptr [rdx + 8]
	mov	eax, dword ptr [rdx + 12]
	mov	ebx, eax
	xor	ebx, ecx
	and	ebx, r11d
	mov	r14d, dword ptr [rdx + 20]
	vmovd	xmm0, r14d
	sub	r14d, r11d
	vmovd	xmm1, r11d
	shld	r11d, r11d, 30
	shld	r10d, r10d, 19
	xor	r10d, r11d
	mov	r11d, dword ptr [rdx]
	shld	r9d, r9d, 10
	xor	r9d, r10d
	mov	r10d, eax
	and	r10d, ecx
	xor	r10d, ebx
	vmovd	xmm2, r8d
	sub	r8d, r11d
	add	r8d, r10d
	add	r8d, r9d
	mov	r10d, ecx
	mov	ebx, r8d
	xor	ebx, eax
	and	ebx, ecx
	mov	r9d, ecx
	shld	r9d, r9d, 30
	shld	r10d, r10d, 19
	xor	r10d, r9d
	mov	r9d, dword ptr [rdx + 24]
	vmovd	xmm3, r9d
	sub	r9d, ecx
	vmovd	xmm4, ecx
	shld	ecx, ecx, 10
	xor	ecx, r10d
	mov	r10d, r8d
	and	r10d, eax
	xor	r10d, ebx
	add	r14d, ecx
	add	r14d, r10d
	mov	ecx, eax
	shld	ecx, ecx, 30
	mov	r10d, eax
	shld	r10d, r10d, 19
	xor	r10d, ecx
	mov	ecx, r14d
	xor	ecx, r8d
	and	ecx, eax
	mov	edx, dword ptr [rdx + 28]
	vmovd	xmm5, edx
	sub	edx, eax
	vmovd	xmm6, eax
	shld	eax, eax, 10
	xor	eax, r10d
	mov	r10d, r14d
	and	r10d, r8d
	xor	r10d, ecx
	add	r9d, eax
	add	r9d, r10d
	mov	eax, r8d
	shld	eax, eax, 30
	mov	ecx, r8d
	shld	ecx, ecx, 19
	xor	ecx, eax
	mov	eax, r9d
	xor	eax, r14d
	and	eax, r8d
	shld	r8d, r8d, 10
	xor	r8d, ecx
	and	r9d, r14d
	xor	r9d, eax
	add	edx, r8d
	add	edx, r9d
	vmovd	xmm7, edx
	mov	qword ptr [rsp + 1024], rdi
	lea	rax, [rdi + 1]
	mov	qword ptr [rsp + 1032], rax
	lea	rax, [rdi + 2]
	mov	qword ptr [rsp + 1040], rax
	vpshufd	xmm7, xmm7, 0                   # xmm7 = xmm7[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 208], xmm7   # 16-byte Spill
	add	rdi, 3
	mov	qword ptr [rsp + 1048], rdi
	vmovd	xmm7, r11d
	vpshufd	xmm7, xmm7, 0                   # xmm7 = xmm7[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 192], xmm7   # 16-byte Spill
	vpshufd	xmm1, xmm1, 0                   # xmm1 = xmm1[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 176], xmm1   # 16-byte Spill
	vpshufd	xmm1, xmm4, 0                   # xmm1 = xmm4[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 160], xmm1   # 16-byte Spill
	vpshufd	xmm1, xmm6, 0                   # xmm1 = xmm6[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 144], xmm1   # 16-byte Spill
	vpshufd	xmm1, xmm2, 0                   # xmm1 = xmm2[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 128], xmm1   # 16-byte Spill
	vpshufd	xmm0, xmm0, 0                   # xmm0 = xmm0[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 112], xmm0   # 16-byte Spill
	vpshufd	xmm0, xmm3, 0                   # xmm0 = xmm3[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 96], xmm0    # 16-byte Spill
	vpshufd	xmm0, xmm5, 0                   # xmm0 = xmm5[0,0,0,0]
	vmovdqa	xmmword ptr [rsp + 80], xmm0    # 16-byte Spill
	lea	rcx, %[k]
	xor	eax, eax
	jmp %=1f #	.LBB0_1
	.p2align	4, 0x90
%=10: #.LBB0_10:                               #   in Loop: Header=BB0_1 Depth=1
	vmovddup	xmm1, qword ptr %[WORDS] + ( 33 * 0x10 ) # xmm1 = [4,4]
                                        # xmm1 = mem[0,0]
	vpaddq	xmm0, xmm1, xmmword ptr [rsp + 1040]
	vpaddq	xmm1, xmm1, xmmword ptr [rsp + 1024]
	vmovdqa	xmmword ptr [rsp + 1024], xmm1
	vmovdqa	xmmword ptr [rsp + 1040], xmm0
	vmovq	rdx, xmm1
	cmp	rdx, rsi
	ja %=11f #	.LBB0_11
%=1: # .LBB0_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_2 Depth 2
                                        #     Child Loop BB0_4 Depth 2
                                        #     Child Loop BB0_7 Depth 2
	vmovdqa	xmm0, xmmword ptr [rsp + 1024]
	vmovdqa	xmm1, xmmword ptr [rsp + 1040]
	vmovdqa	xmm2, xmmword ptr %[WORDS] + ( 0 * 0x10 ) # xmm2 = [3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12]
	vpshufb	xmm0, xmm0, xmm2
	vpshufb	xmm1, xmm1, xmm2
	vpunpckldq	xmm2, xmm0, xmm1        # xmm2 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
	vpunpckhdq	xmm0, xmm0, xmm1        # xmm0 = xmm0[2],xmm1[2],xmm0[3],xmm1[3]
	vpunpckldq	xmm4, xmm2, xmm0        # xmm4 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
	vpunpckhdq	xmm5, xmm2, xmm0        # xmm5 = xmm2[2],xmm0[2],xmm2[3],xmm0[3]
	vpaddd	xmm12, xmm4, xmmword ptr %[WORDS] + ( 1 * 0x10 )
	vpaddd	xmm8, xmm4, xmmword ptr %[WORDS] + ( 2 * 0x10 )
	vpsrld	xmm0, xmm8, 2
	vpslld	xmm1, xmm8, 30
	vpor	xmm0, xmm1, xmm0
	vpsrld	xmm1, xmm8, 13
	vpslld	xmm2, xmm8, 19
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm8, 22
	vpslld	xmm3, xmm8, 10
	vpor	xmm2, xmm3, xmm2
	vpxor	xmm1, xmm1, xmm2
	vpxor	xmm0, xmm1, xmm0
	vpand	xmm1, xmm8, xmmword ptr %[WORDS] + ( 3 * 0x10 )
	vpaddd	xmm0, xmm0, xmm1
	vpsrld	xmm1, xmm12, 6
	vpslld	xmm2, xmm12, 26
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm12, 11
	vpslld	xmm3, xmm12, 21
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm12, 25
	vpslld	xmm6, xmm12, 7
	vpor	xmm3, xmm6, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpxor	xmm1, xmm2, xmm1
	vbroadcastss	xmm9, dword ptr %[WORDS] + ( 34 * 0x10 ) # xmm9 = [1359893119,1359893119,1359893119,1359893119]
	vpand	xmm2, xmm12, xmm9
	vpandn	xmm3, xmm12, xmmword ptr %[WORDS] + ( 5 * 0x10 )
	vpor	xmm2, xmm3, xmm2
	vpaddd	xmm2, xmm5, xmm2
	vpaddd	xmm1, xmm2, xmm1
	vpaddd	xmm0, xmm1, xmm0
	vpaddd	xmm7, xmm0, xmmword ptr %[WORDS] + ( 7 * 0x10 )
	vpaddd	xmm11, xmm1, xmmword ptr %[WORDS] + ( 6 * 0x10 )
	vpsrld	xmm0, xmm7, 2
	vpslld	xmm1, xmm7, 30
	vpor	xmm0, xmm1, xmm0
	vpsrld	xmm1, xmm7, 13
	vpslld	xmm2, xmm7, 19
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm7, 22
	vpslld	xmm3, xmm7, 10
	vpor	xmm2, xmm3, xmm2
	vpxor	xmm1, xmm1, xmm2
	vpxor	xmm0, xmm1, xmm0
	vpand	xmm1, xmm8, xmm7
	vpxor	xmm2, xmm8, xmm7
	vbroadcastss	xmm13, dword ptr %[WORDS] + ( 35 * 0x10 ) # xmm13 = [1779033703,1779033703,1779033703,1779033703]
	vpand	xmm2, xmm13, xmm2
	vpxor	xmm1, xmm2, xmm1
	vpaddd	xmm0, xmm1, xmm0
	vpsrld	xmm1, xmm11, 6
	vpslld	xmm2, xmm11, 26
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm11, 11
	vpslld	xmm3, xmm11, 21
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm11, 25
	vpslld	xmm6, xmm11, 7
	vpor	xmm3, xmm6, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpxor	xmm1, xmm2, xmm1
	vpand	xmm2, xmm11, xmm12
	vpandn	xmm3, xmm11, xmm9
	vpor	xmm2, xmm2, xmm3
	vpaddd	xmm1, xmm1, xmm2
	vpaddd	xmm6, xmm1, xmmword ptr %[WORDS] + ( 9 * 0x10 )
	vpaddd	xmm0, xmm0, xmm1
	vpaddd	xmm10, xmm0, xmmword ptr %[WORDS] + ( 10 * 0x10 )
	mov	edx, 3
	vmovdqa	xmm1, xmm13
	vmovdqa	xmm0, xmm9
	.p2align	4, 0x90
%=2: # .LBB0_2:                                #   Parent Loop BB0_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	vmovdqa	xmm2, xmm0
	vmovdqa	xmm0, xmm12
	vmovdqa	xmm12, xmm11
	vmovdqa	xmm11, xmm6
	vpsrld	xmm3, xmm6, 6
	vpslld	xmm9, xmm6, 26
	vpor	xmm3, xmm9, xmm3
	vpsrld	xmm9, xmm6, 11
	vpslld	xmm13, xmm6, 21
	vpor	xmm9, xmm13, xmm9
	vpsrld	xmm13, xmm6, 25
	vpslld	xmm6, xmm6, 7
	vpor	xmm6, xmm13, xmm6
	vpxor	xmm6, xmm9, xmm6
	vpxor	xmm3, xmm6, xmm3
	vpand	xmm6, xmm11, xmm12
	vpandn	xmm9, xmm11, xmm0
	vpor	xmm6, xmm9, xmm6
	vpaddd	xmm2, xmm2, xmm6
	vpaddd	xmm2, xmm3, xmm2
	vbroadcastss	xmm3, dword ptr [rcx + 4*rdx]
	vpaddd	xmm2, xmm2, xmm3
	vpaddd	xmm6, xmm2, xmm1
	vmovdqa	xmm1, xmm8
	vmovdqa	xmm8, xmm7
	vmovdqa	xmm7, xmm10
	vpsrld	xmm3, xmm10, 2
	vpslld	xmm9, xmm10, 30
	vpor	xmm3, xmm9, xmm3
	vpsrld	xmm9, xmm10, 13
	vpslld	xmm10, xmm10, 19
	vpor	xmm9, xmm10, xmm9
	vpsrld	xmm10, xmm7, 22
	vpslld	xmm13, xmm7, 10
	vpor	xmm10, xmm13, xmm10
	vpxor	xmm9, xmm9, xmm10
	vpxor	xmm3, xmm9, xmm3
	vpand	xmm9, xmm8, xmm7
	vpxor	xmm10, xmm8, xmm7
	vpand	xmm10, xmm10, xmm1
	vpxor	xmm9, xmm10, xmm9
	vpaddd	xmm3, xmm9, xmm3
	vpaddd	xmm10, xmm3, xmm2
	inc	rdx
	cmp	rdx, 15
	jne %=2b #	.LBB0_2

	vpsrld	xmm2, xmm10, 2
	vpslld	xmm3, xmm10, 30
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm10, 13
	vpslld	xmm9, xmm10, 19
	vpor	xmm3, xmm9, xmm3
	vpsrld	xmm9, xmm10, 22
	vpslld	xmm13, xmm10, 10
	vpor	xmm9, xmm13, xmm9
	vpxor	xmm3, xmm9, xmm3
	vpxor	xmm2, xmm3, xmm2
	vpand	xmm3, xmm10, xmm7
	vpxor	xmm9, xmm10, xmm7
	vpand	xmm9, xmm9, xmm8
	vpxor	xmm3, xmm9, xmm3
	vpaddd	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm6, 6
	vpslld	xmm9, xmm6, 26
	vpor	xmm3, xmm9, xmm3
	vpsrld	xmm9, xmm6, 11
	vpslld	xmm13, xmm6, 21
	vpor	xmm9, xmm13, xmm9
	vpsrld	xmm13, xmm6, 25
	vpslld	xmm14, xmm6, 7
	vpor	xmm13, xmm14, xmm13
	vpxor	xmm9, xmm9, xmm13
	vpxor	xmm3, xmm9, xmm3
	vpand	xmm9, xmm11, xmm6
	vpandn	xmm13, xmm6, xmm12
	vpor	xmm9, xmm13, xmm9
	vpaddd	xmm0, xmm9, xmm0
	vpaddd	xmm0, xmm0, xmm3
	vpaddd	xmm0, xmm0, xmmword ptr %[WORDS] + ( 11 * 0x10 )
	vpaddd	xmm9, xmm0, xmm1
	vpaddd	xmm0, xmm2, xmm0
	vpsrld	xmm1, xmm5, 18
	vpslld	xmm2, xmm5, 14
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm5, 3
	vpxor	xmm1, xmm1, xmm2
	vpsrld	xmm2, xmm5, 7
	vpslld	xmm3, xmm5, 25
	vpor	xmm2, xmm3, xmm2
	vpxor	xmm1, xmm1, xmm2
	vpaddd	xmm4, xmm1, xmm4
	vpsrld	xmm1, xmm0, 2
	vpslld	xmm2, xmm0, 30
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm0, 13
	vpslld	xmm3, xmm0, 19
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm0, 22
	vpslld	xmm13, xmm0, 10
	vpor	xmm3, xmm13, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpxor	xmm1, xmm2, xmm1
	vpand	xmm2, xmm10, xmm0
	vpxor	xmm3, xmm10, xmm0
	vpand	xmm3, xmm3, xmm7
	vpxor	xmm2, xmm3, xmm2
	vpaddd	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm9, 6
	vpslld	xmm3, xmm9, 26
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm9, 11
	vpslld	xmm13, xmm9, 21
	vpor	xmm3, xmm13, xmm3
	vpsrld	xmm13, xmm9, 25
	vpslld	xmm14, xmm9, 7
	vpor	xmm13, xmm14, xmm13
	vpxor	xmm3, xmm13, xmm3
	vpxor	xmm2, xmm3, xmm2
	vpand	xmm3, xmm9, xmm6
	vpandn	xmm13, xmm9, xmm11
	vpor	xmm3, xmm13, xmm3
	vpaddd	xmm12, xmm12, xmm4
	vpaddd	xmm3, xmm12, xmm3
	vpaddd	xmm2, xmm3, xmm2
	vpaddd	xmm2, xmm2, xmmword ptr %[WORDS] + ( 12 * 0x10 )
	vpaddd	xmm13, xmm8, xmm2
	vpaddd	xmm1, xmm1, xmm2
	vpaddd	xmm15, xmm5, xmmword ptr %[WORDS] + ( 13 * 0x10 )
	vpsrld	xmm2, xmm1, 2
	vpslld	xmm3, xmm1, 30
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm1, 13
	vpslld	xmm8, xmm1, 19
	vpor	xmm3, xmm8, xmm3
	vpsrld	xmm8, xmm1, 22
	vpslld	xmm12, xmm1, 10
	vpor	xmm8, xmm12, xmm8
	vpxor	xmm3, xmm8, xmm3
	vpxor	xmm2, xmm3, xmm2
	vpand	xmm3, xmm1, xmm0
	vpxor	xmm8, xmm1, xmm0
	vpand	xmm8, xmm8, xmm10
	vpxor	xmm3, xmm8, xmm3
	vpaddd	xmm2, xmm2, xmm3
	vpsrld	xmm3, xmm13, 6
	vpslld	xmm8, xmm13, 26
	vpor	xmm3, xmm8, xmm3
	vpsrld	xmm8, xmm13, 11
	vpslld	xmm12, xmm13, 21
	vpor	xmm8, xmm12, xmm8
	vpsrld	xmm12, xmm13, 25
	vpslld	xmm14, xmm13, 7
	vpor	xmm12, xmm14, xmm12
	vpxor	xmm8, xmm8, xmm12
	vpxor	xmm3, xmm8, xmm3
	vpand	xmm8, xmm13, xmm9
	vpandn	xmm12, xmm13, xmm6
	vpor	xmm8, xmm8, xmm12
	vpaddd	xmm5, xmm11, xmm5
	vpaddd	xmm5, xmm8, xmm5
	vpaddd	xmm3, xmm5, xmm3
	vpaddd	xmm5, xmm3, xmmword ptr %[WORDS] + ( 14 * 0x10 )
	vpaddd	xmm3, xmm5, xmm7
	vpaddd	xmm2, xmm2, xmm5
	vpsrld	xmm5, xmm4, 19
	vpslld	xmm7, xmm4, 13
	vpor	xmm5, xmm7, xmm5
	vpsrld	xmm7, xmm4, 10
	vpxor	xmm5, xmm5, xmm7
	vpsrld	xmm7, xmm4, 17
	vpslld	xmm8, xmm4, 15
	vpor	xmm7, xmm8, xmm7
	vpxor	xmm5, xmm5, xmm7
	vpsrld	xmm7, xmm2, 2
	vpslld	xmm8, xmm2, 30
	vpor	xmm7, xmm8, xmm7
	vpsrld	xmm8, xmm2, 13
	vpslld	xmm11, xmm2, 19
	vpor	xmm8, xmm11, xmm8
	vpsrld	xmm11, xmm2, 22
	vpslld	xmm12, xmm2, 10
	vpor	xmm11, xmm12, xmm11
	vpxor	xmm8, xmm8, xmm11
	vpxor	xmm7, xmm8, xmm7
	vpand	xmm8, xmm2, xmm1
	vpxor	xmm11, xmm2, xmm1
	vpand	xmm11, xmm11, xmm0
	vpxor	xmm8, xmm11, xmm8
	vpaddd	xmm7, xmm8, xmm7
	vpsrld	xmm8, xmm3, 6
	vpslld	xmm11, xmm3, 26
	vpor	xmm8, xmm11, xmm8
	vpsrld	xmm11, xmm3, 11
	vpslld	xmm12, xmm3, 21
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm3, 25
	vpslld	xmm14, xmm3, 7
	vpor	xmm12, xmm14, xmm12
	vpxor	xmm11, xmm11, xmm12
	vpxor	xmm8, xmm11, xmm8
	vpand	xmm11, xmm13, xmm3
	vpandn	xmm12, xmm3, xmm9
	vpor	xmm11, xmm11, xmm12
	vpaddd	xmm6, xmm5, xmm6
	vpaddd	xmm6, xmm11, xmm6
	vpaddd	xmm6, xmm8, xmm6
	vpaddd	xmm6, xmm6, xmmword ptr %[WORDS] + ( 16 * 0x10 )
	vpaddd	xmm12, xmm10, xmm6
	vpaddd	xmm11, xmm7, xmm6
	vmovdqa	xmmword ptr [rsp + 16], xmm15   # 16-byte Spill
	vpsrld	xmm6, xmm15, 19
	vpslld	xmm7, xmm15, 13
	vpor	xmm6, xmm7, xmm6
	vpsrld	xmm7, xmm15, 10
	vpxor	xmm6, xmm6, xmm7
	vpsrld	xmm7, xmm15, 17
	vpslld	xmm8, xmm15, 15
	vpor	xmm7, xmm8, xmm7
	vpxor	xmm6, xmm6, xmm7
	vmovdqa	xmmword ptr [rsp + 32], xmm6    # 16-byte Spill
	vpsrld	xmm6, xmm11, 2
	vpslld	xmm8, xmm11, 30
	vpor	xmm6, xmm8, xmm6
	vpsrld	xmm8, xmm11, 13
	vpslld	xmm10, xmm11, 19
	vpor	xmm8, xmm10, xmm8
	vpsrld	xmm10, xmm11, 22
	vpslld	xmm14, xmm11, 10
	vpor	xmm10, xmm14, xmm10
	vpxor	xmm8, xmm8, xmm10
	vpxor	xmm6, xmm8, xmm6
	vpand	xmm8, xmm11, xmm2
	vpxor	xmm10, xmm11, xmm2
	vpand	xmm10, xmm10, xmm1
	vpxor	xmm8, xmm10, xmm8
	vpaddd	xmm6, xmm8, xmm6
	vpsrld	xmm8, xmm12, 6
	vpslld	xmm10, xmm12, 26
	vpor	xmm8, xmm10, xmm8
	vpsrld	xmm10, xmm12, 11
	vpslld	xmm14, xmm12, 21
	vpor	xmm14, xmm14, xmm10
	vpsrld	xmm10, xmm12, 25
	vpslld	xmm15, xmm12, 7
	vpor	xmm15, xmm15, xmm10
	vpxor	xmm7, xmm5, xmmword ptr %[WORDS] + ( 15 * 0x10 )
	vpxor	xmm5, xmm14, xmm15
	vpxor	xmm5, xmm8, xmm5
	vpand	xmm8, xmm12, xmm3
	vpandn	xmm14, xmm12, xmm13
	vpor	xmm8, xmm8, xmm14
	vmovdqa	xmm10, xmmword ptr [rsp + 32]   # 16-byte Reload
	vpaddd	xmm9, xmm10, xmm9
	vpaddd	xmm8, xmm9, xmm8
	vpaddd	xmm5, xmm8, xmm5
	vpaddd	xmm5, xmm5, xmmword ptr %[WORDS] + ( 17 * 0x10 )
	vpaddd	xmm14, xmm5, xmm0
	vpaddd	xmm0, xmm6, xmm5
	vmovdqa	xmmword ptr [rsp + 224], xmm7   # 16-byte Spill
	vpsrld	xmm5, xmm7, 19
	vpslld	xmm6, xmm7, 13
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm7, 10
	vpxor	xmm5, xmm5, xmm6
	vpsrld	xmm6, xmm7, 17
	vpslld	xmm8, xmm7, 15
	vpor	xmm6, xmm8, xmm6
	vpxor	xmm7, xmm5, xmm6
	vpsrld	xmm5, xmm0, 2
	vpslld	xmm6, xmm0, 30
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm0, 13
	vpslld	xmm9, xmm0, 19
	vpor	xmm6, xmm9, xmm6
	vpsrld	xmm9, xmm0, 22
	vpslld	xmm15, xmm0, 10
	vpor	xmm9, xmm15, xmm9
	vpxor	xmm6, xmm9, xmm6
	vpxor	xmm5, xmm6, xmm5
	vpand	xmm6, xmm11, xmm0
	vpxor	xmm9, xmm11, xmm0
	vpand	xmm9, xmm9, xmm2
	vpxor	xmm6, xmm9, xmm6
	vpaddd	xmm5, xmm5, xmm6
	vpsrld	xmm6, xmm14, 6
	vpslld	xmm9, xmm14, 26
	vpor	xmm6, xmm9, xmm6
	vpsrld	xmm9, xmm14, 11
	vpslld	xmm15, xmm14, 21
	vpor	xmm9, xmm15, xmm9
	vpsrld	xmm15, xmm14, 25
	vpslld	xmm8, xmm14, 7
	vpor	xmm8, xmm8, xmm15
	vpxor	xmm8, xmm9, xmm8
	vpxor	xmm6, xmm8, xmm6
	vpand	xmm8, xmm14, xmm12
	vpandn	xmm9, xmm14, xmm3
	vpor	xmm8, xmm8, xmm9
	vpaddd	xmm9, xmm13, xmm7
	vpaddd	xmm8, xmm9, xmm8
	vpaddd	xmm6, xmm8, xmm6
	vpaddd	xmm6, xmm6, xmmword ptr %[WORDS] + ( 18 * 0x10 )
	vpaddd	xmm9, xmm6, xmm1
	vpaddd	xmm1, xmm5, xmm6
	vmovdqa	xmm8, xmm10
	vpsrld	xmm5, xmm10, 19
	vpslld	xmm6, xmm10, 13
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm10, 10
	vpxor	xmm5, xmm5, xmm6
	vpsrld	xmm6, xmm10, 17
	vpslld	xmm8, xmm10, 15
	vpor	xmm6, xmm8, xmm6
	vpxor	xmm10, xmm5, xmm6
	vpsrld	xmm5, xmm1, 2
	vpslld	xmm6, xmm1, 30
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm1, 13
	vpslld	xmm8, xmm1, 19
	vpor	xmm6, xmm8, xmm6
	vpsrld	xmm8, xmm1, 22
	vpslld	xmm13, xmm1, 10
	vpor	xmm8, xmm13, xmm8
	vpxor	xmm6, xmm8, xmm6
	vpxor	xmm5, xmm6, xmm5
	vpxor	xmm6, xmm1, xmm0
	vpand	xmm6, xmm11, xmm6
	vpand	xmm8, xmm1, xmm0
	vpxor	xmm6, xmm8, xmm6
	vpaddd	xmm5, xmm5, xmm6
	vpsrld	xmm6, xmm9, 6
	vpslld	xmm8, xmm9, 26
	vpor	xmm6, xmm8, xmm6
	vpsrld	xmm8, xmm9, 11
	vpslld	xmm13, xmm9, 21
	vpor	xmm8, xmm13, xmm8
	vpsrld	xmm13, xmm9, 25
	vpslld	xmm15, xmm9, 7
	vpor	xmm13, xmm15, xmm13
	vpxor	xmm8, xmm8, xmm13
	vpxor	xmm6, xmm8, xmm6
	vpand	xmm8, xmm9, xmm14
	vpandn	xmm13, xmm9, xmm12
	vpor	xmm8, xmm8, xmm13
	vpaddd	xmm3, xmm10, xmm3
	vpaddd	xmm3, xmm8, xmm3
	vpaddd	xmm3, xmm3, xmm6
	vpaddd	xmm6, xmm3, xmmword ptr %[WORDS] + ( 19 * 0x10 )
	vpaddd	xmm3, xmm6, xmm2
	vpaddd	xmm2, xmm5, xmm6
	vmovdqa	xmmword ptr [rsp + 240], xmm7   # 16-byte Spill
	vpsrld	xmm5, xmm7, 19
	vpslld	xmm6, xmm7, 13
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm7, 10
	vpxor	xmm5, xmm5, xmm6
	vpsrld	xmm6, xmm7, 17
	vpslld	xmm8, xmm7, 15
	vpor	xmm6, xmm8, xmm6
	vpxor	xmm6, xmm5, xmm6
	vmovdqa	xmmword ptr [rsp], xmm6         # 16-byte Spill
	vpsrld	xmm5, xmm2, 2
	vpslld	xmm8, xmm2, 30
	vpor	xmm5, xmm8, xmm5
	vpsrld	xmm8, xmm2, 13
	vpslld	xmm13, xmm2, 19
	vpor	xmm8, xmm13, xmm8
	vpsrld	xmm13, xmm2, 22
	vpslld	xmm15, xmm2, 10
	vpor	xmm13, xmm15, xmm13
	vpxor	xmm8, xmm8, xmm13
	vpxor	xmm5, xmm8, xmm5
	vpxor	xmm8, xmm2, xmm1
	vpand	xmm8, xmm8, xmm0
	vpand	xmm13, xmm2, xmm1
	vpxor	xmm8, xmm8, xmm13
	vpaddd	xmm5, xmm8, xmm5
	vpsrld	xmm8, xmm3, 6
	vpslld	xmm13, xmm3, 26
	vpor	xmm8, xmm13, xmm8
	vpsrld	xmm13, xmm3, 11
	vpslld	xmm15, xmm3, 21
	vpor	xmm13, xmm15, xmm13
	vpsrld	xmm15, xmm3, 25
	vpslld	xmm7, xmm3, 7
	vpor	xmm7, xmm15, xmm7
	vpxor	xmm7, xmm13, xmm7
	vpxor	xmm7, xmm8, xmm7
	vpand	xmm8, xmm9, xmm3
	vpandn	xmm13, xmm3, xmm14
	vpor	xmm8, xmm8, xmm13
	vpaddd	xmm12, xmm12, xmm6
	vpaddd	xmm8, xmm12, xmm8
	vpaddd	xmm7, xmm8, xmm7
	vpaddd	xmm7, xmm7, xmmword ptr %[WORDS] + ( 21 * 0x10 )
	vpaddd	xmm13, xmm11, xmm7
	vpaddd	xmm15, xmm5, xmm7
	vmovdqa	xmm6, xmm10
	vmovdqa	xmmword ptr [rsp + 64], xmm10   # 16-byte Spill
	vpsrld	xmm5, xmm10, 19
	vpslld	xmm7, xmm10, 13
	vpor	xmm5, xmm7, xmm5
	vpsrld	xmm7, xmm10, 10
	vpxor	xmm5, xmm5, xmm7
	vpsrld	xmm7, xmm10, 17
	vpslld	xmm8, xmm10, 15
	vpor	xmm7, xmm8, xmm7
	vpxor	xmm5, xmm5, xmm7
	vpsrld	xmm7, xmm15, 2
	vpslld	xmm8, xmm15, 30
	vpor	xmm7, xmm8, xmm7
	vpsrld	xmm8, xmm15, 13
	vpslld	xmm11, xmm15, 19
	vpor	xmm8, xmm11, xmm8
	vpsrld	xmm11, xmm15, 22
	vpslld	xmm12, xmm15, 10
	vpor	xmm11, xmm12, xmm11
	vpxor	xmm8, xmm8, xmm11
	vpxor	xmm7, xmm8, xmm7
	vpxor	xmm8, xmm15, xmm2
	vpand	xmm8, xmm8, xmm1
	vpand	xmm11, xmm15, xmm2
	vpxor	xmm8, xmm8, xmm11
	vpaddd	xmm7, xmm8, xmm7
	vpsrld	xmm8, xmm13, 6
	vpslld	xmm11, xmm13, 26
	vpor	xmm8, xmm11, xmm8
	vpsrld	xmm11, xmm13, 11
	vpslld	xmm12, xmm13, 21
	vpor	xmm11, xmm12, xmm11
	vpsrld	xmm12, xmm13, 25
	vpslld	xmm6, xmm13, 7
	vpor	xmm6, xmm12, xmm6
	vpxor	xmm6, xmm11, xmm6
	vpxor	xmm6, xmm8, xmm6
	vpand	xmm8, xmm13, xmm3
	vpandn	xmm11, xmm13, xmm9
	vpor	xmm8, xmm8, xmm11
	vpaddd	xmm10, xmm5, xmm4
	vpaddd	xmm11, xmm10, xmm14
	vpaddd	xmm8, xmm11, xmm8
	vpaddd	xmm6, xmm8, xmm6
	vpaddd	xmm6, xmm6, xmmword ptr %[WORDS] + ( 22 * 0x10 )
	vpaddd	xmm12, xmm6, xmm0
	vpaddd	xmm11, xmm7, xmm6
	vbroadcastss	xmm0, dword ptr %[WORDS] + ( 36 * 0x10 ) # xmm0 = [64,64,64,64]
	vpaddd	xmm5, xmm0, xmmword ptr [rsp]   # 16-byte Folded Reload
	vmovdqa	xmmword ptr [rsp], xmm5         # 16-byte Spill
	vpsrld	xmm0, xmm5, 19
	vpslld	xmm6, xmm5, 13
	vpor	xmm0, xmm6, xmm0
	vpsrld	xmm6, xmm5, 10
	vpxor	xmm0, xmm0, xmm6
	vpsrld	xmm6, xmm5, 17
	vpslld	xmm7, xmm5, 15
	vpor	xmm6, xmm7, xmm6
	vpxor	xmm0, xmm0, xmm6
	vmovdqa	xmmword ptr [rsp + 256], xmm4
	vmovdqa	xmm5, xmmword ptr [rsp + 16]    # 16-byte Reload
	vmovdqa	xmmword ptr [rsp + 272], xmm5
	vpaddd	xmm14, xmm0, xmm5
	vpsrld	xmm0, xmm11, 2
	vpslld	xmm6, xmm11, 30
	vpor	xmm0, xmm6, xmm0
	vpsrld	xmm6, xmm11, 13
	vpslld	xmm7, xmm11, 19
	vpor	xmm6, xmm7, xmm6
	vpsrld	xmm7, xmm11, 22
	vpslld	xmm8, xmm11, 10
	vpor	xmm7, xmm8, xmm7
	vpxor	xmm6, xmm6, xmm7
	vpxor	xmm0, xmm6, xmm0
	vpxor	xmm6, xmm11, xmm15
	vpand	xmm6, xmm6, xmm2
	vpand	xmm7, xmm11, xmm15
	vpxor	xmm6, xmm6, xmm7
	vpaddd	xmm0, xmm0, xmm6
	vpsrld	xmm6, xmm12, 6
	vpslld	xmm7, xmm12, 26
	vpor	xmm6, xmm7, xmm6
	vpsrld	xmm7, xmm12, 11
	vpslld	xmm8, xmm12, 21
	vpor	xmm7, xmm8, xmm7
	vpsrld	xmm8, xmm12, 25
	vpslld	xmm5, xmm12, 7
	vpor	xmm5, xmm8, xmm5
	vpxor	xmm5, xmm7, xmm5
	vpxor	xmm5, xmm5, xmm6
	vpand	xmm6, xmm12, xmm13
	vpandn	xmm7, xmm12, xmm3
	vpor	xmm6, xmm6, xmm7
	vpaddd	xmm7, xmm14, xmm9
	vpaddd	xmm6, xmm7, xmm6
	vpaddd	xmm5, xmm6, xmm5
	vpaddd	xmm5, xmm5, xmmword ptr %[WORDS] + ( 23 * 0x10 )
	vpaddd	xmm1, xmm5, xmm1
	vpaddd	xmm9, xmm0, xmm5
	vmovdqa	xmmword ptr [rsp + 48], xmm10   # 16-byte Spill
	vpsrld	xmm0, xmm10, 19
	vpslld	xmm5, xmm10, 13
	vpor	xmm0, xmm5, xmm0
	vpsrld	xmm5, xmm10, 10
	vpxor	xmm0, xmm0, xmm5
	vpsrld	xmm5, xmm10, 17
	vpslld	xmm6, xmm10, 15
	vpor	xmm5, xmm6, xmm5
	vpxor	xmm0, xmm0, xmm5
	vmovdqa	xmm5, xmmword ptr [rsp + 224]   # 16-byte Reload
	vmovdqa	xmmword ptr [rsp + 288], xmm5
	vpaddd	xmm10, xmm0, xmm5
	vpsrld	xmm0, xmm9, 2
	vpslld	xmm5, xmm9, 30
	vpor	xmm0, xmm5, xmm0
	vpsrld	xmm5, xmm9, 13
	vpslld	xmm6, xmm9, 19
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm9, 22
	vpslld	xmm7, xmm9, 10
	vpor	xmm6, xmm7, xmm6
	vpxor	xmm5, xmm5, xmm6
	vpxor	xmm0, xmm5, xmm0
	vpxor	xmm5, xmm9, xmm11
	vpand	xmm5, xmm15, xmm5
	vpand	xmm6, xmm9, xmm11
	vpxor	xmm5, xmm5, xmm6
	vpaddd	xmm0, xmm0, xmm5
	vpsrld	xmm5, xmm1, 6
	vpslld	xmm6, xmm1, 26
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm1, 11
	vpslld	xmm7, xmm1, 21
	vpor	xmm6, xmm7, xmm6
	vpsrld	xmm7, xmm1, 25
	vpslld	xmm8, xmm1, 7
	vpor	xmm7, xmm8, xmm7
	vpxor	xmm6, xmm6, xmm7
	vpxor	xmm5, xmm6, xmm5
	vpand	xmm6, xmm12, xmm1
	vpandn	xmm7, xmm1, xmm13
	vpor	xmm6, xmm6, xmm7
	vpaddd	xmm3, xmm10, xmm3
	vpaddd	xmm3, xmm3, xmm6
	vpaddd	xmm3, xmm3, xmm5
	vpaddd	xmm3, xmm3, xmmword ptr %[WORDS] + ( 24 * 0x10 )
	vpaddd	xmm2, xmm3, xmm2
	vpaddd	xmm3, xmm0, xmm3
	vmovdqa	xmmword ptr [rsp + 16], xmm14   # 16-byte Spill
	vpsrld	xmm0, xmm14, 19
	vpslld	xmm5, xmm14, 13
	vpor	xmm0, xmm5, xmm0
	vpsrld	xmm5, xmm14, 10
	vpxor	xmm0, xmm0, xmm5
	vpsrld	xmm5, xmm14, 17
	vpslld	xmm6, xmm14, 15
	vpor	xmm5, xmm6, xmm5
	vpxor	xmm0, xmm0, xmm5
	vmovdqa	xmm5, xmmword ptr [rsp + 32]    # 16-byte Reload
	vmovdqa	xmmword ptr [rsp + 304], xmm5
	vpaddd	xmm0, xmm0, xmm5
	vpsrld	xmm5, xmm3, 2
	vpslld	xmm6, xmm3, 30
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm3, 13
	vpslld	xmm7, xmm3, 19
	vpor	xmm6, xmm7, xmm6
	vpsrld	xmm7, xmm3, 22
	vpslld	xmm8, xmm3, 10
	vpor	xmm7, xmm8, xmm7
	vpxor	xmm6, xmm6, xmm7
	vpxor	xmm5, xmm6, xmm5
	vpxor	xmm6, xmm9, xmm3
	vpand	xmm6, xmm11, xmm6
	vpand	xmm7, xmm9, xmm3
	vpxor	xmm6, xmm6, xmm7
	vpaddd	xmm5, xmm5, xmm6
	vpsrld	xmm6, xmm2, 6
	vpslld	xmm7, xmm2, 26
	vpor	xmm6, xmm7, xmm6
	vpsrld	xmm7, xmm2, 11
	vpslld	xmm8, xmm2, 21
	vpor	xmm7, xmm8, xmm7
	vpsrld	xmm8, xmm2, 25
	vpslld	xmm14, xmm2, 7
	vpor	xmm8, xmm14, xmm8
	vpxor	xmm7, xmm8, xmm7
	vpxor	xmm6, xmm7, xmm6
	vpand	xmm7, xmm2, xmm1
	vpandn	xmm8, xmm2, xmm12
	vpor	xmm7, xmm8, xmm7
	vpaddd	xmm8, xmm13, xmm0
	vpaddd	xmm7, xmm8, xmm7
	vpaddd	xmm6, xmm7, xmm6
	vpaddd	xmm6, xmm6, xmmword ptr %[WORDS] + ( 25 * 0x10 )
	vpaddd	xmm13, xmm15, xmm6
	vpaddd	xmm7, xmm5, xmm6
	vpsrld	xmm5, xmm10, 19
	vpslld	xmm6, xmm10, 13
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm10, 10
	vpxor	xmm5, xmm5, xmm6
	vmovdqa	xmm14, xmmword ptr [rsp + 240]  # 16-byte Reload
	vmovdqa	xmmword ptr [rsp + 320], xmm14
	vmovaps	xmm6, xmmword ptr [rsp + 64]    # 16-byte Reload
	vmovaps	xmmword ptr [rsp + 336], xmm6
	vmovaps	xmm6, xmmword ptr [rsp]         # 16-byte Reload
	vmovaps	xmmword ptr [rsp + 352], xmm6
	vmovaps	xmm6, xmmword ptr [rsp + 48]    # 16-byte Reload
	vmovaps	xmmword ptr [rsp + 368], xmm6
	vmovaps	xmm6, xmmword ptr [rsp + 16]    # 16-byte Reload
	vmovaps	xmmword ptr [rsp + 384], xmm6
	vmovdqa	xmmword ptr [rsp + 400], xmm10
	vpsrld	xmm6, xmm10, 17
	vpslld	xmm8, xmm10, 15
	vpor	xmm6, xmm8, xmm6
	vpxor	xmm5, xmm5, xmm6
	vpaddd	xmm6, xmm14, xmm5
	vpsrld	xmm5, xmm7, 2
	vpslld	xmm8, xmm7, 30
	vpor	xmm5, xmm8, xmm5
	vpsrld	xmm8, xmm7, 13
	vpslld	xmm10, xmm7, 19
	vpor	xmm8, xmm10, xmm8
	vpsrld	xmm10, xmm7, 22
	vpslld	xmm14, xmm7, 10
	vpor	xmm10, xmm14, xmm10
	vpxor	xmm8, xmm8, xmm10
	vpxor	xmm5, xmm8, xmm5
	vpxor	xmm8, xmm7, xmm3
	vpand	xmm8, xmm8, xmm9
	vpand	xmm10, xmm7, xmm3
	vpxor	xmm8, xmm8, xmm10
	vpaddd	xmm5, xmm8, xmm5
	vpsrld	xmm8, xmm13, 6
	vpslld	xmm10, xmm13, 26
	vpor	xmm8, xmm10, xmm8
	vpsrld	xmm10, xmm13, 11
	vpslld	xmm14, xmm13, 21
	vpor	xmm10, xmm14, xmm10
	vpsrld	xmm14, xmm13, 25
	vpslld	xmm15, xmm13, 7
	vpor	xmm14, xmm15, xmm14
	vpxor	xmm10, xmm10, xmm14
	vpxor	xmm8, xmm10, xmm8
	vpand	xmm10, xmm13, xmm2
	vpandn	xmm14, xmm13, xmm1
	vpor	xmm10, xmm10, xmm14
	vpaddd	xmm12, xmm12, xmm6
	vpaddd	xmm10, xmm12, xmm10
	vpaddd	xmm8, xmm10, xmm8
	vpaddd	xmm8, xmm8, xmmword ptr %[WORDS] + ( 26 * 0x10 )
	vpaddd	xmm12, xmm8, xmm11
	vpaddd	xmm15, xmm8, xmm5
	vpsrld	xmm5, xmm0, 19
	vpslld	xmm8, xmm0, 13
	vpor	xmm5, xmm8, xmm5
	vpsrld	xmm8, xmm0, 10
	vpxor	xmm5, xmm8, xmm5
	vmovdqa	xmmword ptr [rsp + 416], xmm0
	vpsrld	xmm8, xmm0, 17
	vpslld	xmm0, xmm0, 15
	vpor	xmm0, xmm8, xmm0
	vpxor	xmm0, xmm5, xmm0
	vpaddd	xmm0, xmm0, xmmword ptr [rsp + 64] # 16-byte Folded Reload
	vpsrld	xmm5, xmm15, 2
	vpslld	xmm8, xmm15, 30
	vpor	xmm5, xmm8, xmm5
	vpsrld	xmm8, xmm15, 13
	vpslld	xmm10, xmm15, 19
	vpor	xmm8, xmm10, xmm8
	vpsrld	xmm10, xmm15, 22
	vpslld	xmm11, xmm15, 10
	vpor	xmm10, xmm11, xmm10
	vpxor	xmm8, xmm8, xmm10
	vpxor	xmm5, xmm8, xmm5
	vpxor	xmm8, xmm15, xmm7
	vpand	xmm8, xmm8, xmm3
	vpand	xmm10, xmm15, xmm7
	vpxor	xmm8, xmm8, xmm10
	vpaddd	xmm5, xmm8, xmm5
	vpsrld	xmm8, xmm12, 6
	vpslld	xmm10, xmm12, 26
	vpor	xmm8, xmm10, xmm8
	vpsrld	xmm10, xmm12, 11
	vpslld	xmm11, xmm12, 21
	vpor	xmm10, xmm11, xmm10
	vpsrld	xmm11, xmm12, 25
	vpslld	xmm14, xmm12, 7
	vpor	xmm11, xmm14, xmm11
	vpxor	xmm10, xmm10, xmm11
	vpxor	xmm8, xmm10, xmm8
	vpand	xmm10, xmm12, xmm13
	vpandn	xmm11, xmm12, xmm2
	vpor	xmm10, xmm10, xmm11
	vpaddd	xmm1, xmm0, xmm1
	vpaddd	xmm1, xmm10, xmm1
	vmovdqa	xmmword ptr [rsp + 432], xmm6
	vpaddd	xmm1, xmm8, xmm1
	vpaddd	xmm1, xmm1, xmmword ptr %[WORDS] + ( 27 * 0x10 )
	vpaddd	xmm11, xmm9, xmm1
	vpaddd	xmm10, xmm5, xmm1
	vpsrld	xmm1, xmm6, 19
	vpslld	xmm5, xmm6, 13
	vpor	xmm1, xmm5, xmm1
	vpsrld	xmm5, xmm6, 10
	vpxor	xmm1, xmm1, xmm5
	vpsrld	xmm5, xmm6, 17
	vpslld	xmm6, xmm6, 15
	vpor	xmm5, xmm6, xmm5
	vpxor	xmm1, xmm1, xmm5
	vpaddd	xmm8, xmm1, xmmword ptr [rsp]   # 16-byte Folded Reload
	vpsrld	xmm1, xmm10, 2
	vpslld	xmm5, xmm10, 30
	vpor	xmm1, xmm5, xmm1
	vpsrld	xmm5, xmm10, 13
	vpslld	xmm6, xmm10, 19
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm10, 22
	vpslld	xmm9, xmm10, 10
	vpor	xmm6, xmm9, xmm6
	vpxor	xmm5, xmm5, xmm6
	vpxor	xmm1, xmm5, xmm1
	vpand	xmm5, xmm10, xmm15
	vpxor	xmm6, xmm10, xmm15
	vpand	xmm6, xmm6, xmm7
	vpxor	xmm5, xmm6, xmm5
	vpaddd	xmm1, xmm1, xmm5
	vpsrld	xmm5, xmm11, 6
	vpslld	xmm6, xmm11, 26
	vpor	xmm5, xmm6, xmm5
	vpsrld	xmm6, xmm11, 11
	vpslld	xmm9, xmm11, 21
	vpor	xmm6, xmm9, xmm6
	vpsrld	xmm9, xmm11, 25
	vpslld	xmm14, xmm11, 7
	vpor	xmm9, xmm14, xmm9
	vpxor	xmm6, xmm9, xmm6
	vpxor	xmm5, xmm6, xmm5
	vpand	xmm6, xmm11, xmm12
	vpandn	xmm9, xmm11, xmm13
	vpor	xmm6, xmm9, xmm6
	vmovdqa	xmmword ptr [rsp + 448], xmm0
	vpaddd	xmm2, xmm8, xmm2
	vpaddd	xmm2, xmm2, xmm6
	vpaddd	xmm2, xmm2, xmm5
	vpaddd	xmm2, xmm2, xmmword ptr %[WORDS] + ( 28 * 0x10 )
	vpaddd	xmm6, xmm2, xmm3
	vpaddd	xmm14, xmm1, xmm2
	vpsrld	xmm1, xmm0, 19
	vpslld	xmm2, xmm0, 13
	vpor	xmm1, xmm2, xmm1
	vpsrld	xmm2, xmm0, 10
	vpxor	xmm1, xmm1, xmm2
	vpsrld	xmm2, xmm0, 17
	vpslld	xmm0, xmm0, 15
	vpor	xmm0, xmm0, xmm2
	vpxor	xmm0, xmm1, xmm0
	vpaddd	xmm1, xmm0, xmmword ptr [rsp + 48] # 16-byte Folded Reload
	vpsrld	xmm0, xmm14, 2
	vpslld	xmm2, xmm14, 30
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm14, 13
	vpslld	xmm3, xmm14, 19
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm14, 22
	vpslld	xmm5, xmm14, 10
	vpor	xmm3, xmm5, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpxor	xmm0, xmm2, xmm0
	vpand	xmm2, xmm14, xmm10
	vpxor	xmm3, xmm14, xmm10
	vpand	xmm3, xmm15, xmm3
	vpxor	xmm2, xmm3, xmm2
	vpaddd	xmm0, xmm0, xmm2
	vpsrld	xmm2, xmm6, 6
	vpslld	xmm3, xmm6, 26
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm6, 11
	vpslld	xmm5, xmm6, 21
	vpor	xmm3, xmm5, xmm3
	vpsrld	xmm5, xmm6, 25
	vpslld	xmm9, xmm6, 7
	vpor	xmm5, xmm9, xmm5
	vmovdqa	xmmword ptr [rsp + 464], xmm8
	vpxor	xmm3, xmm3, xmm5
	vpxor	xmm2, xmm3, xmm2
	vpand	xmm3, xmm11, xmm6
	vpandn	xmm5, xmm6, xmm12
	vpor	xmm3, xmm3, xmm5
	vpaddd	xmm5, xmm13, xmm1
	vpaddd	xmm3, xmm5, xmm3
	vpaddd	xmm2, xmm3, xmm2
	vpaddd	xmm2, xmm2, xmmword ptr %[WORDS] + ( 30 * 0x10 )
	vpaddd	xmm5, xmm2, xmm7
	vpaddd	xmm7, xmm0, xmm2
	vpsrld	xmm0, xmm8, 19
	vpslld	xmm2, xmm8, 13
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm8, 10
	vpxor	xmm0, xmm0, xmm2
	vpsrld	xmm2, xmm8, 17
	vpslld	xmm3, xmm8, 15
	vpor	xmm2, xmm3, xmm2
	vpxor	xmm0, xmm0, xmm2
	vpsrld	xmm2, xmm4, 18
	vpslld	xmm3, xmm4, 14
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm4, 3
	vpxor	xmm2, xmm2, xmm3
	vpsrld	xmm3, xmm4, 7
	vpslld	xmm8, xmm4, 25
	vpor	xmm3, xmm8, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpaddd	xmm2, xmm2, xmmword ptr [rsp + 16] # 16-byte Folded Reload
	vpaddd	xmm2, xmm2, xmmword ptr %[WORDS] + ( 20 * 0x10 )
	vpaddd	xmm8, xmm2, xmm0
	vpsrld	xmm0, xmm7, 2
	vpslld	xmm2, xmm7, 30
	vpor	xmm0, xmm2, xmm0
	vpsrld	xmm2, xmm7, 13
	vpslld	xmm3, xmm7, 19
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm7, 22
	vpslld	xmm9, xmm7, 10
	vpor	xmm3, xmm9, xmm3
	vpxor	xmm2, xmm2, xmm3
	vpxor	xmm0, xmm2, xmm0
	vpand	xmm2, xmm14, xmm7
	vpxor	xmm3, xmm14, xmm7
	vpand	xmm3, xmm10, xmm3
	vpxor	xmm2, xmm3, xmm2
	vpaddd	xmm2, xmm0, xmm2
	vpsrld	xmm0, xmm5, 6
	vpslld	xmm3, xmm5, 26
	vpor	xmm0, xmm3, xmm0
	vpsrld	xmm3, xmm5, 11
	vpslld	xmm9, xmm5, 21
	vpor	xmm3, xmm9, xmm3
	vpsrld	xmm9, xmm5, 25
	vpslld	xmm13, xmm5, 7
	vpor	xmm9, xmm13, xmm9
	vpxor	xmm3, xmm9, xmm3
	vpxor	xmm0, xmm3, xmm0
	vpand	xmm3, xmm5, xmm6
	vpandn	xmm9, xmm5, xmm11
	vpor	xmm3, xmm9, xmm3
	vpaddd	xmm9, xmm8, xmm12
	vpaddd	xmm3, xmm9, xmm3
	vpaddd	xmm0, xmm3, xmm0
	vpaddd	xmm3, xmm0, xmmword ptr %[WORDS] + ( 31 * 0x10 )
	vpaddd	xmm0, xmm15, xmm3
	vpaddd	xmm12, xmm2, xmm3
	vpaddd	xmm2, xmm1, xmmword ptr %[WORDS] + ( 29 * 0x10 )
	vmovdqa	xmmword ptr [rsp + 480], xmm2
	vmovdqa	xmmword ptr [rsp + 496], xmm8
	mov	edx, 0
	.p2align	4, 0x90
%=4: # .LBB0_4:                                #   Parent Loop BB0_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	vmovdqa	xmm1, xmm11
	vmovdqa	xmm11, xmm6
	vmovdqa	xmm6, xmm5
	vmovdqa	xmm15, xmm8
	vmovdqa	xmm5, xmm0
	vpsrld	xmm3, xmm2, 19
	vpslld	xmm8, xmm2, 13
	vpor	xmm3, xmm8, xmm3
	vpsrld	xmm8, xmm2, 10
	vpxor	xmm3, xmm8, xmm3
	vpsrld	xmm8, xmm2, 17
	vpslld	xmm2, xmm2, 15
	vpor	xmm2, xmm8, xmm2
	vpxor	xmm2, xmm3, xmm2
	vpaddd	xmm2, xmm4, xmm2
	vmovdqa	xmm4, xmmword ptr [rsp + 4*rdx + 272]
	vpsrld	xmm3, xmm4, 18
	vpslld	xmm8, xmm4, 14
	vpor	xmm3, xmm8, xmm3
	vpsrld	xmm8, xmm4, 3
	vpxor	xmm3, xmm8, xmm3
	vpsrld	xmm8, xmm4, 7
	vpslld	xmm9, xmm4, 25
	vpor	xmm8, xmm9, xmm8
	vpxor	xmm3, xmm8, xmm3
	vpaddd	xmm2, xmm2, xmmword ptr [rsp + 4*rdx + 400]
	vpaddd	xmm8, xmm2, xmm3
	vpsrld	xmm2, xmm0, 6
	vpslld	xmm3, xmm0, 26
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm0, 11
	vpslld	xmm9, xmm0, 21
	vpor	xmm3, xmm9, xmm3
	vpsrld	xmm9, xmm0, 25
	vpslld	xmm0, xmm0, 7
	vpor	xmm0, xmm9, xmm0
	vpxor	xmm0, xmm3, xmm0
	vpxor	xmm0, xmm0, xmm2
	vpand	xmm2, xmm5, xmm6
	vpandn	xmm3, xmm5, xmm11
	vpor	xmm2, xmm3, xmm2
	vpaddd	xmm1, xmm1, xmm2
	vpaddd	xmm0, xmm0, xmm1
	vbroadcastss	xmm1, dword ptr [rdx + rcx + 128]
	vpaddd	xmm0, xmm0, xmm1
	vpaddd	xmm1, xmm8, xmm0
	vpaddd	xmm0, xmm10, xmm1
	vmovdqa	xmm10, xmm14
	vmovdqa	xmm14, xmm7
	vmovdqa	xmm7, xmm12
	vmovdqa	xmmword ptr [rsp + 4*rdx + 512], xmm8
	vpsrld	xmm2, xmm12, 2
	vpslld	xmm3, xmm12, 30
	vpor	xmm2, xmm3, xmm2
	vpsrld	xmm3, xmm12, 13
	vpslld	xmm9, xmm12, 19
	vpor	xmm3, xmm9, xmm3
	vpsrld	xmm9, xmm12, 22
	vpslld	xmm12, xmm12, 10
	vpor	xmm9, xmm12, xmm9
	vpxor	xmm3, xmm9, xmm3
	vpxor	xmm2, xmm3, xmm2
	vpand	xmm3, xmm14, xmm7
	vpxor	xmm9, xmm14, xmm7
	vpand	xmm9, xmm9, xmm10
	vpxor	xmm3, xmm9, xmm3
	vpaddd	xmm2, xmm2, xmm3
	vpaddd	xmm12, xmm2, xmm1
	add	rdx, 4
	vmovdqa	xmm2, xmm15
	cmp	rdx, 100
	jne	%=4b # .LBB0_4

	vpcmpeqd	xmm1, xmm12, xmmword ptr [rsp + 208] # 16-byte Folded Reload
	vptest	xmm1, xmm1
	je %=10b #	.LBB0_10

	mov	edx, 104
	vmovdqa	xmm1, xmmword ptr [rsp + 656]
	.p2align	4, 0x90
%=7: # .LBB0_7:                                #   Parent Loop BB0_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	vmovdqa	xmm2, xmm11
	vmovdqa	xmm11, xmm6
	vmovdqa	xmm6, xmm5
	vmovdqa	xmm5, xmm0
	vpsrld	xmm0, xmm15, 19
	vpslld	xmm3, xmm15, 13
	vpor	xmm0, xmm3, xmm0
	vpsrld	xmm3, xmm15, 10
	vpxor	xmm0, xmm0, xmm3
	vpsrld	xmm3, xmm15, 17
	vpslld	xmm4, xmm15, 15
	vpor	xmm3, xmm4, xmm3
	vpxor	xmm0, xmm0, xmm3
	vpaddd	xmm0, xmm1, xmm0
	vmovdqa	xmm1, xmmword ptr [rsp + 4*rdx + 256]
	vpsrld	xmm3, xmm1, 18
	vpslld	xmm4, xmm1, 14
	vpor	xmm3, xmm4, xmm3
	vpsrld	xmm4, xmm1, 3
	vpxor	xmm3, xmm3, xmm4
	vpsrld	xmm4, xmm1, 7
	vpslld	xmm9, xmm1, 25
	vpor	xmm4, xmm9, xmm4
	vpaddd	xmm0, xmm0, xmmword ptr [rsp + 4*rdx + 384]
	vpxor	xmm3, xmm3, xmm4
	vmovdqa	xmm15, xmm8
	vpaddd	xmm8, xmm0, xmm3
	vpsrld	xmm0, xmm5, 6
	vpslld	xmm3, xmm5, 26
	vpor	xmm0, xmm3, xmm0
	vpsrld	xmm3, xmm5, 11
	vpslld	xmm4, xmm5, 21
	vpor	xmm3, xmm4, xmm3
	vpsrld	xmm4, xmm5, 25
	vpslld	xmm9, xmm5, 7
	vpor	xmm4, xmm9, xmm4
	vpxor	xmm3, xmm3, xmm4
	vpxor	xmm0, xmm3, xmm0
	vpand	xmm3, xmm5, xmm6
	vpandn	xmm4, xmm5, xmm11
	vpor	xmm3, xmm4, xmm3
	vpaddd	xmm2, xmm3, xmm2
	vpaddd	xmm0, xmm2, xmm0
	vbroadcastss	xmm2, dword ptr [rdx + rcx + 124]
	vpaddd	xmm0, xmm0, xmm2
	vpaddd	xmm2, xmm8, xmm0
	vpaddd	xmm0, xmm10, xmm2
	vmovdqa	xmm10, xmm14
	vmovdqa	xmm14, xmm7
	vmovdqa	xmm7, xmm12
	vmovdqa	xmmword ptr [rsp + 4*rdx + 496], xmm8
	vpsrld	xmm3, xmm12, 2
	vpslld	xmm4, xmm12, 30
	vpor	xmm3, xmm4, xmm3
	vpsrld	xmm4, xmm12, 13
	vpslld	xmm9, xmm12, 19
	vpor	xmm4, xmm9, xmm4
	vpsrld	xmm9, xmm12, 22
	vpslld	xmm12, xmm12, 10
	vpor	xmm9, xmm12, xmm9
	vpxor	xmm4, xmm9, xmm4
	vpxor	xmm3, xmm4, xmm3
	vpand	xmm4, xmm14, xmm7
	vpxor	xmm9, xmm14, xmm7
	vpand	xmm9, xmm9, xmm10
	vpxor	xmm4, xmm9, xmm4
	vpaddd	xmm3, xmm3, xmm4
	vpaddd	xmm12, xmm3, xmm2
	add	rdx, 4
	cmp	rdx, 132
	jne	%=7b # .LBB0_7

	vpcmpeqd	xmm1, xmm12, xmmword ptr [rsp + 192] # 16-byte Folded Reload
	vpcmpeqd	xmm2, xmm7, xmmword ptr [rsp + 176] # 16-byte Folded Reload
	vpcmpeqd	xmm3, xmm14, xmmword ptr [rsp + 160] # 16-byte Folded Reload
	vpcmpeqd	xmm4, xmm10, xmmword ptr [rsp + 144] # 16-byte Folded Reload
	vpcmpeqd	xmm0, xmm0, xmmword ptr [rsp + 128] # 16-byte Folded Reload
	vpcmpeqd	xmm5, xmm5, xmmword ptr [rsp + 112] # 16-byte Folded Reload
	vpcmpeqd	xmm6, xmm6, xmmword ptr [rsp + 96] # 16-byte Folded Reload
	vpcmpeqd	xmm7, xmm11, xmmword ptr [rsp + 80] # 16-byte Folded Reload
	vpand	xmm6, xmm6, xmm7
	vpand	xmm5, xmm6, xmm5
	vpand	xmm4, xmm5, xmm4
	vpand	xmm3, xmm4, xmm3
	vpand	xmm2, xmm3, xmm2
	vpand	xmm0, xmm2, xmm0
	vpand	xmm0, xmm0, xmm1
	vpslld	xmm0, xmm0, 31
	vmovmskps	edx, xmm0
	test	edx, edx
	je %=10b #	.LBB0_10

	movzx	eax, dl
	rep		bsf	eax, eax
	shl	eax, 3
	lea	rcx, [rsp + 1024]
	or	rcx, rax
	mov	rax, qword ptr [rcx]
%=11: # .LBB0_11:
	mov	rcx, qword ptr fs:[40]
	cmp	rcx, qword ptr [rsp + 1112]
	jne	%=13f # .LBB0_13

	lea	rsp, [rbp - 16]
	pop	rbx
	pop	r14
	pop	rbp
	.cfi_def_cfa rsp, 8
	ret
%=13: # .LBB0_13:
	.cfi_def_cfa rbp, 16
	call	__stack_chk_fail@PLT
    )"""
        :
        : "rdi"(start),
          "rsi"(end),
          "rdx"(target),
          // constants
          [k] "m"(k),
          [WORDS] "m"(WORDS)
        : // TODO :: proper clobbers based off of final assembly
        "rax",
        "rcx",
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

    return 0;
}
