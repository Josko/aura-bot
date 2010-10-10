/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * CD-Key Decoder Implementation
 * September 29, 2004
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License is included in the BNCSutil
 * distribution in the file COPYING.  If you did not receive this copy,
 * write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 */
 
#include <bncsutil/mutil.h>
#include <bncsutil/cdkeydecoder.h>
#include <bncsutil/keytables.h> // w2/d2 and w3 tables
#include <bncsutil/bsha1.h> // Broken SHA-1
#include <bncsutil/sha1.h> // US Secure Hash Algorithm (for W3)
#include <cctype> // for isdigit(), isalnum(), and toupper()
#include <cstring> // for memcpy()
#include <cstdio> // for sscanf()
// #include <bncsutil/debug.h>

/**
 * Implementation-specific CD-key hash structure.
 */
struct CDKEYHASH {
    uint32_t clientToken;
    uint32_t serverToken;
    uint32_t product;
    uint32_t value1;
    union {
        struct {
            uint32_t zero;
            uint32_t v;
            unsigned char reserved[2];
        } s;
        struct {
            char v[10];
        } l;
    } value2;
};

#define W3_KEYLEN 26
#define W3_BUFLEN (W3_KEYLEN << 1)

/**
 * Creates a new CD-key decoder object.
 * Not really useful unless subclassed.
 */
CDKeyDecoder::CDKeyDecoder() {
    initialized = 0;
    keyOK = 0;
    hashLen = 0;
	cdkey = (char*) 0;
	w3value2 = (char*) 0;
	keyHash = (char*) 0;
}

CDKeyDecoder::CDKeyDecoder(const char* cd_key) {
	CDKeyDecoder(cd_key, std::strlen(cd_key));
}

/**
 * Creates a new CD-key decoder object, using the specified key.
 * keyLength should be the length of the key, NOT INCLUDING the
 * null-terminator.  Applications should use isKeyValid after using
 * this constructor to check the validity of the provided key.
 */
CDKeyDecoder::CDKeyDecoder(const char* cdKey, size_t keyLength) {
    unsigned int i;
    
    initialized = 0;
	product = 0;
	value1 = 0;
	value2 = 0;
    keyOK = 0;
    hashLen = 0;
	cdkey = (char*) 0;
	w3value2 = (char*) 0;
	keyHash = (char*) 0;
    
    if (keyLength <= 0) return;
    
    // Initial sanity check
    if (keyLength == 13) {
        // StarCraft key
        for (i = 0; i < keyLength; i++) {
            if (!isdigit(cdKey[i])) return;
        }
        keyType = KEY_STARCRAFT;
#if DEBUG
				bncsutil_debug_message_a(
					"Created CD key decoder with STAR key %s.", cdKey
				);
#endif
    } else {
        // D2/W2/W3 key
        for (i = 0; i < keyLength; i++) {
            if (!isalnum(cdKey[i])) return;
        }
        switch (keyLength) {
            case 16:
                keyType = KEY_WARCRAFT2;
#if DEBUG
				bncsutil_debug_message_a(
					"Created CD key decoder with W2/D2 key %s.", cdKey
				);
#endif
                break;
            case 26:
                keyType = KEY_WARCRAFT3;
#if DEBUG
				bncsutil_debug_message_a(
					"Created CD key decoder with WAR3 key %s.", cdKey
				);
#endif
                break;
            default:
#if DEBUG
				bncsutil_debug_message_a(
					"Created CD key decoder with unrecognized key %s.", cdKey
				);
#endif
                return;
        }
    }
    
    cdkey = new char[keyLength + 1];
    initialized = 1;
    keyLen = keyLength;
    strcpy(cdkey, cdKey);
    
    switch (keyType) {
        case KEY_STARCRAFT:
            keyOK = processStarCraftKey();
#if DEBUG
			bncsutil_debug_message_a("%s: ok=%d; product=%d; public=%d; "
				"private=%d", cdkey, keyOK, getProduct(), getVal1(), getVal2());
#endif
            break;
        case KEY_WARCRAFT2:
            keyOK = processWarCraft2Key();
#if DEBUG
			bncsutil_debug_message_a("%s: ok=%d; product=%d; public=%d; "
				"private=%d", cdkey, keyOK, getProduct(), getVal1(), getVal2());
#endif
            break;
        case KEY_WARCRAFT3:
            keyOK = processWarCraft3Key();
#if DEBUG
			bncsutil_debug_message_a("%s: ok=%d; product=%d; public=%d; ",
				cdkey, keyOK, getProduct(), getVal1());
#endif
            break;
        default:
            return;
    }
}

CDKeyDecoder::~CDKeyDecoder() {
    if (initialized && cdkey != NULL)
        delete [] cdkey;
    if (hashLen > 0 && keyHash != NULL)
        delete [] keyHash;
	if (w3value2)
		delete [] w3value2;
}

int CDKeyDecoder::isKeyValid() {
    return (initialized && keyOK) ? 1 : 0;
}

int CDKeyDecoder::getVal2Length() {
    return (keyType == KEY_WARCRAFT3) ? 10 : 4;
}

uint32_t CDKeyDecoder::getProduct() {
	switch (keyType) {
		case KEY_STARCRAFT:
		case KEY_WARCRAFT2:
			return (uint32_t) LSB4(product);
		case KEY_WARCRAFT3:
			return (uint32_t) MSB4(product);
		default:
			return (uint32_t) -1;
	}
}

uint32_t CDKeyDecoder::getVal1() {
    switch (keyType) {
		case KEY_STARCRAFT:
		case KEY_WARCRAFT2:
			return (uint32_t) LSB4(value1);
		case KEY_WARCRAFT3:
			return (uint32_t) MSB4(value1);
		default:
			return (uint32_t) -1;
	}
}

uint32_t CDKeyDecoder::getVal2() {
    return (uint32_t) LSB4(value2);
}

int CDKeyDecoder::getLongVal2(char* out) {
    if (w3value2 != NULL && keyType == KEY_WARCRAFT3) {
        memcpy(out, w3value2, 10);
        return 10;
    } else {
        return 0;
    }
}

/**
 * Calculates the CD-Key hash for use in SID_AUTH_CHECK (0x51)
 * Returns the length of the generated hash; call getHash and pass
 * it a character array that is at least this size.  Returns 0 on failure.
 *
 * Note that clientToken and serverToken will be added to the buffer and
 * hashed as-is, regardless of system endianness.  It is assumed that
 * the program's extraction of the server token does not change its
 * endianness, and since the client token is generated by the client,
 * endianness is not a factor.
 */
size_t CDKeyDecoder::calculateHash(uint32_t clientToken,
    uint32_t serverToken)
{
    struct CDKEYHASH kh;
    SHA1Context sha;
    
    if (!initialized || !keyOK) return 0;
    hashLen = 0;
    
    kh.clientToken = clientToken;
    kh.serverToken = serverToken;
    
    switch (keyType) {
        case KEY_STARCRAFT:
        case KEY_WARCRAFT2:
			kh.product = (uint32_t) LSB4(product);
			kh.value1 = (uint32_t) LSB4(value1);

            kh.value2.s.zero = 0;
            kh.value2.s.v = (uint32_t) LSB4(value2);
            
            keyHash = new char[20];
            calcHashBuf((char*) &kh, 24, keyHash);
            hashLen = 20;

#if DEBUG
			bncsutil_debug_message_a("%s: Hash calculated.", cdkey);
			bncsutil_debug_dump(keyHash, 20);
#endif

            return 20;
        case KEY_WARCRAFT3:
			kh.product = (uint32_t) MSB4(product);
			kh.value1 = (uint32_t) MSB4(value1);
            memcpy(kh.value2.l.v, w3value2, 10);

            if (SHA1Reset(&sha))
                return 0;
            if (SHA1Input(&sha, (const unsigned char*) &kh, 26))
                return 0;
            keyHash = new char[20];
            if (SHA1Result(&sha, (unsigned char*) keyHash)) {
                SHA1Reset(&sha);
                return 0;
            }
            SHA1Reset(&sha);
            hashLen = 20;
			
#if DEBUG
			bncsutil_debug_message_a("%s: Hash calculated.", cdkey);
			bncsutil_debug_dump(keyHash, 20);
#endif

            return 20;
        default:
            return 0;
    }
}

/**
 * Places the calculated CD-key hash in outputBuffer.  You must call
 * calculateHash before getHash.  Returns the length of the hash
 * that was copied to outputBuffer, or 0 on failure.
 */
size_t CDKeyDecoder::getHash(char* outputBuffer) {
    if (hashLen == 0 || !keyHash || !outputBuffer)
		return 0;
    memcpy(outputBuffer, keyHash, hashLen);
    return hashLen;
}

/*
void CDKeyDecoder::swapChars(char* string, int a, int b) {
    char temp;
    temp = string[a];
    string[a] = string[b];
    string[b] = temp;
}
*/

int CDKeyDecoder::processStarCraftKey() {
    int accum, pos, i;
    char temp;
    int hashKey = 0x13AC9741;
	char cdkey[14];

	std::strcpy(cdkey, this->cdkey);
    
    // Verification
    accum = 3;
    for (i = 0; i < (int) (keyLen - 1); i++) {
        accum += ((tolower(cdkey[i]) - '0') ^ (accum * 2));
    }
    
	if ((accum % 10) != (cdkey[12] - '0')) {
		// bncsutil_debug_message_a("error: %s is not a valid StarCraft key", cdkey);
        return 0;
	}
    
    // Shuffling
    pos = 0x0B;
    for (i = 0xC2; i >= 7; i -= 0x11) {
        temp = cdkey[pos];
        cdkey[pos] = cdkey[i % 0x0C];
        cdkey[i % 0x0C] = temp;
        pos--;
    }
    
    // Final Value
    for (i = (int) (keyLen - 2); i >= 0; i--) {
        temp = toupper(cdkey[i]);
        cdkey[i] = temp;
        if (temp <= '7') {
            cdkey[i] ^= (char) (hashKey & 7);
            hashKey >>= 3;
        } else if (temp < 'A') {
            cdkey[i] ^= ((char) i & 1);
        }
    }
    
    // Final Calculations
    sscanf(cdkey, "%2ld%7ld%3ld", &product, &value1, &value2);
    
    return 1;
}

int CDKeyDecoder::processWarCraft2Key() {
    unsigned long r, n, n2, v, v2, checksum;
    int i;
    unsigned char c1, c2, c;
	char cdkey[17];

	std::strcpy(cdkey, this->cdkey);
    
    r = 1;
    checksum = 0;
    for (i = 0; i < 16; i += 2) {
        c1 = w2Map[(int) cdkey[i]];
        n = c1 * 3;
        c2 = w2Map[(int) cdkey[i + 1]];
        n = c2 + n * 8;
        
        if (n >= 0x100) {
            n -= 0x100;
            checksum |= r;
        }
        // !
        n2 = n >> 4;
        // !
        cdkey[i] = getHexValue(n2);
        cdkey[i + 1] = getHexValue(n);
        r <<= 1;
    }
    
    v = 3;
    for (i = 0; i < 16; i++) {
        c = cdkey[i];
        n = getNumValue(c);
        n2 = v * 2;
        n ^= n2;
        v += n;
    }
    v &= 0xFF;
    
    if (v != checksum) {
        return 0;
    }
    
    n = 0;
    for (int j = 15; j >= 0; j--) {
        c = cdkey[j];
        if (j > 8) {
            n = (j - 9);
        } else {
            n = (0xF - (8 - j));
        }
        n &= 0xF;
        c2 = cdkey[n];
        cdkey[j] = c2;
        cdkey[n] = c;
    }
    v2 = 0x13AC9741;
    for (int j = 15; j >= 0; j--) {
        c = toupper(cdkey[j]);
        cdkey[j] = c;
        if (c <= '7') {
            v = v2;
            c2 = ((char) (v & 0xFF)) & 7 ^ c;
            v >>= 3;
            cdkey[j] = (char) c2;
            v2 = v;
        } else if (c < 'A') {
            cdkey[j] = ((char) j) & 1 ^ c;
        }
    }

    // Final Calculations
    sscanf(cdkey, "%2lx%6lx%8lx", &product, &value1, &value2);
    return 1;
}

int CDKeyDecoder::processWarCraft3Key() {
    char table[W3_BUFLEN];
    int values[4];
    int a, b;
    int i;
    char decode;
    
    a = 0;
    b = 0x21;
    
    memset(table, 0, W3_BUFLEN);
    memset(values, 0, (sizeof(int) * 4));
    
    for (i = 0; ((unsigned int) i) < keyLen; i++) {
        cdkey[i] = toupper(cdkey[i]);
        a = (b + 0x07B5) % W3_BUFLEN;
        b = (a + 0x07B5) % W3_BUFLEN;
        decode = w3KeyMap[(int)cdkey[i]];
        table[a] = (decode / 5);
        table[b] = (decode % 5);
    }
    
    // Mult
    i = W3_BUFLEN;
    do {
        mult(4, 5, values + 3, table[i - 1]);
    } while (--i);
    
    decodeKeyTable(values);
	
	// 00 00 38 08 f0 64 18 6c 79 14 14 8E B9 49 1D BB
	//          --------
	//            val1

	product = values[0] >> 0xA;
	product = SWAP4(product);
#if LITTLEENDIAN
	for (i = 0; i < 4; i++) {
		values[i] = MSB4(values[i]);
	}
#endif

	value1 = LSB4(*(uint32_t*) (((char*) values) + 2)) & 0xFFFFFF00;

	w3value2 = new char[10];
#if LITTLEENDIAN
	*((uint16_t*) w3value2) = MSB2(*(uint16_t*) (((char*) values) + 6));
	*((uint32_t*) ((char*) w3value2 + 2)) = MSB4(*(uint32_t*) (((char*) values) + 8));
	*((uint32_t*) ((char*) w3value2 + 6)) = MSB4(*(uint32_t*) (((char*) values) + 12));
#else
	*((uint16_t*) w3value2) = LSB2(*(uint16_t*) (((char*) values) + 6));
	*((uint32_t*) ((char*) w3value2 + 2)) = LSB4(*(uint32_t*) (((char*) values) + 8));
	*((uint32_t*) ((char*) w3value2 + 6)) = LSB4(*(uint32_t*) (((char*) values) + 12));
#endif
	return 1;
}

inline void CDKeyDecoder::mult(int r, const int x, int* a, int dcByte) {
    while (r--) {
        int64_t edxeax = ((int64_t) (*a & 0x00000000FFFFFFFFl))
            * ((int64_t) (x & 0x00000000FFFFFFFFl));
        *a-- = dcByte + (int32_t) edxeax;
        dcByte = (int32_t) (edxeax >> 32);
    }
}

void CDKeyDecoder::decodeKeyTable(int* keyTable) {
    unsigned int eax, ebx, ecx, edx, edi, esi, ebp;
    unsigned int varC, var4, var8;
    unsigned int copy[4];
    unsigned char* scopy;
    int* ckt;
    int ckt_temp;
    var8 = 29;
    int i = 464;
    
    // pass 1
    do {
        int j;
        esi = (var8 & 7) << 2;
        var4 = var8 >> 3;
        //varC = (keyTable[3 - var4] & (0xF << esi)) >> esi;
        varC = keyTable[3 - var4];
        varC &= (0xF << esi);
        varC = varC >> esi;
        
        if (i < 464) {
            for (j = 29; (unsigned int) j > var8; j--) {
                /*
                ecx = (j & 7) << 2;
                ebp = (keyTable[0x3 - (j >> 3)] & (0xF << ecx)) >> ecx;
                varC = w3TranslateMap[ebp ^ w3TranslateMap[varC + i] + i];
                */
                ecx = (j & 7) << 2;
                //ebp = (keyTable[0x3 - (j >> 3)] & (0xF << ecx)) >> ecx;
                ebp = (keyTable[0x3 - (j >> 3)]);
                ebp &= (0xF << ecx);
                ebp = ebp >> ecx;
                varC = w3TranslateMap[ebp ^ w3TranslateMap[varC + i] + i];
            }
        }
        
        j = --var8;
        while (j >= 0) {
            ecx = (j & 7) << 2;
            //ebp = (keyTable[0x3 - (j >> 3)] & (0xF << ecx)) >> ecx;
            ebp = (keyTable[0x3 - (j >> 3)]);
            ebp &= (0xF << ecx);
            ebp = ebp >> ecx;
            varC = w3TranslateMap[ebp ^ w3TranslateMap[varC + i] + i];
            j--;
        }
        
        j = 3 - var4;
        ebx = (w3TranslateMap[varC + i] & 0xF) << esi;
        keyTable[j] = (ebx | ~(0xF << esi) & ((int) keyTable[j]));
    } while ((i -= 16) >= 0);
    
    // pass 2
    eax = 0;
    edx = 0;
    ecx = 0;
    edi = 0;
    esi = 0;
    ebp = 0;
    
    for (i = 0; i < 4; i++) {
        copy[i] = LSB4(keyTable[i]);
    }
    scopy = (unsigned char*) copy;
    
    for (edi = 0; edi < 120; edi++) {
        unsigned int location = 12;
        eax = edi & 0x1F;
        ecx = esi & 0x1F;
        edx = 3 - (edi >> 5);
        
        location -= ((esi >> 5) << 2);
        ebp = *(int*) (scopy + location);
        ebp = LSB4(ebp);
        
        //ebp = (ebp & (1 << ecx)) >> ecx;
        ebp &= (1 << ecx);
        ebp = ebp >> ecx;
        
        //keyTable[edx] = ((ebp & 1) << eax) | (~(1 << eax) & keyTable[edx]);
        ckt = (keyTable + edx);
        ckt_temp = *ckt;
        *ckt = ebp & 1;
        *ckt = *ckt << eax;
        *ckt |= (~(1 << eax) & ckt_temp);
        esi += 0xB;
        if (esi >= 120)
            esi -= 120;
    }
}

inline char CDKeyDecoder::getHexValue(int v) {
    v &= 0xF;
    return (v < 10) ? (v + 0x30) : (v + 0x37);
}

inline int CDKeyDecoder::getNumValue(char c) {
    c = toupper(c);
    return (isdigit(c)) ? (c - 0x30) : (c - 0x37);
}
