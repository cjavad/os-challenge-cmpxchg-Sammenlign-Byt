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

void sha256x4_asm(
	uint8_t hash[SHA256_DIGEST_LENGTH * 4],
	const uint8_t data[SHA256_INPUT_LENGTH * 4]
) {
	// cant clobber
	// rdi, rsp, rbp


	__asm__ volatile (
		R"""(
			// set stack pointer (rsp aligned to 16 bytes by default)
			// 
			sub rsp, 512 + 256

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
			

			// restore stack pointer
			add rsp, 512 + 256
		)"""
		// no output operands
		:
		// input operands (System V ABI), incase gcc decides to inline function
		: "rdi"(hash), "rsi"(data),
		// constants
		[byteswap_mask] "m" (byteswap_mask),
		[k] "m" (k)
		: // TODO :: proper clobbers based off of final assembly
		"rax", "rbx", "rcx", "rdx", "rsi", 
		"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", 
		"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
		"xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
		"memory"
	);

}