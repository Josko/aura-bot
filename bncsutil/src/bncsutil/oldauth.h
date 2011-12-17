/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Old Logon System
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

#ifndef BNCSUTIL_OLDAUTH_H
#define BNCSUTIL_OLDAUTH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Double-hashes the given password using the given
 * server and client tokens.
 */
MEXP(void) doubleHashPassword(const char* password, uint32_t clientToken,
    uint32_t serverToken, char* outBuffer);

/**
 * Single-hashes the password for account creation and password changes.
 */
MEXP(void) hashPassword(const char* password, char* outBuffer);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* BNCSUTIL_OLDAUTH_H */
