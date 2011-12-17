/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Broken SHA-1 Interface
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

#ifndef BSHA1_H
#define BSHA1_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * calcHashBuf
 *
 * Calculates a "Broken SHA-1" hash of data.
 *
 * Paramaters:
 * data: The data to hash.
 * length: The length of data.
 * hash: Buffer, at least 20 bytes in length, to receive the hash.
 */
MEXP(void) calcHashBuf(const char* data, size_t length, char* hash);
	
/*
 * New implementation.  Broken.  No plans to fix.
 */
MEXP(void) bsha1_hash(const char* input, unsigned int length, char* result);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
