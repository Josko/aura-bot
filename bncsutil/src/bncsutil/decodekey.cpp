
/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * CD-Key Decoder C Wrappers
 * October 17, 2004
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

#ifndef DECODECDKEY_CPP
#define DECODECDKEY_CPP

#include <bncsutil/mutil.h>
#include <bncsutil/cdkeydecoder.h>
#include <bncsutil/decodekey.h>

#ifdef MOS_WINDOWS
	#include <windows.h>
#else
	#include <pthread.h>
	#include <errno.h>
	#include <time.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_DECODERS_SIZE 4
#define MUTEX_TIMEOUT_MS 6000L

#ifndef CLOCKS_PER_SEC
#  ifdef CLK_TCK
#    define CLOCKS_PER_SEC CLK_TCK
#  else
#    define CLOCKS_PER_SEC 1000000
#  endif
#endif

CDKeyDecoder** decoders;
unsigned int numDecoders = 0;
unsigned int sizeDecoders = 0;

#ifdef MOS_WINDOWS
//	HANDLE mutex;
	CRITICAL_SECTION kd_control;
#else
	pthread_mutex_t mutex;
#endif

int kd_lock_decoders() {
#ifdef MOS_WINDOWS
/*	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(mutex, MUTEX_TIMEOUT_MS);
	switch (dwWaitResult) {
		case WAIT_OBJECT_0:
			// success
			break;
		case WAIT_TIMEOUT:
			return 0;
			break;
		case WAIT_ABANDONED:
			// weird, but should be OK
			break;
		default:
			return 0;
			break;
	}*/
	EnterCriticalSection(&kd_control);
#else
	int err = 0;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	struct timespec wait_time = {0, MUTEX_TIMEOUT_MS * 1000};
	
	err = pthread_cond_timedwait(&cond, &mutex, &wait_time);
    switch (err) {
        case 0:
            // success
            break;
        default:
            // error
            return 0;
		}
#endif
    return 1;
}

#ifdef MOS_WINDOWS
#define kd_unlock_decoders() LeaveCriticalSection(&kd_control)
#else
#define kd_unlock_decoders() pthread_mutex_unlock(&mutex)
#endif

MEXP(int) kd_quick(const char* cd_key, uint32_t client_token,
				   uint32_t server_token, uint32_t* public_value,
				   uint32_t* product, char* hash_buffer, size_t buffer_len)
{
	CDKeyDecoder kd(cd_key, strlen(cd_key));
	size_t hash_len;
	
	if (!kd.isKeyValid())
		return 0;
	
	*public_value = kd.getVal1();
	*product = kd.getProduct();
	
	hash_len = kd.calculateHash(client_token, server_token);
	if (!hash_len || hash_len > buffer_len)
		return 0;
	
	kd.getHash(hash_buffer);
	return 1;
}

MEXP(int) kd_init() {
	static int has_run = 0;

	if (has_run)
		return 1;
	
#ifdef MOS_WINDOWS
	/*mutex = CreateMutex(NULL, FALSE, NULL);
	if (mutex == NULL)
		return 0;*/
	InitializeCriticalSection(&kd_control);
#else
	if (pthread_mutex_init(&mutex, NULL))
		return 0;
#endif
	numDecoders = 0;
	sizeDecoders = 0;
	decoders = (CDKeyDecoder**) 0;
	has_run = 1;

#if DEBUG
	bncsutil_debug_message("Initialized key decoding C API.");
#endif

	return 1;
}

unsigned int kd_findAvailable() {
	unsigned int i;
	CDKeyDecoder** d;

	d = decoders;
	for (i = 0; i < sizeDecoders; i++) {
		if (*d == (CDKeyDecoder*) 0)
			return i;
		d++;
	}

	// no room available, must expand
	decoders = (CDKeyDecoder**) realloc(decoders, sizeof(CDKeyDecoder*) *
		(sizeDecoders + DEFAULT_DECODERS_SIZE));
	if (!decoders)
		return (unsigned int) -1;

	memset(decoders + sizeDecoders, 0,
		sizeof(CDKeyDecoder*) * DEFAULT_DECODERS_SIZE); // zero new memory

	i = sizeDecoders;
	sizeDecoders += DEFAULT_DECODERS_SIZE;
	return i;
}

MEXP(int) kd_create(const char* cdkey, int keyLength) {
	unsigned int i;
	CDKeyDecoder** d;
	static int dcs_initialized = 0;

	if (!dcs_initialized) {
		if (!kd_init())
			return -1;
		dcs_initialized = 1;
	}

	if (!kd_lock_decoders()) return -1;

	i = kd_findAvailable();
	if (i == (unsigned int) -1)
		return -1;

	d = (decoders + i);
	*d = new CDKeyDecoder(cdkey, keyLength);
	if (!(**d).isKeyValid()) {
		delete *d;
		*d = (CDKeyDecoder*) 0;
		return -1;
	}

	numDecoders++;

	kd_unlock_decoders();
	return (int) i;
}

/*
MEXP(int) kd_create(char* cdkey, int keyLength) {
    if (!kd_lock_decoders()) return -1;
	CDKeyDecoder* d;
	unsigned int i;
	if (numDecoders >= sizeDecoders) {
		CDKeyDecoder** temp = new CDKeyDecoder*[sizeDecoders];
		for (unsigned int j = 0; j < sizeDecoders; j++) {
			temp[j] = decoders[j];
		}
		delete [] decoders;
		decoders = new CDKeyDecoder*[sizeDecoders + DEFAULT_DECODERS_SIZE];
		
		for (unsigned int j = 0; j < sizeDecoders; j++) {
			decoders[j] = temp[j];
		}
		delete [] temp;
		sizeDecoders += DEFAULT_DECODERS_SIZE;
	}
	i = numDecoders++;
	decoders[i] = new CDKeyDecoder(cdkey, keyLength);
	d = decoders[i];
	if (!d->isKeyValid()) {
		delete decoders[i];
		return -1;
	}
	kd_unlock_decoders();
	
	d = NULL;
	return (int) i;
}*/

MEXP(int) kd_free(int decoder) {
	CDKeyDecoder* d;

	if (!kd_lock_decoders()) return 0;

	if ((unsigned int) decoder >= sizeDecoders)
		return 0;

	d = *(decoders + decoder);
	if (!d)
		return 0;

	delete d;
	*(decoders + decoder) = (CDKeyDecoder*) 0;
	
	kd_unlock_decoders();
	return 1;
}

MEXP(int) kd_val2Length(int decoder) {
	CDKeyDecoder* d;
	int value;

	if (!kd_lock_decoders()) return -1;

	if ((unsigned int) decoder >= sizeDecoders)
		return -1;

	d = *(decoders + decoder);
	if (!d)
		return -1;

	value = d->getVal2Length();
	
	kd_unlock_decoders();
	return value;
}

MEXP(int) kd_product(int decoder) {
	CDKeyDecoder* d;
	int value;

	if (!kd_lock_decoders()) return -1;

	if ((unsigned int) decoder >= sizeDecoders)
		return -1;

	d = *(decoders + decoder);
	if (!d)
		return -1;

	value = d->getProduct();
	
	kd_unlock_decoders();
	return value;
}

MEXP(int) kd_val1(int decoder) {
	CDKeyDecoder* d;
	int value;

	if (!kd_lock_decoders()) return -1;

	if ((unsigned int) decoder >= sizeDecoders)
		return -1;

	d = *(decoders + decoder);
	if (!d)
		return -1;

	value = d->getVal1();
	
	kd_unlock_decoders();
	return value;
}

MEXP(int) kd_val2(int decoder) {
	CDKeyDecoder* d;
	int value;

	if (!kd_lock_decoders()) return -1;

	if ((unsigned int) decoder >= sizeDecoders)
		return -1;

	d = *(decoders + decoder);
	if (!d)
		return -1;

	value = d->getVal2();
	
	kd_unlock_decoders();
	return value;
}

MEXP(int) kd_longVal2(int decoder, char* out) {
	CDKeyDecoder* d;
	int value;

	if (!kd_lock_decoders()) return -1;

	if ((unsigned int) decoder >= sizeDecoders)
		return -1;

	d = *(decoders + decoder);
	if (!d)
		return -1;

	value = d->getLongVal2(out);
	
	kd_unlock_decoders();
	return value;
}

MEXP(int) kd_calculateHash(int decoder, uint32_t clientToken,
						   uint32_t serverToken)
{
	CDKeyDecoder* d;
	int value;

	if (!kd_lock_decoders()) return -1;

	if ((unsigned int) decoder >= sizeDecoders)
		return -1;

	d = *(decoders + decoder);
	if (!d)
		return -1;

	value = (int) d->calculateHash(clientToken, serverToken);
	
	kd_unlock_decoders();
	return value;
}

MEXP(int) kd_getHash(int decoder, char* out) {
	CDKeyDecoder* d;
	int value;

	if (!kd_lock_decoders()) return -1;

	if ((unsigned int) decoder >= sizeDecoders)
		return -1;

	d = *(decoders + decoder);
	if (!d)
		return -1;

	value = (int) d->getHash(out);
	
	kd_unlock_decoders();
	return value;
}

MEXP(int) kd_isValid(int decoder) {
	CDKeyDecoder* d;
	int value;

	if (!kd_lock_decoders()) return -1;

	if ((unsigned int) decoder >= sizeDecoders)
		return -1;

	d = *(decoders + decoder);
	if (!d)
		return -1;

	value = d->isKeyValid();
	
	kd_unlock_decoders();
	return value;
}

#ifdef __cplusplus
}
#endif

#endif /* DECODECDKEY_CPP -- note to self: why is this here? */
