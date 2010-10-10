/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * CheckRevision Implementation
 * November 12, 2004
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
#include <bncsutil/checkrevision.h>
#include <bncsutil/file.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#ifdef MOS_WINDOWS
#include <windows.h>
#else
#include <bncsutil/pe.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#endif

#ifndef HIWORD
#define HIWORD(l) ((uint16_t) ((l) >> 16))
#endif

#ifndef LOWORD
#define LOWORD(l) ((uint16_t) ((l) & 0xFFFF))
#endif

#ifdef HAVE__SNPRINTF
#define snprintf _snprintf
#endif

// BNCSutil - CheckRevision - GetNumber
#define BUCR_GETNUM(ch) (((ch) == 'S') ? 3 : ((ch) - 'A'))
// BNCSutil - CheckRevision - IsNumber
#define BUCR_ISNUM(ch) (((ch) >= '0') && ((ch) <= '9'))

#include <vector>

#ifdef __cplusplus
extern "C" {
#endif


std::vector<long> checkrevision_seeds;
void initialize_checkrevision_seeds()
{
	static bool run = false;
	
	if (run)
		return;
	
	run = true;
	
	checkrevision_seeds.reserve(8);
	
	checkrevision_seeds.push_back(0xE7F4CB62);
	checkrevision_seeds.push_back(0xF6A14FFC);
	checkrevision_seeds.push_back(0xAA5504AF);
	checkrevision_seeds.push_back(0x871FCDC2);
	checkrevision_seeds.push_back(0x11BF6A18);
	checkrevision_seeds.push_back(0xC57292E6);
	checkrevision_seeds.push_back(0x7927D27E);
	checkrevision_seeds.push_back(0x2FEC8733);
}

MEXP(long) get_mpq_seed(int mpq_number)
{
	if (((size_t) mpq_number) >= checkrevision_seeds.size()) {
		//bncsutil_debug_message_a("error: no known revision check seed for "
		//	"MPQ#%u", mpq_number);
		return 0;
	}
	
	return checkrevision_seeds[mpq_number];
}

MEXP(long) set_mpq_seed(int mpq_number, long new_seed)
{
	long ret;
	
	if (((size_t) mpq_number) >= checkrevision_seeds.size()) {
		ret = 0;
		checkrevision_seeds.reserve((size_t) mpq_number);
	} else {
		ret = checkrevision_seeds[mpq_number];
	}
	
	checkrevision_seeds[mpq_number] = new_seed;
	return ret;
}

MEXP(int) extractMPQNumber(const char* mpqName)
{
	const char* n;
	int mpqNum;
	if (mpqName == NULL)
		return -1;
	if ((n = (const char*) std::strchr(mpqName, '.')) == NULL)
		return -1;
	// extract int value of version number
	mpqNum = atoi(n - 1);
	return mpqNum;
}

const char* get_basename(const char* file_name)
{
	const char* base;

	for (base = (file_name + strlen(file_name)); base >= file_name; base--) {
		if (*base == '\\' || *base == '/')
			break;
	}
	
	return ++base;
}

MEXP(int) checkRevision(const char* formula, const char* files[], int numFiles,
	int mpqNumber, unsigned long* checksum)
{
	uint64_t values[4];
	long ovd[4], ovs1[4], ovs2[4];
	char ops[4];
	const char* token;
	int curFormula = 0;
	file_t f;
	uint8_t* file_buffer;
	uint32_t* dwBuf;
	uint32_t* current;
	size_t seed_count;
	
#if DEBUG
	int i;
	bncsutil_debug_message_a("checkRevision(\"%s\", {", formula);
	for (i = 0; i < numFiles; i++) {
		bncsutil_debug_message_a("\t\"%s\",", files[i]);
	}
	bncsutil_debug_message_a("}, %d, %d, %p);", numFiles, mpqNumber, checksum);
#endif
	
	if (!formula || !files || numFiles == 0 || mpqNumber < 0 || !checksum) {
		//bncsutil_debug_message("error: checkRevision() parameter sanity check "
		//	"failed");
		return 0;
	}
	
	seed_count = checkrevision_seeds.size();
	if (seed_count == 0) {
		initialize_checkrevision_seeds();
		seed_count = checkrevision_seeds.size();
	}
	
	if (seed_count <= (size_t) mpqNumber) {
		//bncsutil_debug_message_a("error: no revision check seed value defined "
		//	"for MPQ number %d", mpqNumber);
		return 0;
	}
	
	token = formula;
	while (token && *token) {
		if (*(token + 1) == '=') {
			int variable = BUCR_GETNUM(*token);
			if (variable < 0 || variable > 3) {
				//bncsutil_debug_message_a("error: Unknown revision check formula"
				//	" variable %c", *token);
				return 0;
			}
			
			token += 2; // skip over equals sign
			if (BUCR_ISNUM(*token)) {
				values[variable] = ATOL64(token);
			} else {
				if (curFormula > 3) {
					// more than 4 operations?  bloody hell.
					//bncsutil_debug_message("error: Revision check formula"
					//	" contains more than 4 operations; unsupported.");
					return 0;
				}
				ovd[curFormula] = variable;
				ovs1[curFormula] = BUCR_GETNUM(*token);
				ops[curFormula] = *(token + 1);
				ovs2[curFormula] = BUCR_GETNUM(*(token + 2));
				curFormula++;
			}
		}
		
		for (; *token != 0; token++) {
			if (*token == ' ') {
				token++;
				break;
			}
		}
	}
	
	// Actual hashing (yay!)
	// "hash A by the hashcode"
	values[0] ^= checkrevision_seeds[mpqNumber];
	
	for (int i = 0; i < numFiles; i++) {
		size_t file_len, remainder, rounded_size, buffer_size;
		
		f = file_open(files[i], FILE_READ);
		if (!f) {
			//bncsutil_debug_message_a("error: Failed to open file %s",
			//	files[i]);
			return 0;
		}
		
		file_len = file_size(f);
		remainder = file_len % 1024;
		rounded_size = file_len - remainder;
		
		file_buffer = (uint8_t*) file_map(f, file_len, 0);
		if (!file_buffer) {
			file_close(f);
			//bncsutil_debug_message_a("error: Failed to map file %s into memory",
			//	files[i]);
			return 0;
		}
		
		if (remainder == 0) {
			// Mapped buffer may be used directly, without padding.
			dwBuf = (uint32_t*) file_buffer;
			buffer_size = file_len;
		} else {
			// Must be padded to nearest KB.
			size_t extra = 1024 - remainder;
			uint8_t pad = (uint8_t) 0xFF;
			uint8_t* pad_dest;
			
			buffer_size = file_len + extra;
			dwBuf = (uint32_t*) malloc(buffer_size);
			if (!dwBuf) {
				//bncsutil_debug_message_a("error: Failed to allocate %d bytes "
				//	"of memory as a temporary buffer", buffer_size);
				file_unmap(f, file_buffer);
				file_close(f);
				return 0;
			}
			
			memcpy(dwBuf, file_buffer, file_len);
			file_unmap(f, file_buffer);
			file_buffer = (uint8_t*) 0;
			
			pad_dest = ((uint8_t*) dwBuf) + file_len;
			for (size_t j = file_len; j < buffer_size; j++) {
				*pad_dest++ = pad--;
			}
			
		}

		current = dwBuf;
		for (size_t j = 0; j < buffer_size; j += 4) {
			values[3] = LSB4(*(current++));
			for (int k = 0; k < curFormula; k++) {
				switch (ops[k]) {
					case '+':
						values[ovd[k]] = values[ovs1[k]] + values[ovs2[k]];
						break;
					case '-':
						values[ovd[k]] = values[ovs1[k]] - values[ovs2[k]];
						break;
					case '^':
						values[ovd[k]] = values[ovs1[k]] ^ values[ovs2[k]];
						break;
					case '*':
						// well, you never know
						values[ovd[k]] = values[ovs1[k]] * values[ovs2[k]];
						break;
					case '/':
						// well, you never know
						values[ovd[k]] = values[ovs1[k]] / values[ovs2[k]];
						break;
					default:
						// unrecognized operation
						// shit
						file_unmap(f, dwBuf);
						file_close(f);
						return 0;
				}
			}
		}

		if (file_buffer)
 			file_unmap(f, file_buffer);
		else if (dwBuf && file_buffer == 0)
			free(dwBuf); // padded buffer
		file_close(f);
	}

	*checksum = (unsigned long) LSB4(values[2]);
#if DEBUG
	bncsutil_debug_message_a("\tChecksum = %lu", *checksum);
#endif
	return 1;
}

MEXP(int) checkRevisionFlat(const char* valueString, const char* file1,
const char* file2, const char* file3, int mpqNumber, unsigned long* checksum)
{
	const char* files[] =
		{file1, file2, file3};
	return checkRevision(valueString, files, 3, mpqNumber,
		checksum);
}

MEXP(int) getExeInfo(const char* file_name, char* exe_info,
					 size_t exe_info_size, uint32_t* version, int platform)
{
	const char* base = (char*) 0;
	unsigned long file_size;
	FILE* f = (FILE*) 0;
	int ret;
#ifdef MOS_WINDOWS
	HANDLE hFile;
	FILETIME ft;
	SYSTEMTIME st;
	LPBYTE buf;
	VS_FIXEDFILEINFO* ffi;
	DWORD infoSize, bytesRead;
#else
	cm_pe_t pe;
	cm_pe_resdir_t* root;
	cm_pe_resdir_t* dir;
	cm_pe_version_t ffi;
	size_t i;
	struct stat st;
	struct tm* time;
#endif
	
	if (!file_name || !exe_info || !exe_info_size || !version)
		return 0;

	base = get_basename(file_name);

	switch (platform) {
		case BNCSUTIL_PLATFORM_X86:
#ifdef MOS_WINDOWS				
			infoSize = GetFileVersionInfoSize(file_name, &bytesRead);
			if (infoSize == 0)
				return 0;
			buf = (LPBYTE) VirtualAlloc(NULL, infoSize, MEM_COMMIT,
										PAGE_READWRITE);
			if (buf == NULL)
				return 0;
			if (GetFileVersionInfo(file_name, NULL, infoSize, buf) == FALSE)
				return 0;
			if (!VerQueryValue(buf, "\\", (LPVOID*) &ffi, (PUINT) &infoSize))
				return 0;
			
			*version =
				((HIWORD(ffi->dwProductVersionMS) & 0xFF) << 24) |
				((LOWORD(ffi->dwProductVersionMS) & 0xFF) << 16) |
				((HIWORD(ffi->dwProductVersionLS) & 0xFF) << 8) | 
				(LOWORD(ffi->dwProductVersionLS) & 0xFF);
#if DEBUG
			bncsutil_debug_message_a("%s version = %d.%d.%d.%d (0x%08X)",
				base, (HIWORD(ffi->dwProductVersionMS) & 0xFF),
				(LOWORD(ffi->dwProductVersionMS) & 0xFF),
				(HIWORD(ffi->dwProductVersionLS) & 0xFF),
				(LOWORD(ffi->dwProductVersionLS) & 0xFF),
				*version);
#endif
			VirtualFree(buf, 0lu, MEM_RELEASE);
#else
			pe = cm_pe_load(file_name);
			if (!pe)
				return 0;
			root = cm_pe_load_resources(pe);
			if (!root) {
				cm_pe_unload(pe);
				return 0;
			}
				
			for (i = 0; i < root->subdir_count; i++) {
				dir = (root->subdirs + i);
				if (dir->name == 16) {
					if (!cm_pe_fixed_version(pe, dir->subdirs->resources,
											 &ffi))
					{
						cm_pe_unload_resources(root);
						cm_pe_unload(pe);
						return 0;
					}
					break;
				}
			}
			*version =
				((HIWORD(ffi.dwProductVersionMS) & 0xFF) << 24) |
				((LOWORD(ffi.dwProductVersionMS) & 0xFF) << 16) |
				((HIWORD(ffi.dwProductVersionLS) & 0xFF) << 8) | 
				(LOWORD(ffi.dwProductVersionLS) & 0xFF);
#if DEBUG
			bncsutil_debug_message_a("%s version = %d.%d.%d.%d (0x%08X)",
				base, (HIWORD(ffi.dwProductVersionMS) & 0xFF),
				(LOWORD(ffi.dwProductVersionMS) & 0xFF),
				(HIWORD(ffi.dwProductVersionLS) & 0xFF),
				(LOWORD(ffi.dwProductVersionLS) & 0xFF),
				*version);
#endif
			
			cm_pe_unload_resources(root);
			cm_pe_unload(pe);
#endif
			break;
		case BNCSUTIL_PLATFORM_MAC:
		case BNCSUTIL_PLATFORM_OSX:
			f = fopen(file_name, "r");
			if (!f)
				return 0;
			if (fseek(f, -4, SEEK_END) != 0) {
				fclose(f);
				return 0;
			}
			if (fread(version, 4, 1, f) != 1) {
				fclose(f);
				return 0;
			}
#ifdef MOS_WINDOWS
			fclose(f);
#endif
	}
	
#ifdef MOS_WINDOWS
	hFile = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ, NULL,
					   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;
	file_size = GetFileSize(hFile, NULL);
	if (!GetFileTime(hFile, &ft, NULL, NULL)) {
		CloseHandle(hFile);
		return 0;
	}
	
	if (!FileTimeToSystemTime(&ft, &st)) {
		CloseHandle(hFile);
		return 0;
	}
	CloseHandle(hFile);
	
	ret = snprintf(exe_info, exe_info_size,
				   "%s %02u/%02u/%02u %02u:%02u:%02u %lu", base, st.wMonth,
				   st.wDay, (st.wYear % 100), st.wHour, st.wMinute, st.wSecond,
				   file_size);

#else
	if (!f)
		f = fopen(file_name, "r");
	if (!f)
		return 0;
	if (fseek(f, 0, SEEK_END) == -1) {
		fclose(f);
		return 0;
	}
	file_size = ftell(f);
	fclose(f);
	
	if (stat(file_name, &st) != 0)
		return 0;
	
	time = gmtime(&st.st_mtime);
	if (!time)
		return 0;
	
	switch (platform) {
		case BNCSUTIL_PLATFORM_MAC:
		case BNCSUTIL_PLATFORM_OSX:
			if (time->tm_year >= 100) // y2k
				time->tm_year -= 100;
			break;
	}
	
	ret = (int) snprintf(exe_info, exe_info_size,
						"%s %02u/%02u/%02u %02u:%02u:%02u %lu", base, 
						(time->tm_mon+1), time->tm_mday, time->tm_year,
						time->tm_hour, time->tm_min, time->tm_sec, file_size);
#endif

#if DEBUG
	bncsutil_debug_message(exe_info);
#endif

	return ret;
}

#ifdef __cplusplus
} // extern "C"
#endif

