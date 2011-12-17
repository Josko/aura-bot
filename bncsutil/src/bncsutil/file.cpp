#include <bncsutil/mutil.h>
#include <bncsutil/file.h>
#include <map>
#include <stdexcept>

#ifdef MOS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define BWIN 1
#include <windows.h>

typedef std::map<const void*, HANDLE> mapping_map;
#else
#define BWIN 0
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

typedef std::map<const void*, size_t> mapping_map;
#endif

struct _file
{
#if BWIN
	HANDLE f;
#else
	FILE* f;
#endif
	const char* filename;
	mapping_map mappings;
};

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if BWIN

file_t file_open(const char* filename, unsigned int mode)
{
	file_t data;
	HANDLE file;
	DWORD access;
	DWORD share_mode;
	DWORD open_mode;
	//const char* sys_err;
	size_t filename_buf_len;
	
	if (mode & FILE_READ) {
		access = GENERIC_READ;
		share_mode = FILE_SHARE_READ;
		open_mode = OPEN_EXISTING;
	} else if (mode & FILE_WRITE) {
		access = GENERIC_WRITE;
		share_mode = 0;
		open_mode = CREATE_ALWAYS;
	}
	
	file = CreateFile(filename, access, share_mode, NULL, open_mode,
		FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (file == INVALID_HANDLE_VALUE) {
		//sys_err = sys_error_msg();
		//bncsutil_debug_message_a("Cannot open file \"%s\"; %s", filename, sys_err);
		//free_sys_err_msg(sys_err);
		return ((file_t) 0);
	}
	
	try {
		data = new _file;
	} catch (std::bad_alloc) {
		//bncsutil_debug_message_a("Failed to allocate %u bytes to hold file structure.", sizeof(struct _file));
		CloseHandle(file);
		return (file_t) 0;
	}

	filename_buf_len = strlen(filename) + 1;
	data->filename = (const char*) malloc(filename_buf_len);
	if (!data->filename) {
		//bncsutil_debug_message_a("Failed to allocate %u bytes to hold filename.", filename_buf_len);
		CloseHandle(file);
		delete data;
		return (file_t) 0;
	}
	strcpy_s((char*) data->filename, filename_buf_len, filename);
	
	data->f = file;
	
	return data;
}

void file_close(file_t file)
{
	mapping_map::iterator it;
	
	if (!file) {
		//bncsutil_debug_message_a("error: null pointer given to file_close");
		return;
	}
	
	for (it = file->mappings.begin(); it != file->mappings.end(); it++) {
		UnmapViewOfFile((*it).first);
		CloseHandle((*it).second);
	}
	
	CloseHandle((HANDLE) file->f);
	free((void*) file->filename);
	delete file;
}

size_t file_read(file_t file, void* ptr, size_t size, size_t count)
{
	DWORD bytes_read;
	
	if (!ReadFile(file->f, ptr, (DWORD) (size * count), &bytes_read, NULL)) {
		return (size_t) 0;
	}
	
	return (size_t) bytes_read;
}

size_t file_write(file_t file, const void* ptr, size_t size,
	size_t count)
{
	DWORD bytes_written;
	
	if (!WriteFile(file->f, ptr, (DWORD) (size * count), &bytes_written, NULL))
		return (size_t) 0;
	
	return (size_t) bytes_written;
}

size_t file_size(file_t file)
{
	return (size_t) GetFileSize(file->f, (LPDWORD) 0);
}

void* file_map(file_t file, size_t len, off_t offset)
{
	HANDLE mapping =
		CreateFileMapping((HANDLE) file->f, NULL, PAGE_READONLY, 0, 0, NULL);
	void* base;
	//const char* err;

	if (!mapping) {
		//err = sys_error_msg();
		//bncsutil_debug_message_a("Failed to create file mapping for \"%s\": %s", file->filename, err);
		//free_sys_err_msg(err);
		return (void*) 0;
	}

	base = MapViewOfFile(mapping, FILE_MAP_READ, 0, (DWORD) offset, len);
	if (!base) {
		CloseHandle(mapping);
		//err = sys_error_msg();
		//bncsutil_debug_message_a("Failed to map %u bytes of \"%s\" starting at %u: %s", len, file->filename, offset, err);
		//free_sys_err_msg(err);
		return (void*) 0;
	}
	
	file->mappings[base] = mapping;

	return base;
}

void file_unmap(file_t file, const void* base)
{
	mapping_map::iterator item = file->mappings.find(base);
	HANDLE mapping;
	
	if (item == file->mappings.end()) {
		//bncsutil_debug_message_a("warning: failed to unmap the block starting at %p from %s; unknown block.", base, file->filename);
		return;
	}
	
	mapping = (*item).second;
	
	UnmapViewOfFile(base);
	CloseHandle(mapping);
	
	file->mappings.erase(item);
}

#else

file_t file_open(const char* filename, unsigned int mode_flags)
{
	char mode[] = "rb";
	file_t data;
	FILE* f;
	size_t filename_buf_len;
	//const char* err;
	
	if (mode_flags & FILE_WRITE)
		mode[0] = 'w';
	
	f = fopen(filename, mode);
	if (!f) {
		return (file_t) 0;
	}
	
	try {
		data = new _file;
	} catch (std::bad_alloc) {
		//bncsutil_debug_message_a("Failed to allocate %u bytes to hold file structure.", sizeof(struct _file));
		fclose(f);
		return (file_t) 0;
	}

	filename_buf_len = strlen(filename) + 1;
	data->filename = (const char*) malloc(filename_buf_len);
	if (!data->filename) {
	   //err = sys_error_msg();
		//bncsutil_debug_message_a("Failed to allocate %u bytes to hold filename; %s", filename_buf_len);
		//free_sys_err_msg(err);
		fclose(f);
		delete data;
		return (file_t) 0;
	}
	strcpy((char*) data->filename, filename);
	
	data->f = f;
		
	return data;
}

void file_close(file_t file)
{
        mapping_map::iterator it;
   
	if (!file) {
		//bncsutil_debug_message("error: null pointer given to file_close");
		return;
	}
	
	for (it = file->mappings.begin(); it != file->mappings.end(); it++) {
		munmap((void*) (*it).first, (*it).second);
	}
	
	fclose(file->f);
	delete file;
}

size_t file_read(file_t file, void* ptr, size_t size, size_t count)
{
	return fread(ptr, size, count, file->f);
}

size_t file_write(file_t file, const void* ptr, size_t size,
	size_t count)
{
	return fwrite(ptr, size, count, file->f);
}

size_t file_size(file_t file)
{
	long cur_pos = ftell(file->f);
	size_t size_of_file;
	
	fseek(file->f, 0, SEEK_END);
	size_of_file = (size_t) ftell(file->f);
	fseek(file->f, cur_pos, SEEK_SET);
	
	return size_of_file;
}

void* file_map(file_t file, size_t len, off_t offset)
{
	int fd = fileno(file->f);
	void* base = mmap((void*) 0, len, PROT_READ, MAP_SHARED, fd, offset);
	//const char* err;
	
	if (!base) {
	   //err = sys_error_msg();
		//bncsutil_debug_message_a("error: failed to map %u bytes of %s starting at %u into memory; %s", len, file->filename, offset,	err);
		//free_sys_err_msg(err);
		return (void*) 0;
	}
	
	file->mappings[base] = len;
	
	return base;
}

void file_unmap(file_t file, const void* base)
{
	mapping_map::iterator item = file->mappings.find(base);
	size_t len;
	
	if (item == file->mappings.end()) {
		//bncsutil_debug_message_a("warning: failed to unmap the block starting at %p from %s; unknown block.", base, file->filename);
		return;
	}
	
	len = (*item).second;
	
	munmap((void*) base, len);
	
	file->mappings.erase(item);
}

#endif

#ifdef __cplusplus
}
#endif
