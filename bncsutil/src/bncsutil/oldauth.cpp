/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Old Logon System Implementation
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
 
#include <bncsutil/mutil.h> // for MEXP()
#include <bncsutil/bsha1.h>
#include <bncsutil/oldauth.h>
#include <cstring> // for strlen

/**
 * Double-hashes the given password using the given
 * server and client tokens.
 *
 * outBuffer MUST be at least 20 bytes long.
 */
MEXP(void) doubleHashPassword(const char* password, uint32_t clientToken,
uint32_t serverToken, char* outBuffer) {
    char intermediate[28];
    uint32_t* lp;

    calcHashBuf(password, std::strlen(password), intermediate + 8);
    lp = (uint32_t*) &intermediate;
    lp[0] = clientToken;
    lp[1] = serverToken;
    calcHashBuf(intermediate, 28, outBuffer);
	
#if DEBUG
	bncsutil_debug_message_a("doubleHashPassword(\"%s\", 0x%08X, 0x%08X) =",
		password, clientToken, serverToken);
	bncsutil_debug_dump(outBuffer, 20);
#endif
}

/**
 * Single-hashes the password for account creation and password changes.
 *
 * outBuffer MUST be at least 20 bytes long.
 */
MEXP(void) hashPassword(const char* password, char* outBuffer) {
    calcHashBuf(password, std::strlen(password), outBuffer);

#if DEBUG
	bncsutil_debug_message_a("hashPassword(\"%s\") =", password);
	bncsutil_debug_dump(outBuffer, 20);
#endif
}
