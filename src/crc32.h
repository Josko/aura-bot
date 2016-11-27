#ifndef AURA_CRC32_H_
#define AURA_CRC32_H_

#include <cstdlib>
#include <cstdint>

constexpr std::size_t MaxSlices = 16;

class CCRC32
{
private:
  uint_fast32_t LUT[MaxSlices][256];
  uint_fast32_t Reflect(uint_fast32_t ulReflect, const uint_fast8_t cChar) const;

public:
  void          Initialize();
  uint_fast32_t CalculateCRC(const uint_fast8_t* data, std::size_t length, uint_fast32_t previous_crc = 0) const;
};

#endif // AURA_CRC32_H_
