/*
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
 * SRP (Secure Remote Password) User Authentication Method for AFP.
 * Protocol: SRP-6a with SHA-1, MGF1 KDF, RFC 5054 group #2 (1536-bit).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <gcrypt.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/uam.h>

/* -------------------- Constants -------------------- */

#define SRP_INIT_MARKER   0x0001
#define SRP_CLIENT_PROOF  0x0003
#define SRP_SERVER_PROOF  0x0004

#define SRP_SHA1_LEN      20
#define SRP_SALT_LEN      16
#define SRP_GROUP_INDEX   0x0002  /* RFC 5054 group #2 */
#define SRP_NBYTES        192     /* 1536-bit prime = 192 bytes */
#define SRP_SESSION_KEY_LEN 40

#define SRP_HEX_SALT_LEN  (SRP_SALT_LEN * 2)
#define SRP_HEX_V_LEN     (SRP_NBYTES * 2)

/* Error code for authentication failure (as observed on Apple servers) */
#define SRP_AUTH_FAILURE   (-6754)

/*!
 * RFC 5054 group #2: 1536-bit MODP group.
 * N is the safe prime, g = 2.
 */
static const unsigned char srp_N_bytes[SRP_NBYTES] = {
    0x9D, 0xEF, 0x3C, 0xAF, 0xB9, 0x39, 0x27, 0x7A,
    0xB1, 0xF1, 0x2A, 0x86, 0x17, 0xA4, 0x7B, 0xBB,
    0xDB, 0xA5, 0x1D, 0xF4, 0x99, 0xAC, 0x4C, 0x80,
    0xBE, 0xEE, 0xA9, 0x61, 0x4B, 0x19, 0xCC, 0x4D,
    0x5F, 0x4F, 0x5F, 0x55, 0x6E, 0x27, 0xCB, 0xDE,
    0x51, 0xC6, 0xA9, 0x4B, 0xE4, 0x60, 0x7A, 0x29,
    0x15, 0x58, 0x90, 0x3B, 0xA0, 0xD0, 0xF8, 0x43,
    0x80, 0xB6, 0x55, 0xBB, 0x9A, 0x22, 0xE8, 0xDC,
    0xDF, 0x02, 0x8A, 0x7C, 0xEC, 0x67, 0xF0, 0xD0,
    0x81, 0x34, 0xB1, 0xC8, 0xB9, 0x79, 0x89, 0x14,
    0x9B, 0x60, 0x9E, 0x0B, 0xE3, 0xBA, 0xB6, 0x3D,
    0x47, 0x54, 0x83, 0x81, 0xDB, 0xC5, 0xB1, 0xFC,
    0x76, 0x4E, 0x3F, 0x4B, 0x53, 0xDD, 0x9D, 0xA1,
    0x15, 0x8B, 0xFD, 0x3E, 0x2B, 0x9C, 0x8C, 0xF5,
    0x6E, 0xDF, 0x01, 0x95, 0x39, 0x34, 0x96, 0x27,
    0xDB, 0x2F, 0xD5, 0x3D, 0x24, 0xB7, 0xC4, 0x86,
    0x65, 0x77, 0x2E, 0x43, 0x7D, 0x6C, 0x7F, 0x8C,
    0xE4, 0x42, 0x73, 0x4A, 0xF7, 0xCC, 0xB7, 0xAE,
    0x83, 0x7C, 0x26, 0x4A, 0xE3, 0xA9, 0xBE, 0xB8,
    0x7F, 0x8A, 0x2F, 0xE9, 0xB8, 0xB5, 0x29, 0x2E,
    0x5A, 0x02, 0x1F, 0xFF, 0x5E, 0x91, 0x47, 0x9E,
    0x8C, 0xE7, 0xA2, 0x8C, 0x24, 0x42, 0xC6, 0xF3,
    0x15, 0x18, 0x0F, 0x93, 0x49, 0x9A, 0x23, 0x4D,
    0xCF, 0x76, 0xE3, 0xFE, 0xD1, 0x35, 0xF9, 0xBB,
};

static const unsigned char srp_g_byte = 0x02;

/* -------------------- Per-session state -------------------- */

static gcry_mpi_t session_N;
static gcry_mpi_t session_g;
static gcry_mpi_t session_v;    /* verifier */
static gcry_mpi_t session_b;    /* server secret ephemeral */
static gcry_mpi_t session_B;    /* server public ephemeral */
static unsigned char session_salt[SRP_SALT_LEN];
static unsigned char session_B_buf[SRP_NBYTES]; /* B in wire format */
static char session_username[UAM_USERNAMELEN + 1];
static struct passwd *srppwd;

/* -------------------- Crypto helpers -------------------- */

/*!
 * @brief Constant-time memory comparison to avoid timing oracles.
 * @returns non-zero if the buffers differ, 0 if equal.
 */
static int ct_memcmp(const unsigned char *a, const unsigned char *b, size_t n)
{
    /* Use volatile to prevent compiler optimizations
     * that could leak timing information. */
    volatile unsigned char diff = 0;

    while (n--) {
        diff |= *a++ ^ *b++;
    }

    return diff != 0;
}

/*!
 * @brief Strip leading zero bytes from a big-endian integer buffer.
 * @returns pointer to the first non-zero byte (or last byte if all zeros).
 */
static const unsigned char *strip_leading_zeros(const unsigned char *buf,
        size_t len, size_t *out_len)
{
    while (len > 1 && *buf == 0) {
        buf++;
        len--;
    }

    *out_len = len;
    return buf;
}

/*!
 * @brief MGF1 mask generation function (PKCS#1 v2.1) with SHA-1.
 * @param[in]  seed     The seed for the mask generation.
 * @param[in]  seed_len The length of the seed.
 * @param[out] out      The output buffer.
 * @param[in]  out_len  The length of the output buffer.
 * @returns 0 on success, -1 on failure.
 */
static int mgf1_sha1(const unsigned char *seed, size_t seed_len,
                     unsigned char *out, size_t out_len)
{
    unsigned char counter_be[4];
    unsigned char hash[SRP_SHA1_LEN];
    size_t pos = 0;
    uint32_t counter = 0;

    while (pos < out_len) {
        counter_be[0] = (counter >> 24) & 0xFF;
        counter_be[1] = (counter >> 16) & 0xFF;
        counter_be[2] = (counter >> 8) & 0xFF;
        counter_be[3] = counter & 0xFF;
        gcry_md_hd_t hd;

        if (gcry_md_open(&hd, GCRY_MD_SHA1, 0) != 0) {
            return -1;
        }

        gcry_md_write(hd, seed, seed_len);
        gcry_md_write(hd, counter_be, 4);
        memcpy(hash, gcry_md_read(hd, GCRY_MD_SHA1), SRP_SHA1_LEN);
        gcry_md_close(hd);
        size_t to_copy = out_len - pos;

        if (to_copy > SRP_SHA1_LEN) {
            to_copy = SRP_SHA1_LEN;
        }

        memcpy(out + pos, hash, to_copy);
        pos += to_copy;
        counter++;
    }

    return 0;
}

/*!
 * @brief Compute SHA-1 incrementally from multiple buffers.
 *
 * Arguments after @p out are pairs of (const unsigned char *data, size_t len),
 * terminated by a NULL data pointer.
 *
 * @param[out] out 20-byte buffer for the SHA-1 digest.
 *
 * @returns 0 on success, -1 on failure.
 */
static int sha1_multi(unsigned char *out, ...)
{
    gcry_md_hd_t hd;
    va_list ap;
    gcry_error_t err = gcry_md_open(&hd, GCRY_MD_SHA1, 0);

    if (err != 0) {
        return -1;
    }

    va_start(ap, out);

    for (;;) {
        const unsigned char *data = va_arg(ap, const unsigned char *);

        if (data == NULL) {
            break;
        }

        size_t len = va_arg(ap, size_t);
        gcry_md_write(hd, data, len);
    }

    va_end(ap);
    memcpy(out, gcry_md_read(hd, GCRY_MD_SHA1), SRP_SHA1_LEN);
    gcry_md_close(hd);
    return 0;
}

/*!
 * @brief Write an MPI to a buffer as a big-endian integer, zero-padded.
 *
 * @param[out] buf    Output buffer (must be at least @p nbytes).
 * @param[in]  nbytes Target length with leading zero padding.
 * @param[in]  m      MPI to serialize.
 */
static void mpi_to_padded_buf(unsigned char *buf, size_t nbytes, gcry_mpi_t m)
{
    size_t nwritten;
    memset(buf, 0, nbytes);
    gcry_mpi_print(GCRYMPI_FMT_USG, buf, nbytes, &nwritten, m);

    if (nwritten < nbytes) {
        memmove(buf + nbytes - nwritten, buf, nwritten);
        memset(buf, 0, nbytes - nwritten);
    }
}

/*!
 * @brief Write a 2-byte big-endian unsigned integer and advance the pointer.
 *
 * @param[in,out] p   Pointer to the write position (advanced by 2).
 * @param[in]     val Value to write.
 */
static void write_uint16_be(unsigned char **p, uint16_t val)
{
    (*p)[0] = (val >> 8) & 0xFF;
    (*p)[1] = val & 0xFF;
    *p += 2;
}

/*!
 * @brief Read a 2-byte big-endian unsigned integer and advance the pointer.
 *
 * @param[in,out] p Pointer to the read position (advanced by 2).
 * @returns The decoded 16-bit value.
 */
static uint16_t read_uint16_be(unsigned char **p)
{
    uint16_t val = (uint16_t)((*p)[0] << 8 | (*p)[1]);
    *p += 2;
    return val;
}

/* -------------------- Verifier file I/O -------------------- */

#define unhex(x)  (isdigit(x) ? (x) - '0' : toupper(x) + 10 - 'A')

/*!
 * @brief Look up a user's salt and verifier from the SRP verifier file.
 *
 * The file format is one line per user: username:hex_salt:hex_verifier
 *
 * @param[in]  path     Path to the verifier file.
 * @param[in]  username User to look up.
 * @param[out] salt_out Buffer for the salt (SRP_SALT_LEN bytes).
 * @param[out] v_out    MPI set to the verifier on success.
 * @returns 0 on success, -1 on failure (user not found or file error).
 */
static int srp_lookup_verifier(const char *path, const char *username,
                               unsigned char *salt_out,
                               gcry_mpi_t *v_out)
{
    FILE *fp;
    char line[UAM_USERNAMELEN + 1 + SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN + 2];
    const char *p;
    size_t ulen = strnlen(username, UAM_USERNAMELEN + 1);

    if (ulen > UAM_USERNAMELEN) {
        return -1;
    }

    fp = fopen(path, "r");

    if (fp == NULL) {
        LOG(log_error, logtype_uams,
            "srp_lookup_verifier: can't open verifier file %s: %s", path, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        /* Find first colon */
        p = strchr(line, ':');

        if (p == NULL) {
            continue;
        }

        /* Check username match */
        if ((size_t)(p - line) != ulen || strncmp(line, username, ulen) != 0) {
            continue;
        }

        p++; /* skip colon, now pointing at hex salt */

        /* Check for placeholder (starts with '*') */
        if (*p == '*') {
            fclose(fp);
            return -1;
        }

        /* Parse hex salt (32 hex chars = 16 bytes) */
        for (int i = 0; i < SRP_SALT_LEN; i++) {
            if (!isxdigit(p[0]) || !isxdigit(p[1])) {
                fclose(fp);
                return -1;
            }

            salt_out[i] = (unsigned char)((unhex(p[0]) << 4) | unhex(p[1]));
            p += 2;
        }

        if (*p != ':') {
            fclose(fp);
            return -1;
        }

        p++; /* skip colon, now pointing at hex verifier */
        /* Parse hex verifier (384 hex chars = 192 bytes) */
        unsigned char v_bytes[SRP_NBYTES];

        for (int i = 0; i < SRP_NBYTES; i++) {
            if (!isxdigit(p[0]) || !isxdigit(p[1])) {
                fclose(fp);
                return -1;
            }

            v_bytes[i] = (unsigned char)((unhex(p[0]) << 4) | unhex(p[1]));
            p += 2;
        }

        gcry_mpi_scan(v_out, GCRYMPI_FMT_USG, v_bytes, SRP_NBYTES, NULL);
        explicit_bzero(v_bytes, sizeof(v_bytes));
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return -1;
}

/* -------------------- SRP protocol handlers -------------------- */

/*! @brief Release all per-session SRP state and zero sensitive buffers. */
static void srp_session_free(void)
{
    gcry_mpi_release(session_N);
    gcry_mpi_release(session_g);
    gcry_mpi_release(session_v);
    gcry_mpi_release(session_b);
    gcry_mpi_release(session_B);
    session_N = NULL;
    session_g = NULL;
    session_v = NULL;
    session_b = NULL;
    session_B = NULL;
    explicit_bzero(session_salt, sizeof(session_salt));
    explicit_bzero(session_B_buf, sizeof(session_B_buf));
    explicit_bzero(session_username, sizeof(session_username));
}

/*!
 * @brief SRP Round 1 setup.
 *
 * Looks up the user's verifier from the verifier file, generates the
 * server ephemeral key pair (b, B), and builds the FPLoginExt response
 * containing the SRP group parameters, salt, and server public ephemeral B.
 *
 * @returns AFPERR_AUTHCONT on success, AFPERR_NOTAUTH if user not found.
 */
static int srp_setup(void *obj, char *ibuf _U_, size_t ibuflen _U_,
                     char *rbuf, size_t *rbuflen)
{
    char *srpfile = NULL;
    size_t len;
    unsigned char *b_binary = NULL;
    gcry_mpi_t k = NULL, kv = NULL;
    int ret;
    *rbuflen = 0;
    /* Get the SRP verifier file path */
    len = UAM_PASSWD_SRP_FILENAME;

    if (uam_afpserver_option(obj, UAM_OPTION_PASSWDOPT,
                             (void *)&srpfile, &len) < 0) {
        LOG(log_error, logtype_uams, "srp_setup: can't get srp passwd file option");
        return AFPERR_MISC;
    }

    if (!srpfile || len == 0) {
        LOG(log_error, logtype_uams, "srp_setup: SRP password file not configured");
        return AFPERR_MISC;
    }

    /* Initialize group parameters */
    gcry_mpi_scan(&session_N, GCRYMPI_FMT_USG, srp_N_bytes, SRP_NBYTES, NULL);
    gcry_mpi_scan(&session_g, GCRYMPI_FMT_USG, &srp_g_byte, 1, NULL);
    /* Look up verifier */
    session_v = NULL;

    if (srp_lookup_verifier(srpfile, session_username,
                            session_salt, &session_v) != 0) {
        LOG(log_info, logtype_uams, "srp_setup: no verifier for user %s",
            session_username);
        /*
         * Unknown user: return AFPERR_NOTAUTH with 2-byte init marker payload.
         * This matches the observed Apple server behavior.
         */
        unsigned char *rbufp = (unsigned char *)rbuf;
        write_uint16_be(&rbufp, SRP_INIT_MARKER);
        *rbuflen = 2;
        ret = AFPERR_NOTAUTH;
        goto error;
    }

    /* Generate server ephemeral: b random, B = k*v + g^b mod N */
    b_binary = calloc(1, SRP_NBYTES);

    if (b_binary == NULL) {
        ret = AFPERR_MISC;
        goto error;
    }

    session_b = gcry_mpi_new(0);
    session_B = gcry_mpi_new(0);
    kv = gcry_mpi_new(0);
    /* k = SHA1(N | PAD(g)) */
    unsigned char g_padded[SRP_NBYTES];
    memset(g_padded, 0, SRP_NBYTES);
    g_padded[SRP_NBYTES - 1] = srp_g_byte;
    unsigned char k_hash[SRP_SHA1_LEN];

    if (sha1_multi(k_hash,
                   srp_N_bytes, (size_t)SRP_NBYTES,
                   g_padded, (size_t)SRP_NBYTES,
                   NULL) != 0) {
        ret = AFPERR_MISC;
        goto error;
    }

    gcry_mpi_scan(&k, GCRYMPI_FMT_USG, k_hash, SRP_SHA1_LEN, NULL);
    /* Generate b, compute B = (k*v + g^b) mod N */
    gcry_mpi_t gb = gcry_mpi_new(0);

    do {
        gcry_randomize(b_binary, SRP_NBYTES, GCRY_STRONG_RANDOM);
        gcry_mpi_release(session_b);
        session_b = NULL;
        gcry_mpi_scan(&session_b, GCRYMPI_FMT_USG, b_binary, SRP_NBYTES, NULL);
        gcry_mpi_mod(session_b, session_b, session_N);
        gcry_mpi_powm(gb, session_g, session_b, session_N);
        gcry_mpi_mulm(kv, k, session_v, session_N);
        gcry_mpi_addm(session_B, kv, gb, session_N);
    } while (gcry_mpi_cmp_ui(session_B, 0) == 0);

    gcry_mpi_release(gb);
    /* Store B in wire format for later use in u computation */
    mpi_to_padded_buf(session_B_buf, SRP_NBYTES, session_B);
    /* Build response:
     * context(2) | group_index(2) | N_len(2) | N(192) |
     * g_len(2) | g(1) | salt_len(2) | salt(16) | B_len(2) | B(192)
     * Total: 413 bytes
     */
    unsigned char *p = (unsigned char *)rbuf;
    /* Transaction context (echoed from client, or server-assigned) */
    write_uint16_be(&p, 0x0000);
    /* Group index */
    write_uint16_be(&p, SRP_GROUP_INDEX);
    /* N */
    write_uint16_be(&p, SRP_NBYTES);
    memcpy(p, srp_N_bytes, SRP_NBYTES);
    p += SRP_NBYTES;
    /* g */
    write_uint16_be(&p, 1);
    *p++ = srp_g_byte;
    /* salt */
    write_uint16_be(&p, SRP_SALT_LEN);
    memcpy(p, session_salt, SRP_SALT_LEN);
    p += SRP_SALT_LEN;
    /* B */
    write_uint16_be(&p, SRP_NBYTES);
    memcpy(p, session_B_buf, SRP_NBYTES);
    p += SRP_NBYTES;
    *rbuflen = p - (unsigned char *)rbuf;
    ret = AFPERR_AUTHCONT;
    free(b_binary);
    gcry_mpi_release(k);
    gcry_mpi_release(kv);
    return ret;
error:
    free(b_binary);
    gcry_mpi_release(k);
    gcry_mpi_release(kv);
    srp_session_free();
    return ret;
}

/*!
 * @brief FPLogin handler for SRP UAM.
 *
 * Parses the username from the non-extended login request, validates
 * the user, and delegates to srp_setup() for Round 1.
 */
static int srp_login(void *obj, struct passwd **uam_pwd _U_,
                     char *ibuf, size_t ibuflen,
                     char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t len, ulen;
    *rbuflen = 0;

    if (ibuflen < 1) {
        return AFPERR_PARAM;
    }

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *)&username, &ulen) < 0) {
        return AFPERR_PARAM;
    }

    len = (unsigned char) * ibuf++;
    ibuflen--;

    if (len > ulen || len > ibuflen) {
        return AFPERR_PARAM;
    }

    memcpy(username, ibuf, len);
    ibuf += len;
    username[len] = '\0';

    if ((unsigned long)ibuf & 1) {
        ++ibuf;
    }

    if ((srppwd = uam_getname(obj, username, (int)ulen)) == NULL) {
        LOG(log_info, logtype_uams, "srp_login: unknown username");
        return AFPERR_NOTAUTH;
    }

    if (uam_checkuser(obj, srppwd) < 0) {
        LOG(log_info, logtype_uams, "srp_login: user not allowed: %s", username);
        return AFPERR_NOTAUTH;
    }

    LOG(log_info, logtype_uams, "srp_login: login: %s", username);
    strlcpy(session_username, username, sizeof(session_username));
    return srp_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief FPLoginExt handler for SRP UAM.
 *
 * Parses the UTF-8 username from the extended login request, validates
 * the user, and delegates to srp_setup() for Round 1.
 */
static int srp_login_ext(void *obj, char *uname, struct passwd **uam_pwd _U_,
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t len, ulen;
    uint16_t temp16;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *)&username, &ulen) < 0) {
        return AFPERR_PARAM;
    }

    if (*uname != 3) {
        return AFPERR_PARAM;
    }

    uname++;
    memcpy(&temp16, uname, sizeof(temp16));
    len = ntohs(temp16);

    if (!len || len > ulen) {
        return AFPERR_PARAM;
    }

    memcpy(username, uname + 2, len);
    username[len] = '\0';

    if ((srppwd = uam_getname(obj, username, (int)ulen)) == NULL) {
        LOG(log_info, logtype_uams, "srp_login_ext: unknown username");
        return AFPERR_NOTAUTH;
    }

    if (uam_checkuser(obj, srppwd) < 0) {
        LOG(log_info, logtype_uams, "srp_login_ext: user not allowed: %s", username);
        return AFPERR_NOTAUTH;
    }

    LOG(log_info, logtype_uams, "srp_login_ext: login: %s", username);
    strlcpy(session_username, username, sizeof(session_username));
    return srp_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief SRP Round 2: FPLoginCont handler.
 *
 * Parses the client's public ephemeral A and proof M1, computes the
 * server-side shared secret S and session key K, verifies M1, and
 * responds with server proof M2 to complete mutual authentication.
 *
 * @returns AFP_OK on success, AFPERR_NOTAUTH on proof mismatch.
 */
static int srp_logincont(void *obj _U_, struct passwd **uam_pwd,
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
{
    int ret = AFPERR_NOTAUTH;
    gcry_mpi_t A = NULL, u = NULL, S = NULL;
    gcry_mpi_t vu = NULL, Avu = NULL, tmp = NULL;
    unsigned char *S_binary = NULL;
    unsigned char A_buf[SRP_NBYTES];
    unsigned char K[SRP_SESSION_KEY_LEN];
    *rbuflen = 0;

    /* Make sure srp_setup actually ran and established session state */
    if (session_v == NULL) {
        LOG(log_error, logtype_uams, "srp_logincont: called without completing setup");
        ret = AFPERR_PARAM;
        goto fail;
    }

    unsigned char *d = (unsigned char *)ibuf;
    const unsigned char *end = d + ibuflen;

    /* Skip 2-byte AFP ID prefix. The framework's afp_logincont() only strips
     * the command + pad bytes; the AFP ID is still at the start of ibuf. */
    if (d + 2 > end) {
        goto fail;
    }

    d += 2;

    /* Step marker */
    if (d + 2 > end) {
        goto fail;
    }

    uint16_t step = read_uint16_be(&d);

    if (step != SRP_CLIENT_PROOF) {
        LOG(log_error, logtype_uams, "srp_logincont: unexpected step marker 0x%04X",
            step);
        goto fail;
    }

    /* A_len + A */
    if (d + 2 > end) {
        goto fail;
    }

    uint16_t A_len = read_uint16_be(&d);

    if (A_len > SRP_NBYTES || d + A_len > end) {
        LOG(log_error, logtype_uams, "srp_logincont: invalid A length %u", A_len);
        goto fail;
    }

    const unsigned char *A_raw = d;
    gcry_mpi_scan(&A, GCRYMPI_FMT_USG, d, A_len, NULL);
    d += A_len;
    /* Validate A % N != 0 */
    tmp = gcry_mpi_new(0);
    gcry_mpi_mod(tmp, A, session_N);

    if (gcry_mpi_cmp_ui(tmp, 0) == 0) {
        LOG(log_error, logtype_uams, "srp_logincont: A mod N == 0");
        goto fail;
    }

    /* M1_len + M1 */
    if (d + 2 > end) {
        goto fail;
    }

    uint16_t M1_len = read_uint16_be(&d);

    if (M1_len != SRP_SHA1_LEN || d + M1_len > end) {
        LOG(log_error, logtype_uams, "srp_logincont: invalid M1 length %u", M1_len);
        goto fail;
    }

    const unsigned char *M1_received = d;
    /* Compute A padded to N length for u computation */
    memset(A_buf, 0, SRP_NBYTES);

    if (A_len <= SRP_NBYTES) {
        memcpy(A_buf + SRP_NBYTES - A_len, A_raw, A_len);
    } else {
        memcpy(A_buf, A_raw + A_len - SRP_NBYTES, SRP_NBYTES);
    }

    /* u = SHA1(PAD(A) | PAD(B)) */
    unsigned char u_hash[SRP_SHA1_LEN];

    if (sha1_multi(u_hash,
                   A_buf, (size_t)SRP_NBYTES,
                   session_B_buf, (size_t)SRP_NBYTES,
                   NULL) != 0) {
        ret = AFPERR_MISC;
        goto fail;
    }

    gcry_mpi_scan(&u, GCRYMPI_FMT_USG, u_hash, SRP_SHA1_LEN, NULL);

    if (gcry_mpi_cmp_ui(u, 0) == 0) {
        LOG(log_error, logtype_uams, "srp_logincont: u == 0, aborting");
        goto fail;
    }

    /* Server-side shared secret: S = (A * v^u)^b mod N */
    vu = gcry_mpi_new(0);
    Avu = gcry_mpi_new(0);
    S = gcry_mpi_new(0);
    gcry_mpi_powm(vu, session_v, u, session_N);   /* v^u mod N */
    gcry_mpi_mulm(Avu, A, vu, session_N);         /* A * v^u mod N */
    gcry_mpi_powm(S, Avu, session_b, session_N);  /* (A * v^u)^b mod N */
    /* K = MGF1-SHA1(strip(S), 40) */
    S_binary = calloc(1, SRP_NBYTES);

    if (S_binary == NULL) {
        ret = AFPERR_MISC;
        goto fail;
    }

    mpi_to_padded_buf(S_binary, SRP_NBYTES, S);
    size_t S_stripped_len;
    const unsigned char *S_stripped = strip_leading_zeros(S_binary, SRP_NBYTES,
                                      &S_stripped_len);

    if (mgf1_sha1(S_stripped, S_stripped_len, K, sizeof(K)) != 0) {
        LOG(log_error, logtype_uams, "srp_logincont: MGF1 failed");
        ret = AFPERR_MISC;
        goto fail;
    }

    /*
     * Verify M1 = SHA1(H(N)^H(g) | H(username) | salt | strip(A) | strip(B) | K)
     */
    /* H(N) — hash of N with leading zeros stripped */
    size_t N_stripped_len;
    const unsigned char *N_stripped = strip_leading_zeros(srp_N_bytes, SRP_NBYTES,
                                      &N_stripped_len);
    unsigned char H_N[SRP_SHA1_LEN];

    if (sha1_multi(H_N, N_stripped, N_stripped_len, NULL) != 0) {
        ret = AFPERR_MISC;
        goto fail;
    }

    /* H(g) — hash of g as minimal bytes */
    unsigned char H_g[SRP_SHA1_LEN];

    if (sha1_multi(H_g, &srp_g_byte, (size_t)1, NULL) != 0) {
        ret = AFPERR_MISC;
        goto fail;
    }

    /* H(N) XOR H(g) */
    unsigned char xor_ng[SRP_SHA1_LEN];

    for (int i = 0; i < SRP_SHA1_LEN; i++) {
        xor_ng[i] = H_N[i] ^ H_g[i];
    }

    /* H(username) */
    unsigned char H_user[SRP_SHA1_LEN];
    size_t uname_len = strnlen(session_username, UAM_USERNAMELEN + 1);

    if (uname_len > UAM_USERNAMELEN) {
        ret = AFPERR_MISC;
        goto fail;
    }

    if (sha1_multi(H_user, (const unsigned char *)session_username, uname_len,
                   NULL) != 0) {
        ret = AFPERR_MISC;
        goto fail;
    }

    /*
     * strip(A) and strip(B) for M1.
     *
     * Latent ambiguity: it is not fully verified whether real Apple peers
     * (Time Capsule, macOS Tahoe client) feed strip(A)/strip(B) or
     * PAD(A)/PAD(B) into M1. The two are byte-for-byte identical whenever
     * the high byte of A and B is non-zero, which is ~99.6% of random
     * values, so every interop test we have run so far cannot distinguish
     * the two conventions. We chose strip() because it matches Tom Wu's
     * reference SRP-6a derivation, but if intermittent M1 mismatches are ever
     * observed at roughly a 1-in-256 rate, A or B with a leading zero byte
     * is the prime suspect and the fix is likely to switch this to PAD()
     * (or to regenerate b until B has no leading zero,
     * sidestepping the question entirely).
     */
    size_t A_stripped_len, B_stripped_len;
    const unsigned char *A_stripped = strip_leading_zeros(A_buf, SRP_NBYTES,
                                      &A_stripped_len);
    const unsigned char *B_stripped = strip_leading_zeros(session_B_buf, SRP_NBYTES,
                                      &B_stripped_len);
    unsigned char M1_expected[SRP_SHA1_LEN];

    if (sha1_multi(M1_expected,
                   xor_ng, (size_t)SRP_SHA1_LEN,
                   H_user, (size_t)SRP_SHA1_LEN,
                   session_salt, (size_t)SRP_SALT_LEN,
                   A_stripped, A_stripped_len,
                   B_stripped, B_stripped_len,
                   K, (size_t)SRP_SESSION_KEY_LEN,
                   NULL) != 0) {
        ret = AFPERR_MISC;
        goto fail;
    }

    if (ct_memcmp(M1_received, M1_expected, SRP_SHA1_LEN)) {
        LOG(log_info, logtype_uams, "srp_logincont: M1 verification failed for %s",
            session_username);
        ret = SRP_AUTH_FAILURE;
        goto fail;
    }

    /* Compute M2 = SHA1(strip(A) | M1 | K) */
    unsigned char M2[SRP_SHA1_LEN];

    if (sha1_multi(M2,
                   A_stripped, A_stripped_len,
                   M1_received, (size_t)SRP_SHA1_LEN,
                   K, (size_t)SRP_SESSION_KEY_LEN,
                   NULL) != 0) {
        ret = AFPERR_MISC;
        goto fail;
    }

    /* Build response: step_marker(2) | M2_len(2) | M2(20) = 24 bytes */
    unsigned char *p = (unsigned char *)rbuf;
    write_uint16_be(&p, SRP_SERVER_PROOF);
    write_uint16_be(&p, SRP_SHA1_LEN);
    memcpy(p, M2, SRP_SHA1_LEN);
    p += SRP_SHA1_LEN;
    *rbuflen = p - (unsigned char *)rbuf;
    /* Authentication successful */
    *uam_pwd = srppwd;
    ret = AFP_OK;
fail:
    explicit_bzero(K, sizeof(K));
    free(S_binary);
    gcry_mpi_release(A);
    gcry_mpi_release(u);
    gcry_mpi_release(S);
    gcry_mpi_release(vu);
    gcry_mpi_release(Avu);
    gcry_mpi_release(tmp);
    srp_session_free();
    return ret;
}

/* -------------------- UAM module interface -------------------- */

static int uam_setup(void *obj _U_, const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "SRP",
                     srp_login, srp_logincont, NULL, srp_login_ext) < 0) {
        return -1;
    }

    return 0;
}

static void uam_cleanup(void)
{
    uam_unregister(UAM_SERVER_LOGIN, "SRP");
}

UAM_MODULE_EXPORT struct uam_export uams_srp = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};
