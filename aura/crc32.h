#ifndef CRC32_H
#define CRC32_H

#define CRC32_POLYNOMIAL 0x04c11db7

class CCRC32
{
public:
	void Initialize( );
	uint32_t FullCRC( unsigned char *sData, uint32_t ulLength );
	void PartialCRC( uint32_t *ulInCRC, unsigned char *sData, uint32_t ulLength );

private:
	uint32_t Reflect( uint32_t ulReflect, char cChar );
	uint32_t ulTable[256];
};

#endif
