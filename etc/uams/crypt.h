#ifndef __UAMS_CRYPT_H
#define __UAMS_CRYPT_H

/*
 * $Id: crypt.h,v 1.1 2003-06-11 06:29:30 srittau Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <sys/types.h>

typedef void *CryptHandle;

void hexify(u_int8_t *dst, size_t dstlen, const u_int8_t *src, size_t srclen);
void unhexify(u_int8_t *dst, size_t dstlen, const u_int8_t *src, size_t srclen);

int encrypt_start(CryptHandle *handle, u_int8_t *key);
int encrypt_do(CryptHandle handle, u_int8_t *dst, u_int8_t *src);
void encrypt_end(CryptHandle handle);

int encrypt(u_int8_t *key, u_int8_t *dst, u_int8_t *src);
int decrypt(u_int8_t *key, u_int8_t *dst, u_int8_t *src);

#endif /* __UAMS_CRYPT_H */
