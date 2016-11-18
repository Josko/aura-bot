#ifndef AURA_CRC32_H_
#define AURA_CRC32_H_

#include <cstdlib>
#include <cstdint>

constexpr std::size_t MaxSlices = 16;

class CCRC32
{
private:
  uint32_t LUT[MaxSlices][256];
  uint32_t Reflect(uint32_t ulReflect, const uint8_t cChar) const;

public:
  void     Initialize();
  uint32_t CalculateCRC(const uint8_t* data, std::size_t length, uint32_t previous_crc = 0) const;
};

#endif // AURA_CRC32_H_
