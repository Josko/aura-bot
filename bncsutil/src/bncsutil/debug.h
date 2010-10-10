/*
 * BNCSutil
 * Copyright (c) 2004-2006 Eric Naeseth.
 *
 * Debugging Facilities
 * April 1, 2006
 */

/* #ifndef _DEBUG_H_INCLUDED_ */
/* #define _DEBUG_H_INCLUDED_ 1 */

/* #undef DEBUG */
/* #if defined(BNCSUTIL_DEBUG_MESSAGES) && BNCSUTIL_DEBUG_MESSAGES */
/* #	define INTERNAL_DEBUG(msg) bncsutil_debug_message(msg) */
/* #	define INTERNAL_DUMP(data, length) bncsutil_debug_dump((data), (length)) */
/* #	define DEBUG 1 */
/* #else */
/* #	define INTERNAL_DEBUG(msg) */
/* #	define INTERNAL_DUMP(data, length) */
/* #	define DEBUG 0 */
/* #endif */

/* #undef DEBUG */

/* #ifdef __cplusplus */
/* extern "C" { */
/* #endif */

/* #ifdef MOS_WINDOWS */
/* #include <windows.h> */
/* #endif */

/* #include <bncsutil/mutil.h> */
/* #include <stdio.h> */

/* typedef struct _bncsutil_debug_env* debug_env_t; */

/* debug_env_t get_debug_environment(); */

/* /\** */
/*  * Returns nonzero if debugging messages are enabled or zero if disabled. */
/*  *\/ */
/* MEXP(int) bncsutil_debug_status(void); */
/* /\** */
/*  * Set new_status to zero to turn debugging messages off or nonzero to turn */
/*  * them on.  Returns nonzero on success and zero on failure. */
/*  *\/ */
/* MEXP(int) bncsutil_set_debug_status(int new_status); */

/* /\** */
/*  * Returns nonzero if internal debug messages were compiled in or zero if */
/*  * they were not. */
/*  *\/ */
/* MEXP(int) bncsutil_internal_debug_messages(); */

/* /\** */
/*  * Prints a debug message. */
/*  *\/ */
/* MEXP(void) bncsutil_debug_message(const char* message); */

/* /\** */
/*  * Prints a debug message using a printf-style format string. */
/*  *\/ */
/* MEXP(void) bncsutil_debug_message_a(const char* message, ...); */

/* /\** */
/*  * Prints a hex dump of data to the debug console. */
/*  *\/ */
/* MEXP(void) bncsutil_debug_dump(const void* data, size_t data_length); */

/* /\** */
/*  * Returns a pointer to a message describing the last system error event. */
/*  *\/ */
/* const char* sys_error_msg(); */

/* /\** */
/*  * Frees the pointer provided by sys_error_msg(). */
/*  *\/ */
/* void free_sys_err_msg(const char* message_pointer); */

/* /\* Not implemented! *\/ */
/* MEXP(void) bncsutil_print_dump(FILE* stream, const void* data, size_t length); */
/* #ifdef MOS_WINDOWS */
/* //MEXP(void) bncsutil_print_dump_win(HANDLE stream, const void* data, */
/* //	size_t length); */
/* #endif */

/* #ifdef __cplusplus */
/* } // extern "C" */
/* #endif */

/* #endif /\* DEBUG *\/ */


#ifndef __DEBUG_H__
#define __DEBUG_H__

#define bncsutil_debug_message(x)
#define free_sys_err_msg(x)
#define bncsutil_debug_message_a(x, y)
#define sys_error_msg(x)


#endif
