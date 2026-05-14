/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef UAM_COMMON_H
#define UAM_COMMON_H

#include <stddef.h>
#include <stdint.h>

#include <gcrypt.h>

void uam_mpi_to_padded_buf(unsigned char *buf, size_t nbytes, gcry_mpi_t m);
void uam_mpi_release(gcry_mpi_t *m);
int uam_ct_memcmp(const void *a, const void *b, size_t n);

int uam_dh_params_generate(gcry_mpi_t *out_p, gcry_mpi_t *out_g,
                           unsigned int bits);

int uam_cast5_cbc_encrypt(const unsigned char *key, size_t keylen,
                          const unsigned char *iv, size_t ivlen,
                          unsigned char *out, size_t outlen,
                          const unsigned char *in, size_t inlen);
int uam_cast5_cbc_decrypt(const unsigned char *key, size_t keylen,
                          const unsigned char *iv, size_t ivlen,
                          unsigned char *out, size_t outlen,
                          const unsigned char *in, size_t inlen);

int uam_extract_username_v1(char **ibuf, size_t *ibuflen,
                            char *dst, size_t dstlen);
int uam_extract_username_v2(const char *uname, char *dst, size_t dstlen);

#ifdef UAM_COMMON_USE_PAM

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif

struct uam_pam_ctx {
    const char *username;
    const char *password;
    /* If non-NULL, the conv answers the FIRST PAM_PROMPT_ECHO_OFF with
     * old_password and clears the field (so the next ECHO_OFF returns
     * password).  Used by pam_chauthtok when running non-root, where
     * pam_unix asks for the current password first then the new one. */
    const char *old_password;
    const char *log_tag;
};

int uam_pam_conv(int num_msg,
#ifdef HAVE_PAM_CONV_CONST_PAM_MESSAGE
                 const struct pam_message **msg,
#else
                 struct pam_message **msg,
#endif
                 struct pam_response **resp,
                 void *appdata_ptr);

#endif /* UAM_COMMON_USE_PAM */

#endif /* UAM_COMMON_H */
