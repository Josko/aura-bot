/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * New Logon System (SRP) Interface
 * November 26, 2004
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

#ifndef BNCSUTIL_NLS_H_INCLUDED
#define BNCSUTIL_NLS_H_INCLUDED

#include <bncsutil/mutil.h>

// modulus ("N") in base-16
#define NLS_VAR_N_STR \
    "F8FF1A8B619918032186B68CA092B5557E976C78C73212D91216F6658523C787"
// generator var ("g")
#define NLS_VAR_g 0x2F
// SHA1(g) ^ SHA1(N) ("I")
#define NLS_VAR_I_STR "8018CF0A425BA8BEB8958B1AB6BF90AED970E6C"
// Server Signature Key
#define NLS_SIGNATURE_KEY 0x10001

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Data type used by NLS functions.
 */
typedef struct _nls nls_t;

/**
 * Allocates and initializes an nls_t structure.
 * Returns a NULL pointer on failure.
 */
MEXP(nls_t*) nls_init(const char* username, const char* password);

/**
 * Allocates and initializes an nls_t structure, using the given string lengths.
 * Returns a NULL pointer on failure.
 * (Lengths do not include the null-terminator.)
 */
MEXP(nls_t*) nls_init_l(const char* username, unsigned long username_length,
    const char* password, unsigned long password_length);

/**
 * Frees an nls_t structure.
 */
MEXP(void) nls_free(nls_t* nls);

/**
 * Re-initializes an nls_t structure with a new username and
 * password.  Returns the nls argument on success or a NULL
 * pointer on failure.
 */
MEXP(nls_t*) nls_reinit(nls_t* nls, const char* username,
	const char* password);

/**
 * Re-initializes an nls_t structure with a new username and
 * password and their given lengths.  Returns the nls argument
 * on success or a NULL pointer on failure.
 */
MEXP(nls_t*) nls_reinit_l(nls_t* nls, const char* username,
	unsigned long username_length, const char* password,
	unsigned long password_length);

/* Packet Generation Functions */

/**
 * Builds the content of an SID_AUTH_ACCOUNTCREATE packet.
 * buf should be at least strlen(username) + 65 bytes long.
 * bufSize should be the size of buf.
 * Returns the number of bytes placed in buf, or 0 on error.
 */
MEXP(unsigned long) nls_account_create(nls_t* nls, char* buf, unsigned long bufSize);

/**
 * [ DEPRECATED ] Uses the internal username buffer when
 *                generating the packet, which is in all
 *                upper-case.
 *
 * Builds the content of an SID_AUTH_ACCOUNTLOGON packet.
 * buf should be at least strlen(username) + 33 bytes long.
 * bufSize should be size of buf.
 * Returns the number of bytes placed in buf, or 0 on error.
 */
MEXP(unsigned long) nls_account_logon(nls_t* nls, char* buf, unsigned long bufSize);

/**
 * NOTE
 * There is no nls_account_logon_proof function, as it'd be the same as
 * nls_get_M1, and no nls_account_change function, as it's probably easier to
 * just assemble that packet yourself.
 */

/**
 * Builds the content of an SID_AUTH_ACCOUNTCHANGEPROOF packet and places it in
 * buf, which should be at least 84 bytes long.  The account's new password
 * should be placed in new_password.  A new nls_t* pointer will be returned
 * as if you'd called nls_init.  If you're not going to check the server
 * password proof (the response to this packet) then you can nls_free the value
 * of nls and use the returned value.  Any login operations with the new
 * password should use the returned value.  Returns a NULL pointer on error.
 */
MEXP(nls_t*) nls_account_change_proof(nls_t* nls, char* buf,
    const char* new_password, const char* B, const char* salt);
 
/* Calculation Functions */

/**
 * Gets the "secret" value (S). (32 bytes)
 */
MEXP(void) nls_get_S(nls_t* nls, char* out, const char* B, const char* salt);

/**
 * Gets the password verifier (v). (32 bytes)
 */
MEXP(void) nls_get_v(nls_t* nls, char* out, const char* salt);

/**
 * Gets the public key (A). (32 bytes)
 */
MEXP(void) nls_get_A(nls_t* nls, char* out);

/**
 * Gets "K" value, which is based on the secret (S).
 * The buffer "out" must be at least 40 bytes long.
 */
MEXP(void) nls_get_K(nls_t* nls, char* out, const char* S);

/**
 * Gets the "M[1]" value, which proves that you know your password.
 * The buffer "out" must be at least 20 bytes long.
 */
MEXP(void) nls_get_M1(nls_t* nls, char* out, const char* B, const char* salt);

/**
 * Checks the "M[2]" value, which proves that the server knows your
 * password.  Pass the M2 value in the var_M2 argument.  Returns 0
 * if the check failed, nonzero if the proof matches.  Now that
 * calculated value caching has been added, B and salt can be
 * safely set to NULL.
 */
MEXP(int) nls_check_M2(nls_t* nls, const char* var_M2, const char* B,
	const char* salt);
	
/**
 * Checks the server signature received in SID_AUTH_INFO (0x50).
 * Pass the IPv4 address of the server you're connecting to in the address
 * paramater and the 128-byte server signature in the signature_raw paramater.
 * Address paramater should be in network byte order (big-endian).
 * Returns a nonzero value if the signature matches or 0 on failure.
 * Note that this function does NOT take an nls_t* argument!
 */
MEXP(int) nls_check_signature(uint32_t address, const char* signature_raw);

#ifdef __cplusplus
} // extern "C"

#include <string>
#include <vector>
#include <utility>

/**
 * NLS convenience class.  Getter functions which return pointers return those
 * pointers from dynamically-allocated memory that will be automatically freed
 * when the NLS object is destroyed.  They should not be free()'d or deleted.
 */
class NLS
{
public:
	NLS(const char* username, const char* password) : n((nls_t*) 0)
	{
		n = nls_init(username, password);
	}
	
	NLS(const char* username, size_t username_length, const char* password,
		size_t password_length)
	{
		n = nls_init_l(username, username_length, password, password_length);
	}
	
	NLS(const std::string& username, const std::string& password)
	{
		n = nls_init_l(username.c_str(), username.length(),
			password.c_str(), password.length());
	}
	
	virtual ~NLS()
	{
		std::vector<char*>::iterator i;
		
		if (n)
			nls_free(n);
		
		for (i = blocks.begin(); i != blocks.end(); i++) {
			delete [] *i;
		}
	}
	
	void getSecret(char* out, const char* salt, const char* B)
	{
		nls_get_S(n, out, B, salt);
	}
	
	const char* getSecret(const char* salt, const char* B)
	{
		char* buf = allocateBuffer(32);
		getSecret(buf, salt, B);
		return buf;
	}
	
	void getVerifier(char* out, const char* salt)
	{
		nls_get_v(n, out, salt);
	}
	
	const char* getVerifier(const char* salt)
	{
		char* buf = allocateBuffer(32);
		getVerifier(buf, salt);
		return buf;
	}
	
	void getPublicKey(char* out)
	{
		nls_get_A(n, out);
	}
	
	const char* getPublicKey(void)
	{
		char* buf = allocateBuffer(32);
		getPublicKey(buf);
		return buf;
	}
	
	void getHashedSecret(char* out, const char* secret)
	{
		nls_get_K(n, out, secret);
	}
	
	const char* getHashedSecret(const char* secret)
	{
		char* buf = allocateBuffer(40);
		getHashedSecret(buf, secret);
		return buf;
	}
	
	void getClientSessionKey(char* out, const char* salt, const char* B)
	{
		nls_get_M1(n, out, B, salt);
	}
	
	const char* getClientSessionKey(const char* salt, const char* B)
	{
		char* buf = allocateBuffer(20);
		getClientSessionKey(buf, salt, B);
		return buf;
	}
	
	bool checkServerSessionKey(const char* key, const char* salt,
		const char* B)
	{
		return (nls_check_M2(n, key, B, salt) != 0);
	}
	
	NLS makeChangeProof(char* buf, const char* new_password, const char* salt,
		const char* B)
	{
		return NLS(nls_account_change_proof(n, buf, new_password, B, salt));
	}
	
	NLS makeChangeProof(char* buf, const std::string& new_password,
		const char* salt, const char* B)
	{
		return NLS(nls_account_change_proof(n, buf, new_password.c_str(), B,
			salt));
	}
	
	std::pair<NLS, const char*> makeChangeProof(const char* new_password,
		const char* salt, const char* B)
	{
		char* buf = allocateBuffer(84);
		NLS nls = NLS(nls_account_change_proof(n, buf, new_password, B, salt));
		return std::pair<NLS, const char*>(nls, buf);
	}
	
	std::pair<NLS, const char*> makeChangeProof(const std::string& new_password,
		const char* salt, const char* B)
	{
		return makeChangeProof(new_password.c_str(), salt, B);
	}
private:
	std::vector<char*> blocks;
	nls_t* n;
	
	NLS(nls_t* nls)
	{
		n = nls;
	}
	
	char* allocateBuffer(size_t length)
	{
		char* buf = new char[length];
		blocks.push_back(buf);
		return buf;
	}
};

#endif

#endif /* BNCSUTIL_NLS_H_INCLUDED */
