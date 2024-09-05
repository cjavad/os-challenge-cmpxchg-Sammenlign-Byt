#pragma once
#include <stdint.h>

// The format over the wire is big endian but all values are generated in little
// endian

// Big-endian
typedef struct {
  // Hash to be reversed
  uint8_t hash[32];
  // Range of the input that the hash originates from.
  // 64-bit unsigned integers
  uint8_t start[8];
  uint8_t end[8];
  // Priority
  uint8_t p;

} BEProtocolRequest;

// Little-endian
typedef struct {
  uint64_t hash[4];
  uint64_t start;
  uint64_t end;
  uint8_t p;
} LEProtocolRequest;

// Big-endian
typedef struct {
  uint8_t answer[8];
} BEProtocolResponse;

// Little-endian
typedef struct {
  uint64_t answer;
} LEProtocolResponse;

void protocol_request_to_le(const BEProtocolRequest *be, LEProtocolRequest *le);
void protocol_response_to_le(const BEProtocolResponse *be,
                             LEProtocolResponse *le);

void protocol_request_to_be(const LEProtocolRequest *le, BEProtocolRequest *be);
void protocol_response_to_be(const LEProtocolResponse *le,
                             BEProtocolResponse *be);
