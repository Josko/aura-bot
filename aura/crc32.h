#ifndef AURA_CRC32_H_
#define AURA_CRC32_H_

#define CRC32_POLYNOMIAL 0x04c11db7

#include <cstdint>

class CCRC32
{
public:
  void Initialize( );
  uint32_t FullCRC( uint8_t *sData, uint32_t ulLength );
  void PartialCRC( uint32_t *ulInCRC, uint8_t *sData, uint32_t ulLength );

private:
  uint32_t Reflect( uint32_t ulReflect, char cChar );
  uint32_t ulTable[256];
};

#endif  // AURA_CRC32_H_
