#ifndef __UAMS_CRYPT_H
#define __UAMS_CRYPT_H

/*
 * $Id: crypt.h,v 1.2 2003-06-11 07:14:12 srittau Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <sys/types.h>

typedef void *CryptHandle;

void atalk_hexify(u_int8_t *dst, size_t dstlen, const u_int8_t *src, size_t srclen);
void atalk_unhexify(u_int8_t *dst, size_t dstlen, const u_int8_t *src, size_t srclen);

int atalk_encrypt_start(CryptHandle *handle, u_int8_t *key);
int atalk_encrypt_do(CryptHandle handle, u_int8_t *dst, u_int8_t *src);
void atalk_encrypt_end(CryptHandle handle);

int atalk_encrypt(u_int8_t *key, u_int8_t *dst, u_int8_t *src);
int atalk_decrypt(u_int8_t *key, u_int8_t *dst, u_int8_t *src);

#endif /* __UAMS_CRYPT_H */
