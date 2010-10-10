/*
 * BNCSutil
 * Copyright (c) 2004-2006 Eric Naeseth.
 *
 * Debugging Facilities
 * April 1, 2006
 */

#include <bncsutil/debug.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef MOS_WINDOWS
#include <windows.h>
#else
#include <time.h>
#endif

typedef struct _bncsutil_debug_env
{
	int enabled;
#ifdef MOS_WINDOWS
	HANDLE output;
	BOOL console_was_created;
	WORD orig_attributes;
	WORD sans_foreground;
#else
	FILE* output;
#endif
} debug_env_t;

const char default_err_msg[] = "[unknown error]";

#ifdef MOS_WINDOWS
BOOL debug_set_color(debug_env_t env, WORD color)
{
	return SetConsoleTextAttribute(env.output, color | env.sans_foreground);
}

BOOL debug_restore_color(debug_env_t env)
{
	return SetConsoleTextAttribute(env.output, env.orig_attributes);
}

BOOL debug_intense_color(debug_env_t env)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(env.output, &info))
		return FALSE;
	return SetConsoleTextAttribute(env.output, info.wAttributes |
		FOREGROUND_INTENSITY);
}

BOOL debug_mellow_color(debug_env_t env)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(env.output, &info))
		return FALSE;
	return SetConsoleTextAttribute(env.output, info.wAttributes |
		~FOREGROUND_INTENSITY);
}

void debug_setup_console(debug_env_t env)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (AllocConsole()) {
		// only set title if new console was created
		SetConsoleTitle("BNCSutil Debug Console");
		env.console_was_created = TRUE;
	} else {
		env.console_was_created = FALSE;
	}
	env.output = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(env.output, &info);
	env.orig_attributes = info.wAttributes;
	env.sans_foreground = info.wAttributes &
		~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
		FOREGROUND_INTENSITY);
}
#endif

size_t debug_write(debug_env_t env, const char* message)
{
#ifdef MOS_WINDOWS
	DWORD chars_written;
	BOOL res;
	if (!env.output)
		debug_setup_console(env);
	res = WriteConsole(env.output, message, (DWORD) strlen(message),
		&chars_written, (LPVOID) 0);
	return (res) ? (size_t) chars_written : (size_t) -1;
#else
	return fwrite(message, 1, strlen(message), env->output);
#endif
}

size_t get_console_width(debug_env_t env)
{
#ifdef MOS_WINDOWS
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(env.output, &info))
		return (size_t) 0;
	return (size_t) info.dwSize.X;
#else
	char* columns = getenv("COLUMNS");
	if (!columns)
		return (size_t) 0;
	return (size_t) strtol(columns, (char**) 0, 0);
#endif
}

char* produce_dump(const char* data, size_t length)
{
	return (char*) 0;
}

debug_env_t get_debug_environment()
{
	static debug_env_t env = (debug_env_t) 0;
	
	if (!env) {
		env = (debug_env_t) malloc(sizeof(struct _bncsutil_debug_env));
/*#if defined(BNCSUTIL_DEBUG_MESSAGES) && BNCSUTIL_DEBUG_MESSAGES
		env->enabled = 1;
#else
		env->enabled = 0;
#endif*/
		env->enabled = 0;

#ifdef MOS_WINDOWS
		env->output = (HANDLE) NULL;
		//debug_setup_console(env);
#else
		env->output = stderr;
#endif
	}
	
	return env;
}

/**
 * Returns nonzero if debugging messages are enabled or zero if disabled.
 */
MEXP(int) bncsutil_debug_status(void)
{
	debug_env_t env = get_debug_environment();
	return env->enabled;
}
/**
 * Set new_status to zero to turn debugging messages off or nonzero to turn
 * them on.  Returns nonzero on success and zero on failure.
 */
MEXP(int) bncsutil_set_debug_status(int new_status)
{
	debug_env_t env = get_debug_environment();
	if (!env) {
		return 0;
	}
#ifdef MOS_WINDOWS
	if (env->enabled && !new_status && env->console_was_created) {
		FreeConsole();
	} else if (!env->enabled && new_status) {
		debug_setup_console(env);
	}
#endif
	env->enabled = (new_status != 0) ? 1 : 0;
	return 1;
}

MEXP(int) bncsutil_internal_debug_messages()
{
#if DEBUG
	return 1;
#else
	return 0;
#endif
}

MEXP(void) bncsutil_debug_message(const char* message)
{
	debug_env_t env = get_debug_environment();
	char timestamp[12];
	/*size_t length;*/
#ifdef MOS_WINDOWS
	SYSTEMTIME local_time;
#else
	time_t unix_time;
	struct tm* local_time;
#endif

	if (!env->enabled) {
		return;
	}

#ifdef MOS_WINDOWS
	GetLocalTime(&local_time);
	sprintf(timestamp, "[%02d:%02d:%02d] ", local_time.wHour,
		local_time.wMinute, local_time.wSecond);
	debug_set_color(env, FOREGROUND_RED | FOREGROUND_GREEN |
		FOREGROUND_INTENSITY);
#else	
	time(&unix_time);
	local_time = localtime(&unix_time);
	sprintf(timestamp, "[%02d:%02d:%02d] ", local_time->tm_hour,
		local_time->tm_min, local_time->tm_sec);
#endif


	debug_write(env, timestamp);
#ifdef MOS_WINDOWS
	debug_restore_color(env);
#endif
	
	debug_write(env, message);
	debug_write(env, "\r\n");
}

MEXP(void) bncsutil_debug_message_a(const char* message, ...)
{
	char buf[4092];
	va_list args;
	va_start(args, message);

	vsprintf(buf, message, args);
	va_end(args);
	
	bncsutil_debug_message(buf);
}

MEXP(void) bncsutil_debug_dump(const void* data, size_t data_length)
{
	char ascii[] = "  \0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	char hex[4];
	char pos_indicator[] = "0000  ";
	debug_env_t env = get_debug_environment();
	size_t i;
	size_t j = 2;	/* printing to ASCII buffer skips over 2 spaces */
	size_t k;
	char cur;
	int on_boundary;
	size_t current_string_length;

	if (!env->enabled) {
		return;
	}
	
	debug_intense_color(env);
	for (i = 0; i < data_length; i++) {
		on_boundary = ((i + 1) % 16 == 0);
		
		if ((i + 1) % 16 == 1) {
#ifdef MOS_WINDOWS
			debug_set_color(env, FOREGROUND_RED | FOREGROUND_BLUE |
				FOREGROUND_INTENSITY);
#endif
			debug_write(env, pos_indicator);
#ifdef MOS_WINDOWS
			debug_restore_color(env);
#endif
		}
		
		cur = *(((char*) data) + i);
		/* The cast to unsigned char and then int is required to properly
		 * handle 8 bit characters. */
		ascii[j++] = (isprint((int) (unsigned char) cur)) ? cur : '.';
		sprintf(hex, "%02X ", (cur & 0xFF));
		debug_write(env, hex);
		
		if ((i + 1) % 8 == 0) {
			debug_write(env, " ");
		}
		
		if (on_boundary || (i + 1) == data_length) {
			if (!on_boundary) {
				current_string_length = 3 * (i % 16);
				if ((i % 16) > 8)
					current_string_length++;
				
				for (k = 0; k < (47 - current_string_length); k++) {
					debug_write(env, " ");
				}
			}
#ifdef MOS_WINDOWS
			debug_set_color(env, FOREGROUND_BLUE | FOREGROUND_GREEN |
				FOREGROUND_INTENSITY);
#endif	
			ascii[j] = 0;
			debug_write(env, ascii);
			debug_write(env, "\r\n");
			j = 2;	/* reset position in ASCII buffer */
#ifdef MOS_WINDOWS
			debug_set_color(env, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
			sprintf(pos_indicator, "%04X  ", i + 1);
#ifdef MOS_WINDOWS
			debug_restore_color(env);
#endif	
		}
	}
}

#ifdef MOS_WINDOWS
const char* sys_error_msg()
{
	const char* buffer;
	DWORD res;

	res = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		(const void*) 0, (DWORD) 0, (DWORD) 0, (LPTSTR) &buffer, (DWORD) 0,
		(va_list*) 0);
	if (!res) {
		bncsutil_debug_message("error: Failed to get Windows error message.");
		return default_err_msg;
	}

	return buffer;
}

void free_sys_err_msg(const char* message_pointer)
{
	if (message_pointer)
		LocalFree(message_pointer);
}

#else
const char* sys_error_msg()
{
	return (const char*) strerror(errno);
}

void free_sys_err_msg()
{
	/* Do nothing. */
}
#endif

MEXP(void) bncsutil_print_dump(FILE* stream, const void* data, size_t length)
{
	
}

#ifdef MOS_WINDOWS
MEXP(void) bncsutil_print_dump_win(HANDLE stream, const void* data,
	size_t length)
{
	
}
#endif
