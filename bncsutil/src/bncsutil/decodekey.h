
/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * CD-Key Decoder C Wrappers
 * October 17, 2004
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

#ifndef DECODEKEY_H
#define DECODEKEY_H

#ifdef __cplusplus
extern "C" {
#endif
	
/**
 * Decodes a CD-key, retrieves its relevant values, and calculates a hash
 * suitable for SID_AUTH_CHECK (0x51) in one function call.  Returns 1 on
 * success or 0 on failure.  A call to kd_init does NOT need to be made before
 * calling this function.  Available since BNCSutil 1.1.0.
 */
MEXP(int) kd_quick(const char* cd_key, uint32_t client_token,
				   uint32_t server_token, uint32_t* public_value,
				   uint32_t* product, char* hash_buffer, size_t buffer_len);

/**
 * Initializes the CD-key decoding C wrappers.
 * Returns 1 on success or 0 on failure.
 */
MEXP(int) kd_init();

/**
 * Creates a new CD-key decoder.  Returns its ID on success
 * or -1 on failure or invalid CD-key.
 */
MEXP(int) kd_create(const char* cdkey, int keyLength);

/**
 * Frees the specified CD-key decoder.
 * Returns 1 on success or 0 on failure.
 */
MEXP(int) kd_free(int decoder);

/**
 * Gets the length of the private value for the given decoder.
 * Returns the length or 0 on failure.
 */
MEXP(int) kd_val2Length(int decoder);

/**
 * Gets the product value for the given decoder.
 * Returns the length or 0 on failure.
 */
MEXP(int) kd_product(int decoder);

/**
 * Gets the public value for the given decoder.
 * Returns the length or 0 on failure.
 */
MEXP(int) kd_val1(int decoder);

/**
 * Gets the rivate value for the given decoder.
 * Only use this function if kd_val2Length <= 4.
 * Returns the length or 0 on failure.
 */
MEXP(int) kd_val2(int decoder);

/**
 * Gets the length of the private value for the given decoder.
 * Only use this function if kd_val2Length > 4.
 * Returns the length or 0 on failure.
 */
MEXP(int) kd_longVal2(int decoder, char* out);

/**
 * Calculates the hash of the key values for SID_AUTH_CHECK (0x51).
 * Returns the hash length or 0 on failure.
 */
MEXP(int) kd_calculateHash(int decoder, uint32_t clientToken,
						   uint32_t serverToken);

/**
 * Places the key hash in "out".  The "out" buffer must be
 * at least the length returned from kd_calculateHash.
 * Returns the hash length or 0 on failure.
 */
MEXP(int) kd_getHash(int decoder, char* out);

/**
 * [!] kd_isValid has been deprecated.  kd_create checks
 *     whether the key was valid before returning, and
 *     destroys the decoder and returns an error code
 *     if it was invalid.
 */
MEXP(int) kd_isValid(int decoder);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DECODEKEY_H
