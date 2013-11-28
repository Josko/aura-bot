#include "crc32.h"

void CCRC32::Initialize( )
{
  for ( uint32_t iCodes = 0; iCodes <= 0xFF; ++iCodes )
  {
    ulTable[iCodes] = Reflect( iCodes, 8 ) << 24;

    for ( uint32_t iPos = 0; iPos < 8; ++iPos )
      ulTable[iCodes] = ( ulTable[iCodes] << 1 ) ^ ( ulTable[iCodes] & ( 1 << 31 ) ? CRC32_POLYNOMIAL : 0 );

    ulTable[iCodes] = Reflect( ulTable[iCodes], 32 );
  }
}

uint32_t CCRC32::Reflect( uint32_t ulReflect, char cChar )
{
  uint32_t ulValue = 0;

  for ( int32_t iPos = 1; iPos < ( cChar + 1 ); ++iPos )
  {
    if ( ulReflect & 1 )
      ulValue |= 1 << ( cChar - iPos );

    ulReflect >>= 1;
  }

  return ulValue;
}

uint32_t CCRC32::FullCRC( uint8_t *sData, uint32_t ulLength )
{
  uint32_t ulCRC = 0xFFFFFFFF;
  PartialCRC( &ulCRC, sData, ulLength );
  return ulCRC ^ 0xFFFFFFFF;
}

void CCRC32::PartialCRC( uint32_t *ulInCRC, uint8_t *sData, uint32_t ulLength )
{
  while ( ulLength-- )
    *ulInCRC = ( *ulInCRC >> 8 ) ^ ulTable[( *ulInCRC & 0xFF ) ^ *sData++];
}
