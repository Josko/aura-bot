/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Utility Headers
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


#ifndef MUTIL_H
#define MUTIL_H

#define LITTLEENDIAN 1


#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

/* Specific-Sized Integers */
#include "mutil_types.h"
#include	 <stdlib.h>	

// functions for converting a string to a 64-bit number.
#if defined(_MSC_VER)
#define ATOL64(x) _atoi64(x)
#else
#define ATOL64(x) atoll(x)
#endif

#ifdef _MSC_VER
#pragma intrinsic(_lrotl,_lrotr)		/* use intrinsic compiler rotations */
#define	ROL(x,n)	_lrotl((x),(n))			
#define	ROR(x,n)	_lrotr((x),(n))
#else
#ifndef ROL
#define ROL(a,b) (((a) << (b)) | ((a) >> 32 - (b)))
#endif
#ifndef ROR
#define ROR(a,b) (((a) >> (b)) | ((a) << 32 - (b)))
#endif
#endif

#if (!defined(MUTIL_CPU_PPC))
    #if (defined(__ppc__) || defined(__PPC__) || defined(powerpc) || \
    defined(powerpc) || defined(ppc) || defined(_M_MPPC))
        #define MUTIL_CPU_PPC 1
        #define MUTIL_CPU_X86 0
    #else
        #define MUTIL_CPU_PPC 0
    #endif
#endif

#if (!defined(MUTIL_CPU_X86))
    #if (__INTEL__ || defined(__i386__) || defined(i386) || defined(intel) || \
    defined(_M_IX86))
        #define MUTIL_CPU_X86 1
        #ifndef MUTIL_CPU_PPC
            #define MUTIL_CPU_PPC 0
        #endif
    #else
        #define MUTIL_CPU_X86 0
    #endif
#endif

#if (!defined(BIGENDIAN)) && (!defined(LITTLEENDIAN))
#if MUTIL_CPU_PPC
    #define BIGENDIAN 1
    #define LITTLEENDIAN 0
#elif MUTIL_CPU_X86
    #define LITTLEENDIAN 1
    #define BIGENDIAN 0
#else
    #error Unable to determine byte order, define BIGENDIAN or LITTLEENDIAN.
#endif
#elif defined(BIGENDIAN) && (!defined(LITTLEENDIAN))
    #undef BIGENDIAN
    #define BIGENDIAN 1
    #define LITTLEENDIAN 0
#elif defined(LITTLEENDIAN) && (!defined(BIGENDIAN))
    #undef LITTLEENDIAN
    #define LITTLEENDIAN 1
    #define BIGENDIAN 0
#endif

#define SWAP2(num) ((((num) >> 8) & 0x00FF) | (((num) << 8) & 0xFF00))
#define SWAP4(num) ((((num) >> 24) & 0x000000FF) | (((num) >> 8) & 0x0000FF00) | (((num) << 8) & 0x00FF0000) | (((num) << 24) & 0xFF000000))
#define SWAP8(x)													   \
	(uint64_t)((((uint64_t)(x) & 0xff) << 56) |						   \
            ((uint64_t)(x) & 0xff00ULL) << 40 |                        \
            ((uint64_t)(x) & 0xff0000ULL) << 24 |                      \
            ((uint64_t)(x) & 0xff000000ULL) << 8 |                     \
            ((uint64_t)(x) & 0xff00000000ULL) >> 8 |                   \
            ((uint64_t)(x) & 0xff0000000000ULL) >> 24 |                \
            ((uint64_t)(x) & 0xff000000000000ULL) >> 40 |              \
            ((uint64_t)(x) & 0xff00000000000000ULL) >> 56)

/* For those who think in bits */
#define SWAP16 SWAP2
#define SWAP32 SWAP4
#define SWAP64 SWAP8

#if BIGENDIAN
#define LSB2(num) SWAP2(num)
#define LSB4(num) SWAP4(num)
#define MSB2(num) (num)
#define MSB4(num) (num)
#else /* (little endian) */
#define LSB2(num) (num)
#define LSB4(num) (num)
#define MSB2(num) SWAP2(num)
#define MSB4(num) SWAP4(num)
#endif /* (endianness) */

#ifndef MOS_WINDOWS 
/* attempt automatic Windows detection  */
  #ifdef _MSC_VER 
    /* Microsoft C++ compiler, has to be windows  */
    #define MOS_WINDOWS 
  #else 
    #if defined(_WIN) || defined(_WINDOWS) || defined(WINDOWS) || \
    defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
      #define MOS_WINDOWS
    #endif 
  #endif 
#endif

#if !(defined(MUTIL_LIB_BUILD)) && defined(BNCSUTIL_EXPORTS)
#  define MUTIL_LIB_BUILD
#endif

#ifdef MOS_WINDOWS
#  ifdef MUTIL_LIB_BUILD
#    if 1
#      define MEXP(type) __declspec(dllexport) type __stdcall
#    else
#      define MEXP(type) type __stdcall
#    endif
#    define MCEXP(name) class __declspec(dllexport) name
#  else
#    define MEXP(type) __declspec(dllimport) type __stdcall
#    define MCEXP(name) class __declspec(dllimport) name
#  endif
#else
#  ifdef MUTIL_LIB_BUILD
#    define MEXP(type) type
#    define MCEXP(name) class name
#  else
#    define MEXP(type) extern type
#    define MCEXP(name) class name
#  endif
#endif
#define MYRIAD_UTIL

#ifndef NULL
#define NULL 0
#endif /* NULL */

// #include <bncsutil/debug.h>

#endif /* MUTIL_H */
