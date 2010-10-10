/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * Portable Executable Processing
 * August 16, 2005
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

#ifndef CM_PE_H_INCLUDED
#define CM_PE_H_INCLUDED 1

#include "mutil.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cm_pe_header {
	uint32_t signature;
	uint16_t machine;					// IMAGE_FILE_MACHINE_*
	uint16_t section_count;
	uint32_t timestamp;
	uint32_t symbol_table_offset;
	uint32_t symbol_count;
	uint16_t optional_header_size;
	uint16_t characteristics;
} cm_pe_header_t;

#define IMAGE_FORMAT_PE32		0x10B
#define IMAGE_FORMAT_PE32_PLUS	0x20B

typedef struct cm_pe_optional_header {
	uint16_t magic;						// image format (PE32/PE32+)
	uint8_t major_linker_version;
	uint8_t minor_linker_version;
	uint32_t code_section_size;
	uint32_t initialized_data_size;
	uint32_t uninitialized_data_size;
	uint32_t entry_point;
	uint32_t code_base;
	uint32_t data_base;					// not present in PE32+!
} cm_pe_optional_header_t;

#define PE_OPTIONAL_HEADER_MIN_SIZE (sizeof(cm_pe_optional_header_t) - 4)

typedef struct cm_pe_windows_header {
	uint64_t image_base;
	uint32_t section_alignment;
	uint32_t file_alignment;
	uint16_t major_os_version;
	uint16_t minor_os_version;
	uint16_t major_image_version;
	uint16_t minor_image_version;
	uint16_t major_subsystem_version;
	uint16_t minor_subsystem_version;
	uint32_t reserved;
	uint32_t image_size;
	uint32_t headers_size;
	uint32_t checksum;
	uint16_t subsystem;
	uint16_t dll_characteristics;
	uint64_t stack_reserve_size;
	uint64_t stack_commit_size;
	uint64_t heap_reserve_size;
	uint64_t heap_commit_size;
	uint32_t loader_flags;
	uint32_t data_directory_count;
} cm_pe_windows_header_t;

typedef struct cm_pe_data_directory {
	uint32_t rva;
	uint32_t size;
} cm_pe_data_directory_t;

typedef struct cm_pe_section {
	char name[8];
	uint32_t virtual_size;
	uint32_t virtual_address;
	uint32_t raw_data_size;
	uint32_t raw_data_offset;
	uint32_t relocations_offset;
	uint32_t line_numbers_offset;
	uint16_t relocation_count;
	uint16_t line_number_count;
	uint32_t characteristics;
} cm_pe_section_t;

typedef struct VS_FIXEDFILEINFO { 
	uint32_t dwSignature;
	uint32_t dwStrucVersion;
	uint32_t dwFileVersionMS;
	uint32_t dwFileVersionLS;
	union {
		struct {
			uint32_t dwProductVersionMS; 
			uint32_t dwProductVersionLS;
		};
		uint64_t qwProductVersion;
	};
	uint32_t dwFileFlagsMask;
	uint32_t dwFileFlags;
	uint32_t dwFileOS;
	uint32_t dwFileType;
	uint32_t dwFileSubtype;
	union {
		struct {
			uint32_t dwFileDateMS;
			uint32_t dwFileDateLS;
		};
		uint64_t qwFileDate;
	};
} cm_pe_version_t;

typedef struct cm_pe_resource {
	uint32_t name;
	uint32_t offset;
	uint32_t file_offset;
} cm_pe_res_t;
#define CM_RES_REAL_SIZE	8

typedef struct cm_pe_resdir {
	uint32_t characteristics;
	uint32_t timestamp;
	uint16_t major_version;
	uint16_t minor_version;
	uint16_t named_entry_count;
	uint16_t id_entry_count;
	
	size_t subdir_count;
	struct cm_pe_resdir* subdirs;
	size_t resource_count;
	cm_pe_res_t* resources;
	
	uint32_t offset;
	uint32_t name;
} cm_pe_resdir_t;

typedef struct cm_pe {
	FILE* f;
	cm_pe_header_t header;
	cm_pe_optional_header_t optional_header;
	cm_pe_windows_header_t windows_header;
	cm_pe_data_directory_t* data_directories;
	cm_pe_section_t* sections;
} *cm_pe_t;

#define IMAGE_FILE_MACHINE_UNKNOWN  	0x0
#define IMAGE_FILE_MACHINE_ALPHA        0x184
#define IMAGE_FILE_MACHINE_ARM          0x1c0
#define IMAGE_FILE_MACHINE_ALPHA64      0x284
#define IMAGE_FILE_MACHINE_I386         0x14c
#define IMAGE_FILE_MACHINE_IA64         0x200
#define IMAGE_FILE_MACHINE_M68K         0x268
#define IMAGE_FILE_MACHINE_MIPS16       0x266
#define IMAGE_FILE_MACHINE_MIPSFPU      0x366
#define IMAGE_FILE_MACHINE_MIPSFPU16	0x466
#define IMAGE_FILE_MACHINE_POWERPC      0x1f0
#define IMAGE_FILE_MACHINE_R3000        0x162
#define IMAGE_FILE_MACHINE_R4000        0x166
#define IMAGE_FILE_MACHINE_R10000       0x168
#define IMAGE_FILE_MACHINE_SH3          0x1a2
#define IMAGE_FILE_MACHINE_SH4          0x1a6
#define IMAGE_FILE_MACHINE_THUMB        0x1c2

MEXP(cm_pe_t) cm_pe_load(const char* filename);
MEXP(void) cm_pe_unload(cm_pe_t pe);
MEXP(cm_pe_resdir_t*) cm_pe_load_resources(cm_pe_t pe);
MEXP(int) cm_pe_unload_resources(cm_pe_resdir_t* dir);
MEXP(int) cm_pe_fixed_version(cm_pe_t pe, cm_pe_res_t* res,
							  cm_pe_version_t* ver);

#ifdef __cplusplus
}
#endif

#endif
