/*
  100% free public domain implementation of the SHA-1
  algorithm by Dominik Reichl <Dominik.Reichl@tiscali.de>

 * modified by Trevor Hogan for use with GHost++ *

  === Test Vectors (from FIPS PUB 180-1) ===

  "abc"
    A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D

  "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1

  A million repetitions of "a"
    34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
 */

#ifndef AURA_SHA1_H_
#define AURA_SHA1_H_

#include <stdio.h>  // Needed for file access
#include <memory.h> // Needed for memset and memcpy
#include <string.h> // Needed for strcat and strcpy
#include <cstdint>

#define MAX_FILE_READ_BUFFER 8000

class CSHA1
{
public:
// Rotate x bits to the left
#define ROL32(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#ifdef AURA_BIG_ENDIAN
#define SHABLK0(i) (block->l[i])
#else
#define SHABLK0(i) (block->l[i] = (ROL32(block->l[i], 24) & 0xFF00FF00) | (ROL32(block->l[i], 8) & 0x00FF00FF))
#endif

#define SHABLK(i) (block->l[i & 15] = ROL32(block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15] ^ block->l[(i + 2) & 15] ^ block->l[i & 15], 1))

// SHA-1 rounds
#define R0(v, w, x, y, z, i)                                          \
  {                                                                   \
    z += ((w & (x ^ y)) ^ y) + SHABLK0(i) + 0x5A827999 + ROL32(v, 5); \
    w = ROL32(w, 30);                                                 \
  }
#define R1(v, w, x, y, z, i)                                         \
  {                                                                  \
    z += ((w & (x ^ y)) ^ y) + SHABLK(i) + 0x5A827999 + ROL32(v, 5); \
    w = ROL32(w, 30);                                                \
  }
#define R2(v, w, x, y, z, i)                                 \
  {                                                          \
    z += (w ^ x ^ y) + SHABLK(i) + 0x6ED9EBA1 + ROL32(v, 5); \
    w = ROL32(w, 30);                                        \
  }
#define R3(v, w, x, y, z, i)                                               \
  {                                                                        \
    z += (((w | x) & y) | (w & x)) + SHABLK(i) + 0x8F1BBCDC + ROL32(v, 5); \
    w = ROL32(w, 30);                                                      \
  }
#define R4(v, w, x, y, z, i)                                 \
  {                                                          \
    z += (w ^ x ^ y) + SHABLK(i) + 0xCA62C1D6 + ROL32(v, 5); \
    w = ROL32(w, 30);                                        \
  }

  typedef union {
    uint8_t  c[64];
    uint32_t l[16];
  } SHA1_WORKSPACE_BLOCK;

  // Two different formats for ReportHash(...)

  enum
  {
    REPORT_HEX   = 0,
    REPORT_DIGIT = 1
  };

  // Constructor and Destructor
  CSHA1();
  virtual ~CSHA1();

  uint32_t m_state[5];
  uint32_t m_count[2];
  uint8_t  m_buffer[64];
  uint8_t  m_digest[20];

  void Reset();

  // Update the hash value
  void Update(uint8_t* data, uint32_t len);

  // Finalize hash and report
  void Final();
  void GetHash(uint8_t* uDest);

private:
  // Private SHA-1 transformation
  void Transform(uint32_t state[5], uint8_t buffer[64]);
};

#endif // AURA_SHA1_H_
