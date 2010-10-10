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

#include <bncsutil/mutil.h>
#include <bncsutil/libinfo.h>
#include <cstdio>

MEXP(unsigned long) bncsutil_getVersion() {
    return BNCSUTIL_VERSION;
}

MEXP(int) bncsutil_getVersionString(char* outBuf) {
    unsigned long major, minor, rev, ver;
    int printed;
    ver = BNCSUTIL_VERSION;
    // major
    major = (unsigned long) (BNCSUTIL_VERSION / 10000);
    if (major > 99 || major < 0) return 0;
    
    // minor
    ver -= (major * 10000);
    minor = (unsigned long) (ver / 100);
    if (minor > 99 || minor < 0) return 0;
    
    // revision
    ver -= (minor * 100);
    rev = ver;
    if (rev > 99 || rev < 0) return 0;
    
    printed = std::sprintf(outBuf, "%lu.%lu.%lu", major, minor, rev);
    if (printed < 0) return 0;
    outBuf[8] = '\0';
    return printed;
}
