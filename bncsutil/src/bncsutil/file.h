/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * File Access
 * February 12, 2006
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

#ifndef _FILE_H_INCLUDED_
#define _FILE_H_INCLUDED_ 1

#include <bncsutil/mutil.h>

#ifdef MOS_WINDOWS
typedef long off_t;
#else
#include <sys/types.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _file* file_t;

#define FILE_READ	(0x01)
#define FILE_WRITE	(0x02)


file_t file_open(const char* filename, unsigned int mode);
void file_close(file_t file);
size_t file_read(file_t file, void* ptr, size_t size, size_t count);
size_t file_write(file_t file, const void* ptr, size_t size,
	size_t count);
size_t file_size(file_t file);

void* file_map(file_t file, size_t len, off_t offset);
void file_unmap(file_t file, const void* mapping);

#ifdef __cplusplus
}
#endif

#endif /* FILE */
