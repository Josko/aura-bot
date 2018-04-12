// Implementation of a fast CRC algorithm taken from (with modifications)
// http://create.stephan-brumme.com/crc32/ on 27.11.2016
// Copyright (c) 2011-2015 Stephan Brumme. All rights reserved.

#include "crc32.h"

// define endianess and some integer data types
#if defined(_MSC_VER) || defined(__MINGW32__)
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#elif defined(__APPLE__)
#include <machine/endian.h>
#define __LITTLE_ENDIAN __DARWIN_LITTLE_ENDIAN
#define __BIG_ENDIAN __DARWIN_BIG_ENDIAN
#define __BYTE_ORDER __DARWIN_BYTE_ORDER
#else
// defines __BYTE_ORDER as __LITTLE_ENDIAN or __BIG_ENDIAN
#include <sys/param.h>
#endif

constexpr std::size_t Polynomial = 0x04C11DB7;

constexpr static inline uint32_t swap(const uint32_t i)
{
#if !defined(__APPLE__) && (defined(__GNUC__) || defined(__clang__))
  return __builtin_bswap32(i);
#else
  return (i >> 24) |
         ((i >>  8) & 0x0000FF00) |
         ((i <<  8) & 0x00FF0000) |
         (i << 24);
#endif
}

void CCRC32::Initialize()
{
  for (uint32_t i = 0; i <= 0xFF; ++i)
  {
    LUT[0][i] = Reflect(i, 8) << 24;

    for (uint32_t iPos = 0; iPos < 8; ++iPos)
      LUT[0][i]        = (LUT[0][i] << 1) ^ (LUT[0][i] & (1 << 31) ? Polynomial : 0);

    LUT[0][i] = Reflect(LUT[0][i], 32);
  }

  for (uint32_t i = 0; i <= 0xFF; ++i)
    for (uint32_t slice = 1; slice < MaxSlices; ++slice)
      LUT[slice][i]     = (LUT[slice - 1][i] >> 8) ^ LUT[0][LUT[slice - 1][i] & 0xFF];
}

uint32_t CCRC32::Reflect(uint32_t reflect, const uint8_t val) const
{
  uint32_t value = 0;

  for (int32_t i = 1; i < (val + 1); ++i)
  {
    if (reflect & 1)
      value |= 1 << (val - i);

    reflect >>= 1;
  }

  return value;
}

uint32_t CCRC32::CalculateCRC(const uint8_t* data, std::size_t length, uint32_t previous_crc) const
{
  uint32_t        crc     = ~previous_crc; // same as previousCrc32 ^ 0xFFFFFFFF
  const uint32_t* current = reinterpret_cast<const uint32_t*>(data);

  // enabling optimization (at least -O2) automatically unrolls the inner for-loop
  constexpr size_t  Unroll      = 4;
  constexpr uint8_t BytesAtOnce = 16 * Unroll;

  while (length >= BytesAtOnce)
  {
    for (size_t unrolling = 0; unrolling < Unroll; ++unrolling)
    {
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
      const uint32_t one   = *current++ ^ swap(crc);
      const uint32_t two   = *current++;
      const uint32_t three = *current++;
      const uint32_t four  = *current++;
      crc                  = LUT[0][four & 0xFF] ^
            LUT[1][(four >> 8) & 0xFF] ^
            LUT[2][(four >> 16) & 0xFF] ^
            LUT[3][(four >> 24) & 0xFF] ^
            LUT[4][three & 0xFF] ^
            LUT[5][(three >> 8) & 0xFF] ^
            LUT[6][(three >> 16) & 0xFF] ^
            LUT[7][(three >> 24) & 0xFF] ^
            LUT[8][two & 0xFF] ^
            LUT[9][(two >> 8) & 0xFF] ^
            LUT[10][(two >> 16) & 0xFF] ^
            LUT[11][(two >> 24) & 0xFF] ^
            LUT[12][one & 0xFF] ^
            LUT[13][(one >> 8) & 0xFF] ^
            LUT[14][(one >> 16) & 0xFF] ^
            LUT[15][(one >> 24) & 0xFF];
#else
      const uint32_t one   = *current++ ^ crc;
      const uint32_t two   = *current++;
      const uint32_t three = *current++;
      const uint32_t four  = *current++;
      crc                  = LUT[0][(four >> 24) & 0xFF] ^
            LUT[1][(four >> 16) & 0xFF] ^
            LUT[2][(four >> 8) & 0xFF] ^
            LUT[3][four & 0xFF] ^
            LUT[4][(three >> 24) & 0xFF] ^
            LUT[5][(three >> 16) & 0xFF] ^
            LUT[6][(three >> 8) & 0xFF] ^
            LUT[7][three & 0xFF] ^
            LUT[8][(two >> 24) & 0xFF] ^
            LUT[9][(two >> 16) & 0xFF] ^
            LUT[10][(two >> 8) & 0xFF] ^
            LUT[11][two & 0xFF] ^
            LUT[12][(one >> 24) & 0xFF] ^
            LUT[13][(one >> 16) & 0xFF] ^
            LUT[14][(one >> 8) & 0xFF] ^
            LUT[15][one & 0xFF];
#endif
    }

    length -= BytesAtOnce;
  }

  const uint8_t* currentChar = reinterpret_cast<const uint8_t*>(current);
  // remaining 1 to 63 bytes (standard algorithm)
  while (length-- != 0)
    crc = (crc >> 8) ^ LUT[0][(crc & 0xFF) ^ *currentChar++];

  return ~crc; // same as crc ^ 0xFFFFFFFF
}
