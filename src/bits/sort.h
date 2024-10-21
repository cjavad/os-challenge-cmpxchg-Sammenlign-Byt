#pragma once

#include <stdint.h>

void radix_sort16(uint16_t* dest, uint16_t* src, uint16_t* restrict buffer, uint32_t length);
void radix_sort32(uint32_t* dest, uint32_t* src, uint32_t* restrict buffer, uint32_t length);