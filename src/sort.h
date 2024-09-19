#pragma once

#include <stdint.h>

// for lsp, because clangd doesn't support multiline literals in C, but does with C++ (gcc does support them in C)
#ifdef __cplusplus
#define restrict __restrict
#endif

void radix_sort16(uint16_t* dest, uint16_t* src, uint16_t* restrict buffer, uint32_t length);
void radix_sort32(uint32_t* dest, uint32_t* src, uint32_t* restrict buffer, uint32_t length);