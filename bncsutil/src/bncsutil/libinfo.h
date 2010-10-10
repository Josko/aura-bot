/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Library Information
 * November 16, 2004
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

#ifndef BNCSUTIL_LIBINFO_H
#define BNCSUTIL_LIBINFO_H

#ifdef __cplusplus
extern "C" {
#endif

// Library Version
// Defined as follows, for library version a.b.c:
// (a * 10^4) + (b * 10^2) + c
// Version 1.2.0:
#define BNCSUTIL_VERSION  10300

// Get Version
MEXP(unsigned long) bncsutil_getVersion();

// Get Version as String
// Copies version into outBuf, returns number of bytes copied (or 0 on fail).
// (outBuf should be at least 9 bytes long)
MEXP(int) bncsutil_getVersionString(char* outbuf);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* BNCSUTIL_LIBINFO_H */
