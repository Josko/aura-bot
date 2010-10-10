/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Broken SHA-1 Implementation
 * October 23, 2004
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
#include <bncsutil/bsha1.h>
#include <cstring>

#define USE_NEW_BSHA1	0

#define BSHA_IC1 0x67452301lu
#define BSHA_IC2 0xEFCDAB89lu
#define BSHA_IC3 0x98BADCFElu
#define BSHA_IC4 0x10325476lu
#define BSHA_IC5 0xC3D2E1F0lu

#define BSHA_OC1 0x5A827999lu
#define BSHA_OC2 0x6ED9EBA1lu
#define BSHA_OC3 0x70E44324lu
#define BSHA_OC4 0x359D3E2Alu

#if !USE_NEW_BSHA1
#	define BSHA_COP e = d; d = c; c = ROL(b, 30); b = a; a = g;
#else
#	define BSHA_N_COP t[4] = t[3]; t[3] = t[2]; t[2] = ROL(t[1], 30); \
		t[1] = t[0]; t[0] = x
#endif

#if !USE_NEW_BSHA1
#define BSHA_OP1(a, b, c, d, e, f, g) g = LSB4(f) + ROL(a, 5) + e + \
    ((b & c) | (~b & d)) + BSHA_OC1; BSHA_COP
#define BSHA_OP2(a, b, c, d, e, f, g) g = (d ^ c ^ b) + e + ROL(g, 5) + \
    LSB4(f) + BSHA_OC2; BSHA_COP
#define BSHA_OP3(a, b, c, d, e, f, g) g = LSB4(f) + ROL(g, 5) + e + \
    ((c & b) | (d & c) | (d & b)) - BSHA_OC3; BSHA_COP
#define BSHA_OP4(a, b, c, d, e, f, g) g = (d ^ c ^ b) + e + ROL(g, 5) + \
    LSB4(f) - BSHA_OC4; BSHA_COP

#else

#define BSHA_N_OP1() x = LSB4(*p++) + ROL(t[0], 5) + t[4] + \
	((t[1] & t[2]) | (~t[1] & t[3])) + BSHA_OC1; BSHA_N_COP
#define BSHA_N_OP2() x = (t[3] ^ t[2] ^ t[1]) + t[4] + ROL(x, 5) + \
	LSB4(*p++) + BSHA_OC2; BSHA_N_COP
#define BSHA_N_OP3() x = LSB4(*p++) + ROL(x, 5) + t[4] + \
	((t[2] & t[1]) | (t[3] & t[2]) | (t[3] & t[1])) - BSHA_OC3; BSHA_N_COP
#define BSHA_N_OP4() x = (t[3] ^ t[2] ^ t[1]) + t[4] + ROL(x, 5) + \
	LSB4(*p++) - BSHA_OC4; BSHA_N_COP
#endif

#if USE_NEW_BSHA1
MEXP(void) calcHashBuf(const char* input, unsigned int length, char* result) {
	uint32_t vals[5];
	uint32_t t[5];		// a, b, c, d, e
	uint32_t buf[0x50];
	uint32_t* p;
	uint32_t x;
	const char* in = input;
	unsigned int i, j;
	unsigned int sub_length;
	
	/* Initializer Values */
	p = vals;
	*p++ = BSHA_IC1;
	*p++ = BSHA_IC2;
	*p++ = BSHA_IC3;
	*p++ = BSHA_IC4;
	*p++ = BSHA_IC5;
	
	memset(buf, 0, 320);		// zero buf
	
	/* Process input in chunks. */
	for (i = 0; i < length; i += 0x40) {
		sub_length = length - i;
		
		/* Maximum chunk size is 0x40 (64) bytes. */
		if (sub_length > 0x40)
			sub_length = 0x40;
		
		memcpy(buf, in, sub_length);
		in += sub_length;
		
		/* If necessary, pad with zeroes to 64 bytes. */
		if (sub_length < 0x40)
			memset(buf + sub_length, 0, 0x40 - sub_length);
		
		for (j = 0; j < 64; j++) {
			buf[j + 16] =
			LSB4(ROL(1, LSB4(buf[j] ^ buf[j+8] ^ buf[j+2] ^ buf[j+13]) % 32));
		}
		
		memcpy(t, vals, 20);
		p = buf;
		
		/* It's a kind of magic. */
		BSHA_N_OP1(); BSHA_N_OP1(); BSHA_N_OP1(); BSHA_N_OP1(); BSHA_N_OP1();
		BSHA_N_OP1(); BSHA_N_OP1(); BSHA_N_OP1(); BSHA_N_OP1(); BSHA_N_OP1();
		
		BSHA_N_OP2(); BSHA_N_OP2(); BSHA_N_OP2(); BSHA_N_OP2(); BSHA_N_OP2();
		BSHA_N_OP2(); BSHA_N_OP2(); BSHA_N_OP2(); BSHA_N_OP2(); BSHA_N_OP2();
		
		BSHA_N_OP3(); BSHA_N_OP3(); BSHA_N_OP3(); BSHA_N_OP3(); BSHA_N_OP3();
		BSHA_N_OP3(); BSHA_N_OP3(); BSHA_N_OP3(); BSHA_N_OP3(); BSHA_N_OP3();
		
		BSHA_N_OP4(); BSHA_N_OP4(); BSHA_N_OP4(); BSHA_N_OP4(); BSHA_N_OP4();
		BSHA_N_OP4(); BSHA_N_OP4(); BSHA_N_OP4(); BSHA_N_OP4(); BSHA_N_OP4();
		
		vals[0] += t[0];
		vals[1] += t[1];
		vals[2] += t[2];
		vals[3] += t[3];
		vals[4] += t[4];
	}
	
	/* Return result. */
	memcpy(result, vals, 20);
}

#else

MEXP(void) calcHashBuf(const char* input, size_t length, char* result) {
    int i;
    uint32_t a, b, c, d, e, g;
	uint32_t* ldata;
    char data[1024];
    memset(data, 0, 1024);
    memcpy(data, input, length);
    ldata = (uint32_t*) data;
    
    for (i = 0; i < 64; i++) {
        ldata[i + 16] =
    LSB4(ROL(1, LSB4(ldata[i] ^ ldata[i+8] ^ ldata[i+2] ^ ldata[i+13]) % 32));
    }
    
	//dumpbuf(data, 1024);
    
    a = BSHA_IC1;
    b = BSHA_IC2;
    c = BSHA_IC3;
    d = BSHA_IC4;
    e = BSHA_IC5;
    g = 0;
    
    // Loops unrolled.
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)
    BSHA_OP1(a, b, c, d, e, *ldata++, g) BSHA_OP1(a, b, c, d, e, *ldata++, g)

    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)
    BSHA_OP2(a, b, c, d, e, *ldata++, g) BSHA_OP2(a, b, c, d, e, *ldata++, g)

    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)
    BSHA_OP3(a, b, c, d, e, *ldata++, g) BSHA_OP3(a, b, c, d, e, *ldata++, g)

    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)
    BSHA_OP4(a, b, c, d, e, *ldata++, g) BSHA_OP4(a, b, c, d, e, *ldata++, g)

    ldata = (uint32_t*) result;
    ldata[0] = LSB4(BSHA_IC1 + a);
    ldata[1] = LSB4(BSHA_IC2 + b);
    ldata[2] = LSB4(BSHA_IC3 + c);
    ldata[3] = LSB4(BSHA_IC4 + d);
    ldata[4] = LSB4(BSHA_IC5 + e);
    ldata = NULL;
}
#endif
