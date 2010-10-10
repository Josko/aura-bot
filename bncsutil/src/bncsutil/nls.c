/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * New Logon System (SRP) Implementation
 * February 13, 2005
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

#include <bncsutil/nls.h>
#include <bncsutil/sha1.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "gmp.h"

struct _nls {
    const char* username;
    const char* password;
    unsigned long username_len;
    unsigned long password_len;
    
    mpz_t n;
    mpz_t a;
    
    gmp_randstate_t rand;

	char* A;
	char* S;
	char* K;
	char* M1;
	char* M2;
};

#ifdef MOS_WINDOWS
#  include <windows.h>
#  if DEBUG
#    define nls_dbg(msg) bncsutil_debug_message(msg)
#  else
#    define nls_dbg(msg)
#  endif
#else
#  define nls_dbg(msg)
#  include <time.h>
#endif

/* Raw large integer constants. */
const char bncsutil_NLS_I[] = {
    0x6c, 0xe, 0x97, 0xed, 0xa, 0xf9, 0x6b, 0xab, 0xb1, 0x58, 0x89, 0xeb,
    0x8b, 0xba, 0x25, 0xa4, 0xf0, 0x8c, 0x1, 0xf8
};

const char bncsutil_NLS_sig_n[] = {
    0xD5, 0xA3, 0xD6, 0xAB, 0x0F, 0x0D, 0xC5, 0x0F, 0xC3, 0xFA, 0x6E, 0x78,
    0x9D, 0x0B, 0xE3, 0x32, 0xB0, 0xFA, 0x20, 0xE8, 0x42, 0x19, 0xB4, 0xA1,
    0x3A, 0x3B, 0xCD, 0x0E, 0x8F, 0xB5, 0x56, 0xB5, 0xDC, 0xE5, 0xC1, 0xFC,
    0x2D, 0xBA, 0x56, 0x35, 0x29, 0x0F, 0x48, 0x0B, 0x15, 0x5A, 0x39, 0xFC,
    0x88, 0x07, 0x43, 0x9E, 0xCB, 0xF3, 0xB8, 0x73, 0xC9, 0xE1, 0x77, 0xD5,
    0xA1, 0x06, 0xA6, 0x20, 0xD0, 0x82, 0xC5, 0x2D, 0x4D, 0xD3, 0x25, 0xF4,
    0xFD, 0x26, 0xFC, 0xE4, 0xC2, 0x00, 0xDD, 0x98, 0x2A, 0xF4, 0x3D, 0x5E,
    0x08, 0x8A, 0xD3, 0x20, 0x41, 0x84, 0x32, 0x69, 0x8E, 0x8A, 0x34, 0x76,
    0xEA, 0x16, 0x8E, 0x66, 0x40, 0xD9, 0x32, 0xB0, 0x2D, 0xF5, 0xBD, 0xE7,
    0x57, 0x51, 0x78, 0x96, 0xC2, 0xED, 0x40, 0x41, 0xCC, 0x54, 0x9D, 0xFD,
    0xB6, 0x8D, 0xC2, 0xBA, 0x7F, 0x69, 0x8D, 0xCF
};

/* Private-use function prototypes. */

unsigned long nls_pre_seed();

void nls_get_x(nls_t* nls, mpz_t x_c, const char* raw_salt);

void nls_get_v_mpz(nls_t* nls, mpz_t v, mpz_t x);

uint32_t nls_get_u(const char* B);

/* Function definitons */

MEXP(nls_t*) nls_init(const char* username, const char* password) {
    return nls_init_l(username, (unsigned long) strlen(username),
		password, (unsigned long) strlen(password));
}

MEXP(nls_t*) nls_init_l(const char* username, unsigned long username_length,
    const char* password, unsigned long password_length)
{
    unsigned int i;
    char* d; /* destination */
    const char* o; /* original */
    nls_t* nls;
    
    nls = (nls_t*) malloc(sizeof(nls_t));
    if (!nls)
        return (nls_t*) 0;
    
    nls->username_len = username_length;
    nls->password_len = password_length;
    
    nls->username = (char*) malloc(nls->username_len + 1);
    nls->password = (char*) malloc(nls->password_len + 1);
    if (!nls->username || !nls->password) {
        free(nls);
        return (nls_t*) 0;
    }
    
    d = (char*) nls->username;
    o = username;
    for (i = 0; i < nls->username_len; i++) {
        *d = (char) toupper(*o);
        d++;
        o++;
    }
    
    *((char*) nls->username + username_length) = 0;
    *((char*) nls->password + password_length) = 0;
    
    d = (char*) nls->password;
    o = password;
    for (i = 0; i < nls->password_len; i++) {
        *d = (char) toupper(*o);
        d++;
        o++;
    }
    
    mpz_init_set_str(nls->n, NLS_VAR_N_STR, 16);
    
    gmp_randinit_default(nls->rand);
    gmp_randseed_ui(nls->rand, nls_pre_seed());
    mpz_init2(nls->a, 256);
    mpz_urandomm(nls->a, nls->rand, nls->n); /* generates the private key */

    /* The following line replaces preceding 2 lines during testing. */
    /*mpz_init_set_str(nls->a, "1234", 10);*/

	nls->A = (char*) 0;
	nls->S = (char*) 0;
	nls->K = (char*) 0;
	nls->M1 = (char*) 0;
	nls->M2 = (char*) 0;
    
    return nls;
}

MEXP(void) nls_free(nls_t* nls) {
    mpz_clear(nls->a);
    mpz_clear(nls->n);

    gmp_randclear(nls->rand);

    free((void*) nls->username);
    free((void*) nls->password);

	if (nls->A)
		free(nls->A);
	if (nls->S)
		free(nls->S);
	if (nls->K)
		free(nls->K);
	if (nls->M1)
		free(nls->M1);
	if (nls->M2)
		free(nls->M2);

    free(nls);
}

MEXP(nls_t*) nls_reinit(nls_t* nls, const char* username,
	const char* password)
{
	return nls_reinit_l(nls, username, (unsigned long) strlen(username),
		password, (unsigned long) strlen(password));
}

MEXP(nls_t*) nls_reinit_l(nls_t* nls, const char* username,
	unsigned long username_length, const char* password,
	unsigned long password_length)
{
	unsigned int i;
    char* d; /* destination */
    const char* o; /* original */

	if (nls->A)
		free(nls->A);
	if (nls->S)
		free(nls->S);
	if (nls->K)
		free(nls->K);
	if (nls->M1)
		free(nls->M1);
	if (nls->M2)
		free(nls->M2);

	nls->username_len = username_length;
    nls->password_len = password_length;
    
	nls->username = (const char*) realloc((void*) nls->username,
		nls->username_len + 1);
	if (!nls->username) {
        free(nls);
        return (nls_t*) 0;
    }
    nls->password = (const char*) realloc((void*) nls->password,
		nls->password_len + 1);
    if (!nls->password) {
		free((void*) nls->username);
        free(nls);
        return (nls_t*) 0;
    }
    
    d = (char*) nls->username;
    o = username;
    for (i = 0; i < nls->username_len; i++) {
        *d = (char) toupper(*o);
        d++;
        o++;
    }

	d = (char*) nls->password;
    o = password;
    for (i = 0; i < nls->password_len; i++) {
        *d = (char) toupper(*o);
        d++;
        o++;
    }
    
    *((char*) nls->username + username_length) = 0;
    *((char*) nls->password + password_length) = 0;

	mpz_urandomm(nls->a, nls->rand, nls->n); /* generates the private key */

	nls->A = (char*) 0;
	nls->S = (char*) 0;
	nls->K = (char*) 0;
	nls->M1 = (char*) 0;
	nls->M2 = (char*) 0;

	return nls;
}

MEXP(unsigned long) nls_account_create(nls_t* nls, char* buf, unsigned long bufSize) {
	mpz_t s; /* salt */
    mpz_t v;
    mpz_t x;

    if (!nls)
        return 0;
    if (bufSize < nls->username_len + 65)
        return 0;
    
    mpz_init2(s, 256);
    mpz_urandomb(s, nls->rand, 256);
    mpz_export(buf, (size_t*) 0, -1, 1, 0, 0, s);
	/*memset(buf, 0, 32);*/
    
    nls_get_x(nls, x, buf);
    nls_get_v_mpz(nls, v, x);
    mpz_export(buf + 32, (size_t*) 0, -1, 1, 0, 0, v);
    
	mpz_clear(x);
    mpz_clear(v);
    mpz_clear(s);
    
    strcpy(buf + 64, nls->username);
    return nls->username_len + 65;
}

MEXP(unsigned long) nls_account_logon(nls_t* nls, char* buf, unsigned long bufSize) {
    if (!nls)
        return 0;
    if (bufSize < nls->username_len + 33)
        return 0;
    
    nls_get_A(nls, buf);
    strcpy(buf + 32, nls->username);
    return nls->username_len + 33;
}

MEXP(nls_t*) nls_account_change_proof(nls_t* nls, char* buf,
    const char* new_password, const char* B, const char* salt)
{
    nls_t* nouveau;
    mpz_t s; /* salt */
    
    if (!nls)
        return (nls_t*) 0;
        
    /* create new nls_t */
    nouveau = nls_init_l(nls->username, nls->username_len, new_password,
        (unsigned long) strlen(new_password));
    if (!nouveau)
        return (nls_t*) 0;
    
    /* fill buf */
    nls_get_M1(nls, buf, B, salt);
    
    mpz_init2(s, 256);
    mpz_urandomb(s, nouveau->rand, 256);
    mpz_export(buf + 20, (size_t*) 0, -1, 1, 0, 0, s);
    
	nls_get_v(nouveau, buf + 52, buf + 20);

    mpz_clear(s);
    
    return nouveau;
}

MEXP(void) nls_get_S(nls_t* nls, char* out, const char* B, const char* salt) {
    mpz_t temp;
    mpz_t S_base, S_exp;
    mpz_t x;
    mpz_t v;
    
    if (!nls)
        return;

	if (nls->S) {
		memcpy(out, nls->S, 32);
		return;
	}
    
    mpz_init2(temp, 256);
    mpz_import(temp, 32, -1, 1, 0, 0, B);
    
    nls_get_x(nls, x, salt);
    nls_get_v_mpz(nls, v, x);
    
    mpz_init_set(S_base, nls->n);
    mpz_add(S_base, S_base, temp);
    mpz_sub(S_base, S_base, v);
    mpz_mod(S_base, S_base, nls->n);
    
    mpz_init_set(S_exp, x);
    mpz_mul_ui(S_exp, S_exp, nls_get_u(B));
    mpz_add(S_exp, S_exp, nls->a);
    
    mpz_clear(x);
    mpz_clear(v);
    mpz_clear(temp);
    
    mpz_init(temp);
    mpz_powm(temp, S_base, S_exp, nls->n);
    mpz_clear(S_base);
    mpz_clear(S_exp);
    mpz_export(out, (size_t*) 0, -1, 1, 0, 0, temp);
    mpz_clear(temp);

	nls->S = (char*) malloc(32);
	if (nls->S)
		memcpy(nls->S, out, 32);
}

MEXP(void) nls_get_v(nls_t* nls, char* out, const char* salt) {
    mpz_t g;
    mpz_t v;
    mpz_t x;
    
    if (!nls)
        return;
    
    mpz_init_set_ui(g, NLS_VAR_g);
    mpz_init(v);
    nls_get_x(nls, x, salt);
    
    mpz_powm(v, g, x, nls->n);
    
    mpz_export(out, (size_t*) 0, -1, 1, 0, 0, v);
    mpz_clear(v);
    mpz_clear(g);
    mpz_clear(x);
}


MEXP(void) nls_get_A(nls_t* nls, char* out) {
    mpz_t g;
    mpz_t A;
    size_t o;
    
    if (!nls)
        return;

	if (nls->A) {
		memcpy(out, nls->A, 32);
		return;
	}
    
    mpz_init_set_ui(g, NLS_VAR_g);
    mpz_init2(A, 256);
    
    mpz_powm(A, g, nls->a, nls->n);
    mpz_export(out, &o, -1, 1, 0, 0, A);
    
    mpz_clear(A);
    mpz_clear(g);

	nls->A = (char*) malloc(32);
	if (nls->A)
		memcpy(nls->A, out, 32);
}


MEXP(void) nls_get_K(nls_t* nls, char* out, const char* S) {
    char odd[16], even[16];
    uint8_t odd_hash[20], even_hash[20];
    
    char* Sp = (char*) S;
    char* op = odd;
    char* ep = even;
    unsigned int i;
    
    SHA1Context ctx;

	if (!nls)
		return;

	if (nls->K) {
		memcpy(out, nls->K, 40);
		return;
	}
    
    for (i = 0; i < 16; i++) {
        *(op++) = *(Sp++);
        *(ep++) = *(Sp++);
    }
    
    SHA1Reset(&ctx);
    SHA1Input(&ctx, (uint8_t*) odd, 16);
    SHA1Result(&ctx, odd_hash);
    
    SHA1Reset(&ctx);
    SHA1Input(&ctx, (uint8_t*) even, 16);
    SHA1Result(&ctx, even_hash);
    
    Sp = out;
    op = (char*) odd_hash;
    ep = (char*) even_hash;
    for (i = 0; i < 20; i++) {
        *(Sp++) = *(op++);
        *(Sp++) = *(ep++);
    }

	nls->K = (char*) malloc(40);
	if (nls->K)
		memcpy(nls->K, out, 40);
}

MEXP(void) nls_get_M1(nls_t* nls, char* out, const char* B, const char* salt) {
    SHA1Context sha;
    uint8_t username_hash[20];
    char A[32];
    char S[32];
    char K[40];

	if (!nls)
		return;

	if (nls->M1) {
		nls_dbg("nls_get_M1(): Using cached M[1] value.");
		memcpy(out, nls->M1, 20);
		return;
	}

    /* calculate SHA-1 hash of username */
    SHA1Reset(&sha);
    SHA1Input(&sha, (uint8_t*) nls->username, nls->username_len);
    SHA1Result(&sha, username_hash);

    
    nls_get_A(nls, A);
    nls_get_S(nls, S, B, salt);
    nls_get_K(nls, K, S);

    /* calculate M[1] */
    SHA1Reset(&sha);
    SHA1Input(&sha, (uint8_t*) bncsutil_NLS_I, 20);
    SHA1Input(&sha, username_hash, 20);
    SHA1Input(&sha, (uint8_t*) salt, 32);
    SHA1Input(&sha, (uint8_t*) A, 32);
    SHA1Input(&sha, (uint8_t*) B, 32);
    SHA1Input(&sha, (uint8_t*) K, 40);
    SHA1Result(&sha, (uint8_t*) out);

	nls->M1 = (char*) malloc(20);
	if (nls->M1)
		memcpy(nls->M1, out, 20);
}

MEXP(int) nls_check_M2(nls_t* nls, const char* var_M2, const char* B,
    const char* salt)
{
    SHA1Context sha;
    char local_M2[20];
	char* A;
	char S[32];
	char* K;
	char* M1;
    uint8_t username_hash[20];
	int res;
	int mustFree = 0;

	if (!nls)
		return 0;

	if (nls->M2)
		return (memcmp(nls->M2, var_M2, 20) == 0);

	if (nls->A && nls->K && nls->M1) {
		A = nls->A;
		K = nls->K;
		M1 = nls->M1;
	} else {
		if (!B || !salt)
			return 0;

		A = (char*) malloc(32);
		if (!A)
			return 0;
		K = (char*) malloc(40);
		if (!K) {
			free(A);
			return 0;
		}
		M1 = (char*) malloc(20);
		if (!M1) {
			free(K);
			free(A);
			return 0;
		}

		mustFree = 1;

		/* get the other values needed for the hash */
		nls_get_A(nls, A);
		nls_get_S(nls, S, (char*) B, (char*) salt);
		nls_get_K(nls, K, S);

		/* calculate SHA-1 hash of username */
		SHA1Reset(&sha);
		SHA1Input(&sha, (uint8_t*) nls->username, nls->username_len);
		SHA1Result(&sha, username_hash);
	    
		/* calculate M[1] */
		SHA1Reset(&sha);
		SHA1Input(&sha, (uint8_t*) bncsutil_NLS_I, 20);
		SHA1Input(&sha, username_hash, 20);
		SHA1Input(&sha, (uint8_t*) salt, 32);
		SHA1Input(&sha, (uint8_t*) A, 32);
		SHA1Input(&sha, (uint8_t*) B, 32);
		SHA1Input(&sha, (uint8_t*) K, 40);
		SHA1Result(&sha, (uint8_t*) M1);
	}
    
    /* calculate M[2] */
    SHA1Reset(&sha);
    SHA1Input(&sha, (uint8_t*) A, 32);
    SHA1Input(&sha, (uint8_t*) M1, 20);
    SHA1Input(&sha, (uint8_t*) K, 40);
    SHA1Result(&sha, (uint8_t*) local_M2);

	res = (memcmp(local_M2, var_M2, 20) == 0);

	if (mustFree) {
		free(A);
		free(K);
		free(M1);
	}

	/* cache result */
	nls->M2 = (char*) malloc(20);
	if (nls->M2)
		memcpy(nls->M2, local_M2, 20);
    
    return res;
}

MEXP(int) nls_check_signature(uint32_t address, const char* signature_raw) {
    char* result_raw;
    char check[32];
    mpz_t result;
    mpz_t modulus;
    mpz_t signature;
	size_t size, alloc_size;
	int cmp_result;
    
    /* build the "check" array */
	memcpy(check, &address, 4);
	memset(check + 4, 0xBB, 28);
    
    /* initialize the modulus */
    mpz_init2(modulus, 1024);
    mpz_import(modulus, 128, -1, 1, 0, 0, bncsutil_NLS_sig_n);
    
    /* initialize the server signature */
    mpz_init2(signature, 1024);
    mpz_import(signature, 128, -1, 1, 0, 0, signature_raw);
    
    /* initialize the result */
    mpz_init2(result, 1024);
    
    /* calculate the result */
    mpz_powm_ui(result, signature, NLS_SIGNATURE_KEY, modulus);
    
    /* clear (free) the intermediates */
    mpz_clear(signature);
    mpz_clear(modulus);

	/* allocate space for raw signature */
	alloc_size = mpz_size(result) * sizeof(mp_limb_t);
	result_raw = (char*) malloc(alloc_size);
	if (!result_raw) {
		mpz_clear(result);
		return 0;
	}
    
    /* get a byte array of the signature */
    mpz_export(result_raw, &size, -1, 1, 0, 0, result);
    
    /* clear (free) the result */
    mpz_clear(result);
    
    /* check the result */
    cmp_result = (memcmp(result_raw, check, 32) == 0);

	/* free the result_raw buffer */
	free(result_raw);

	/* return */
	return cmp_result;
}
    
unsigned long nls_pre_seed() {
#ifdef MOS_WINDOWS
    return (unsigned long) GetTickCount();
#else
    FILE* f;
    unsigned long r;
    /* try to get data from /dev/random or /dev/urandom */
    f = fopen("/dev/urandom", "r");
    if (!f) {
        f = fopen("/dev/random", "r");
        if (!f) {
            srand(time(NULL));
            return (unsigned long) rand();
        }
    }
    if (fread(&r, sizeof(unsigned long), 1, f) != 1) {
        fclose(f);
        srand(time(NULL));
        return (unsigned long) rand();
    }
    fclose(f);
    return r;
#endif
}

void nls_get_x(nls_t* nls, mpz_t x_c, const char* raw_salt) {
    char* userpass;
    uint8_t hash[20], final_hash[20];
    SHA1Context shac;
    
    // build the string Username:Password
    userpass = (char*) malloc(nls->username_len + nls->password_len + 2);
    memcpy(userpass, nls->username, nls->username_len);
    userpass[nls->username_len] = ':';
    memcpy(userpass + nls->username_len + 1, nls->password, nls->password_len);
    userpass[nls->username_len + nls->password_len + 1] = 0; // null-terminator
    
    // get the SHA-1 hash of the string
    SHA1Reset(&shac);
    SHA1Input(&shac, (uint8_t*) userpass,
        (nls->username_len + nls->password_len + 1));
    SHA1Result(&shac, hash);
    free(userpass);
    
    // get the SHA-1 hash of the salt and user:pass hash
    SHA1Reset(&shac);
    SHA1Input(&shac, (uint8_t*) raw_salt, 32);
    SHA1Input(&shac, hash, 20);
    SHA1Result(&shac, final_hash);
    SHA1Reset(&shac);
    
    // create an arbitrary-length integer from the hash and return it
    mpz_init2(x_c, 160);
    mpz_import(x_c, 20, -1, 1, 0, 0, (char*) final_hash);
}

void nls_get_v_mpz(nls_t* nls, mpz_t v, mpz_t x) {
    mpz_t g;
    mpz_init_set_ui(g, NLS_VAR_g);
    mpz_init(v);
    mpz_powm(v, g, x, nls->n);
    mpz_clear(g);
}

uint32_t nls_get_u(const char* B) {
    SHA1Context sha;
    uint8_t hash[20];
    uint32_t u;
    
    SHA1Reset(&sha);
    SHA1Input(&sha, (uint8_t*) B, 32);
    SHA1Result(&sha, hash);
    SHA1Reset(&sha);
    
    u = *(uint32_t*) hash;
    u = MSB4(u); // needed? yes
    return u;
}
