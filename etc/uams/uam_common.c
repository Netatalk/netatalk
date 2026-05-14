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

/*!
 * @file
 * Shared helpers for UAM authentication modules.
 *
 * These helpers exist to consolidate code that has historically been
 * copy-pasted between uams_dhx*, uams_srp, and uams_*_pam.  Crypto
 * primitives, AFP login-message parsing, and PAM conversation glue
 * all benefit from a single audited implementation rather than a
 * per-module copy that can drift during hardening work.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <gcrypt.h>

#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/uam.h>

#include "uam_common.h"


/* -------------------------------------------------------------------- */
/*  MPI / big-integer helpers                                           */
/* -------------------------------------------------------------------- */

/*!
 * @brief Serialize an MPI as a fixed-width big-endian byte string,
 *        left-padded with zeros.
 *
 * Replaces the gcry_mpi_print + memmove/memset dance copied across
 * every DH-based UAM.  Always writes exactly @p nbytes into @p buf.
 *
 * @param[out] buf    Output buffer; must be at least @p nbytes.
 * @param[in]  nbytes Target width (caller-defined wire-format size).
 * @param[in]  m      MPI to serialize.
 */
void uam_mpi_to_padded_buf(unsigned char *buf, size_t nbytes, gcry_mpi_t m)
{
    size_t nwritten = 0;
    memset(buf, 0, nbytes);
    gcry_mpi_print(GCRYMPI_FMT_USG, buf, nbytes, &nwritten, m);

    if (nwritten < nbytes) {
        memmove(buf + nbytes - nwritten, buf, nwritten);
        memset(buf, 0, nbytes - nwritten);
    }
}

/*!
 * @brief Release an MPI and null the handle.
 *
 * Safe to call with @p *m == NULL.  After return, @p *m is NULL so
 * cleanup paths can be idempotent without double-free risk.
 */
void uam_mpi_release(gcry_mpi_t *m)
{
    if (m == NULL) {
        return;
    }

    if (*m != NULL) {
        gcry_mpi_release(*m);
        *m = NULL;
    }
}

/*!
 * @brief Constant-time memory compare.
 *
 * Lifted from uams_srp.c so DHX/DHX2 nonce checks can stop using
 * plain memcmp() (a timing oracle for replay/forgery attempts).
 *
 * @returns 0 if the buffers are equal, non-zero otherwise.
 */
int uam_ct_memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *pa = a;
    const unsigned char *pb = b;
    /* volatile prevents the compiler from short-circuiting the loop
     * and leaking timing information. */
    volatile unsigned char diff = 0;

    while (n--) {
        diff |= *pa++ ^ *pb++;
    }

    return diff != 0;
}


/* -------------------------------------------------------------------- */
/*  Diffie-Hellman parameter generation (DHX2)                          */
/* -------------------------------------------------------------------- */

/*!
 * @brief Generate a fresh DH prime p and generator g.
 *
 * One authoritative implementation, replacing the two slightly
 * divergent dh_params_generate() copies in uams_dhx2_passwd.c and
 * uams_dhx2_pam.c.  Caller owns the returned MPIs and must release
 * them with gcry_mpi_release / uam_mpi_release.
 *
 * Algorithm taken from GNUTLS:gnutls_dh_primes.c.
 *
 * @param[out] out_p Receives the prime.  Must not be NULL.
 * @param[out] out_g Receives the generator.  Must not be NULL.
 * @param[in]  bits  Prime bit length (e.g. 1024).
 * @returns 0 on success, a non-zero AFPERR_* code on failure.
 */
int uam_dh_params_generate(gcry_mpi_t *out_p, gcry_mpi_t *out_g,
                           unsigned int bits)
{
    int result = AFPERR_MISC;
    int times = 0, qbits;
    gcry_mpi_t prime = NULL, g = NULL;
    gcry_mpi_t *factors = NULL;
    gcry_error_t err;

    if (out_p == NULL || out_g == NULL) {
        return AFPERR_MISC;
    }

    *out_p = NULL;
    *out_g = NULL;

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        LOG(log_error, logtype_uams,
            "uam_dh_params_generate: libgcrypt version mismatch. "
            "Needs: %s Has: %s",
            UAM_NEED_LIBGCRYPT_VERSION, gcry_check_version(NULL));
        return AFPERR_MISC;
    }

    if (bits < 256) {
        qbits = bits / 2;
    } else {
        qbits = (bits / 40) + 105;
    }

    if (qbits & 1) {
        qbits++;
    }

    do {
        if (times) {
            gcry_mpi_release(prime);
            prime = NULL;
            gcry_prime_release_factors(factors);
            factors = NULL;
        }

        err = gcry_prime_generate(&prime, bits, qbits, &factors, NULL, NULL,
                                  GCRY_STRONG_RANDOM,
                                  GCRY_PRIME_FLAG_SPECIAL_FACTOR);

        if (err != 0) {
            goto error;
        }

        err = gcry_prime_check(prime, 0);
        times++;
    } while (err != 0 && times < 10);

    if (err != 0) {
        goto error;
    }

    err = gcry_prime_group_generator(&g, prime, factors, NULL);

    if (err != 0) {
        goto error;
    }

    gcry_prime_release_factors(factors);
    *out_p = prime;
    *out_g = g;
    return 0;
error:
    gcry_prime_release_factors(factors);
    gcry_mpi_release(g);
    gcry_mpi_release(prime);
    return result;
}


/* -------------------------------------------------------------------- */
/*  CAST5-CBC one-shot helpers                                          */
/* -------------------------------------------------------------------- */

/*!
 * @brief Internal helper: open/setkey/setiv/encrypt-or-decrypt/close.
 *
 * Single-exit error handling ensures the cipher context is always
 * closed on failure paths, eliminating the ctx-leak class of bug that
 * the current scattered cipher sequences are vulnerable to.
 *
 * Pass @p in == NULL (and @p inlen == 0) for in-place operation on
 * @p out.  Otherwise libgcrypt reads from @p in and writes to @p out.
 */
static int uam_cast5_cbc_op(const unsigned char *key, size_t keylen,
                            const unsigned char *iv, size_t ivlen,
                            unsigned char *out, size_t outlen,
                            const unsigned char *in, size_t inlen,
                            int encrypt)
{
    gcry_cipher_hd_t ctx = NULL;
    gcry_error_t err;
    int rc = -1;
    err = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
                           GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(err) != GPG_ERR_NO_ERROR) {
        goto out;
    }

    err = gcry_cipher_setkey(ctx, key, keylen);

    if (gcry_err_code(err) != GPG_ERR_NO_ERROR) {
        goto out;
    }

    err = gcry_cipher_setiv(ctx, iv, ivlen);

    if (gcry_err_code(err) != GPG_ERR_NO_ERROR) {
        goto out;
    }

    if (encrypt) {
        err = gcry_cipher_encrypt(ctx, out, outlen, in, inlen);
    } else {
        err = gcry_cipher_decrypt(ctx, out, outlen, in, inlen);
    }

    if (gcry_err_code(err) != GPG_ERR_NO_ERROR) {
        goto out;
    }

    rc = 0;
out:

    if (ctx != NULL) {
        gcry_cipher_close(ctx);
    }

    return rc;
}

/*!
 * @brief One-shot CAST5-CBC encrypt.
 *
 * For in-place encryption, pass @p in == NULL and @p inlen == 0.
 *
 * @param[in]     key    Key bytes.
 * @param[in]     keylen Key length (16 for DHX, 16 for DHX2-MD5-derived).
 * @param[in]     iv     IV bytes.
 * @param[in]     ivlen  IV length.
 * @param[in,out] out    Output buffer (also input when in-place).
 * @param[in]     outlen Output length (must be a multiple of 8).
 * @param[in]     in     Optional input buffer; NULL for in-place.
 * @param[in]     inlen  Input length; 0 for in-place.
 * @returns 0 on success, -1 on any cipher error.
 */
int uam_cast5_cbc_encrypt(const unsigned char *key, size_t keylen,
                          const unsigned char *iv, size_t ivlen,
                          unsigned char *out, size_t outlen,
                          const unsigned char *in, size_t inlen)
{
    return uam_cast5_cbc_op(key, keylen, iv, ivlen,
                            out, outlen, in, inlen, 1);
}

/*! @brief One-shot CAST5-CBC decrypt; mirror of uam_cast5_cbc_encrypt(). */
int uam_cast5_cbc_decrypt(const unsigned char *key, size_t keylen,
                          const unsigned char *iv, size_t ivlen,
                          unsigned char *out, size_t outlen,
                          const unsigned char *in, size_t inlen)
{
    return uam_cast5_cbc_op(key, keylen, iv, ivlen,
                            out, outlen, in, inlen, 0);
}


/* -------------------------------------------------------------------- */
/*  AFP login username extraction                                       */
/* -------------------------------------------------------------------- */

/*!
 * @brief Parse a v1 (1-byte length prefix) AFP login username.
 *
 * On success, copies the username into @p dst NUL-terminated, and
 * advances @p *ibuf / @p *ibuflen past the username and the optional
 * pad byte that aligns subsequent reads to an even pointer.
 *
 * Performs full bounds checking: length must be non-zero, must fit
 * in remaining ibuflen, and must fit in dstlen.  This is the stricter
 * of the two patterns currently in the tree; the weaker form (e.g.
 * uams_dhx2_passwd.c passwd_logincont) is hardened by adopting this.
 *
 * @returns 0 on success, -1 on malformed input.
 */
int uam_extract_username_v1(char **ibuf, size_t *ibuflen,
                            char *dst, size_t dstlen)
{
    char *p;
    size_t remaining, len;

    if (ibuf == NULL || ibuflen == NULL || dst == NULL || dstlen == 0) {
        return -1;
    }

    p = *ibuf;
    remaining = *ibuflen;

    if (remaining < 1) {
        return -1;
    }

    len = (unsigned char) * p++;
    remaining--;

    /* dst needs room for NUL */
    if (len == 0 || len > remaining || len >= dstlen) {
        return -1;
    }

    memcpy(dst, p, len);
    dst[len] = '\0';
    p += len;
    remaining -= len;

    /* AFP pads to an even pointer offset.  The pad byte is only
     * consumed when there is more data after it — packets that end
     * right after the username (DHX/DHX2/randnum v1 AFPLogin) may
     * be exactly at the buffer end with an odd pointer; that is a
     * valid terminus, not a malformed packet. */
    if ((((uintptr_t) p) & 1) && remaining > 0) {
        p++;
        remaining--;
    }

    *ibuf = p;
    *ibuflen = remaining;
    return 0;
}

/*!
 * @brief Parse a v2/ext-login (type=3, 2-byte BE length) username.
 *
 * Wire format: 1 byte type (must be 3 for UTF-8), 2 bytes big-endian
 * length, then the raw bytes.  Callers of the ext-login path do not
 * currently advance ibuf, so this helper takes the username buffer
 * directly and does not return a new offset.
 *
 * NOTE: callers must ensure that @p uname points to at least 3 bytes
 * of validated input before calling.  Callers that have access to
 * the surrounding ibuflen should validate it before calling here;
 * the helper has no way to bounds-check the input pointer.
 *
 * @returns 0 on success, -1 on malformed input.
 */
int uam_extract_username_v2(const char *uname, char *dst, size_t dstlen)
{
    uint16_t len_be;
    size_t len;

    if (uname == NULL || dst == NULL || dstlen == 0) {
        return -1;
    }

    if (*uname != 3) {
        return -1;
    }

    uname++;
    memcpy(&len_be, uname, sizeof(len_be));
    len = ntohs(len_be);

    if (len == 0 || len >= dstlen) {
        return -1;
    }

    memcpy(dst, uname + 2, len);
    dst[len] = '\0';
    return 0;
}


/* -------------------------------------------------------------------- */
/*  PAM conversation helper                                             */
/* -------------------------------------------------------------------- */

#ifdef UAM_COMMON_USE_PAM

/*!
 * @brief Generic PAM conversation function.
 *
 * Replaces the three near-identical PAM_conv() copies in uams_pam.c,
 * uams_dhx_pam.c, and uams_dhx2_pam.c.  The username and password
 * are taken from the @c struct @c uam_pam_ctx passed via PAM's
 * appdata_ptr rather than from module-local globals.
 *
 * Wire into PAM via:
 * @code
 *   struct uam_pam_ctx ctx = { user, pass, "uams_dhx2_pam" };
 *   struct pam_conv conv = { uam_pam_conv, &ctx };
 *   pam_start(svc, user, &conv, &pamh);
 * @endcode
 */
int uam_pam_conv(int num_msg,
#ifdef HAVE_PAM_CONV_CONST_PAM_MESSAGE
                 const struct pam_message **msg,
#else
                 struct pam_message **msg,
#endif
                 struct pam_response **resp,
                 void *appdata_ptr)
{
    /* ctx is mutable: old_password is a one-shot that we clear on use. */
    struct uam_pam_ctx *ctx = appdata_ptr;
    const char *tag = (ctx && ctx->log_tag) ? ctx->log_tag : "uam_pam_conv";
    struct pam_response *reply;
    int i;

    if (num_msg < 1 || ctx == NULL) {
        LOG(log_info, logtype_uams, "%s: PAM conversation: bad arguments",
            tag);
        return PAM_CONV_ERR;
    }

    reply = calloc(num_msg, sizeof(struct pam_response));

    if (reply == NULL) {
        LOG(log_info, logtype_uams, "%s: PAM conversation: calloc -- %s",
            tag, strerror(errno));
        return PAM_CONV_ERR;
    }

    for (i = 0; i < num_msg; i++) {
        char *string = NULL;

        switch (msg[i]->msg_style) {
        case PAM_PROMPT_ECHO_ON:
            if (ctx->username == NULL) {
                LOG(log_info, logtype_uams,
                    "%s: PAM asked for username but none provided", tag);
                goto fail;
            }

            string = strdup(ctx->username);

            if (string == NULL) {
                goto fail;
            }

            break;

        case PAM_PROMPT_ECHO_OFF: {
            const char *src;

            /* one-shot old_password takes precedence on the first prompt */
            if (ctx->old_password != NULL) {
                src = ctx->old_password;
                ctx->old_password = NULL;
            } else {
                src = ctx->password;
            }

            if (src == NULL) {
                LOG(log_info, logtype_uams,
                    "%s: PAM asked for password but none provided", tag);
                goto fail;
            }

            string = strdup(src);

            if (string == NULL) {
                goto fail;
            }

            break;
        }

        case PAM_TEXT_INFO:
#ifdef PAM_BINARY_PROMPT
        case PAM_BINARY_PROMPT:
#endif
            /* informational; no answer needed */
            break;

        case PAM_ERROR_MSG:
        default:
            LOG(log_info, logtype_uams,
                "%s: unexpected PAM message style %d",
                tag, msg[i]->msg_style);
            goto fail;
        }

        if (string != NULL) {
            reply[i].resp_retcode = 0;
            reply[i].resp = string;
        }
    }

    *resp = reply;
    return PAM_SUCCESS;
fail:

    for (i = 0; i < num_msg; i++) {
        if (reply[i].resp == NULL) {
            continue;
        }

        switch (msg[i]->msg_style) {
        case PAM_PROMPT_ECHO_OFF:
            /* Clear password reply before freeing. */
            memset(reply[i].resp, 0, strlen(reply[i].resp));

        /* fallthrough */
        case PAM_PROMPT_ECHO_ON:
            free(reply[i].resp);
            reply[i].resp = NULL;
            break;
        }
    }

    free(reply);
    return PAM_CONV_ERR;
}

#endif /* UAM_COMMON_USE_PAM */
