/*
 * $Id: crypt.c,v 1.3 2003-06-11 07:49:52 srittau Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * Copyright (C) 2003 Sebastian Rittau (srittau@debian.org)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_GCRYPT
#include <gcrypt.h>
#define DES_KEY_SZ 8
#else /* HAVE_GCRYPT */
#include <des.h>
#endif /* HAVE_GCRYPT */

#include <atalk/afp.h>

#include "crypt.h"

/* Cannot perform in-place operation. dstlen must be at least srclen*2. */
void atalk_hexify(u_int8_t *dst, size_t dstlen, const u_int8_t *src, size_t srclen)
{
	static const unsigned char hextable[] = "0123456789ABCDEF";

	assert(dstlen >= srclen * 2);

	for (; srclen > 0; dst += 2, src++, dstlen -= 2, srclen--) {
		dst[0] = hextable[(*src & 0xF0) >> 4];
		dst[1] = hextable[(*src & 0x0F)];
	}

	memset(dst, 0, dstlen);
}

/* Can perform in-place operation. dstlen must be at least srclen/2. */
#define unhex(x)  (isdigit(x) ? (x) - '0' : toupper(x) + 10 - 'A')
void atalk_unhexify(u_int8_t *dst, size_t dstlen, const u_int8_t *src, size_t srclen)
{
	assert(srclen % 2 == 0);
	assert(dstlen >= srclen / 2);

	for (; srclen > 0; dst++, src += 2, dstlen--, srclen -= 2)
		*dst = (unhex(src[0]) << 4) | unhex(src[1]);

	memset(dst, 0, dstlen);
}

int atalk_encrypt_start(CryptHandle *handle, u_int8_t *key)
{
#ifdef HAVE_GCRYPT
	GcryCipherHd hd;

	hd = gcry_cipher_open(GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
	if (!hd)
		return AFPERR_PARAM;
	gcry_cipher_setkey(hd, key, DES_KEY_SZ);

	*handle = hd;

	return AFP_OK;
#else /* HAVE_GCRYPT */
	DES_key_schedule *sched = malloc(sizeof(DES_key_schedule));
	DES_set_key_unchecked((DES_cblock *) key, sched);

	*handle = sched;

	return AFP_OK;
#endif /* HAVE_GCRYPT */
}

int atalk_encrypt_do(CryptHandle handle, u_int8_t *dst, u_int8_t *src)
{
#ifdef HAVE_GCRYPT
	int ret;

	ret = gcry_cipher_encrypt(*(GcryCipherHd *) handle,
				  dst, DES_KEY_SZ, src, DES_KEY_SZ);

	return ret == 0 ? AFP_OK : AFPERR_PARAM;
#else /* HAVE_GCRYPT */
	DES_ecb_encrypt((DES_cblock *) src, (DES_cblock *) dst,
			(DES_key_schedule *) handle, DES_ENCRYPT);

	return AFP_OK;
#endif /* HAVE_GCRYPT */
}

void atalk_encrypt_end(CryptHandle handle)
{
#ifdef HAVE_GCRYPT
	gcry_cipher_close(*(GcryCipherHd *) handle);
#else /* HAVE_GCRYPT */
	memset(handle, 0, sizeof(DES_key_schedule));
	free(handle);
#endif /* HAVE_GCRYPT */
}

int atalk_encrypt(u_int8_t *key, u_int8_t *dst, u_int8_t *src)
{
#ifdef HAVE_GCRYPT
	GcryCipherHd hd;
	int ret;
                                                                                
	hd = gcry_cipher_open(GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
	if (!hd)
		return AFPERR_PARAM;
	gcry_cipher_setkey(hd, key, DES_KEY_SZ);

	ret = gcry_cipher_encrypt(hd, dst, DES_KEY_SZ, src, DES_KEY_SZ);

	gcry_cipher_close(hd);

	return ret == 0 ? AFP_OK : AFPERR_PARAM;
#else /* HAVE_GCRYPT */
	DES_key_schedule sched;

	DES_set_key_unchecked((DES_cblock *) key, &sched);
	DES_ecb_encrypt((DES_cblock *) src, (DES_cblock *) dst,
			&sched, DES_ENCRYPT);

	memset(&sched, 0, sizeof(sched));

	return AFP_OK;
#endif /* HAVE_GCRYPT */
}

int atalk_decrypt(u_int8_t *key, u_int8_t *dst, u_int8_t *src)
{
#ifdef HAVE_GCRYPT
	GcryCipherHd hd;
	int ret;

	hd = gcry_cipher_open(GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
	if (!hd)
		return AFPERR_PARAM;
	gcry_cipher_setkey(hd, key, DES_KEY_SZ);

	ret = gcry_cipher_encrypt(hd, dst, DES_KEY_SZ, src, DES_KEY_SZ);

	gcry_cipher_close(hd);

	return ret == 0 ? AFP_OK : AFPERR_PARAM;
#else /* HAVE_GCRYPT */
	DES_key_schedule sched;

	DES_set_key_unchecked((DES_cblock *) key, &sched);
	DES_ecb_encrypt((DES_cblock *) src, (DES_cblock *) dst,
			&sched, DES_ENCRYPT);

	memset(&sched, 0, sizeof(sched));

	return AFP_OK;
#endif /* HAVE_GCRYPT */
}
