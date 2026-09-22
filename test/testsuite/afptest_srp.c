/*
   Copyright (c) 2026 Contributors to the Netatalk Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <atalk/compat.h>

#include "afptest_srp.h"

/* -------------------- Crypto helpers -------------------- */

/*!
 * @brief Strip leading zero bytes from a big-endian integer buffer.
 * @returns pointer to the first non-zero byte (or last byte if all zeros).
 */
const unsigned char *srp_strip_leading_zeros(const unsigned char *buf,
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
int srp_mgf1_sha1(const unsigned char *seed, size_t seed_len,
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
void srp_mpi_to_padded_buf(unsigned char *buf, size_t nbytes, gcry_mpi_t m)
{
    size_t nwritten;
    memset(buf, 0, nbytes);
    gcry_mpi_print(GCRYMPI_FMT_USG, buf, nbytes, &nwritten, m);

    if (nwritten < nbytes) {
        memmove(buf + nbytes - nwritten, buf, nwritten);
        memset(buf, 0, nbytes - nwritten);
    }
}

/* -------------------- SRP-6a derivations -------------------- */

/*!
 * @brief x = SHA1(salt | SHA1(username ":" password))
 *
 * The private key both the verifier and the client proof derive from. Bounds
 * both inputs so a caller cannot hash past its buffers.
 *
 * @param[out] x_out  SRP_SHA1_LEN bytes
 *
 * @returns 0 on success, -1 on a length or libgcrypt failure
 */
int srp_derive_x(const char *username, const char *password,
                 const unsigned char *salt, unsigned char *x_out)
{
    gcry_md_hd_t hd;
    unsigned char inner[SRP_SHA1_LEN];
    size_t ulen = strnlen(username, SRP_USERNAME_MAX_LEN + 1);
    size_t plen = strnlen(password, SRP_PASSWDLEN + 1);

    if (ulen > SRP_USERNAME_MAX_LEN || plen > SRP_PASSWDLEN) {
        return -1;
    }

    if (gcry_md_open(&hd, GCRY_MD_SHA1, 0) != 0) {
        return -1;
    }

    gcry_md_write(hd, username, ulen);
    gcry_md_write(hd, ":", 1);
    gcry_md_write(hd, password, plen);
    memcpy(inner, gcry_md_read(hd, GCRY_MD_SHA1), SRP_SHA1_LEN);
    gcry_md_close(hd);

    if (sha1_multi(x_out,
                   salt, (size_t)SRP_SALT_LEN,
                   inner, (size_t)SRP_SHA1_LEN,
                   NULL) != 0) {
        explicit_bzero(inner, sizeof(inner));
        return -1;
    }

    explicit_bzero(inner, sizeof(inner));
    return 0;
}

/*!
 * @brief k = SHA1(N | PAD(g))
 *
 * @param[out] k_out  SRP_SHA1_LEN bytes
 *
 * @returns 0 on success, -1 on failure
 */
int srp_compute_k(unsigned char *k_out)
{
    unsigned char g_padded[SRP_NBYTES];
    memset(g_padded, 0, SRP_NBYTES);
    g_padded[SRP_NBYTES - 1] = srp_g_byte;
    return sha1_multi(k_out,
                      srp_N_bytes, (size_t)SRP_NBYTES,
                      g_padded, (size_t)SRP_NBYTES,
                      NULL);
}

/*!
 * @brief u = SHA1(PAD(A) | PAD(B))
 *
 * Both inputs must already be SRP_NBYTES, zero-padded on the left.
 *
 * @param[out] u_out  SRP_SHA1_LEN bytes
 *
 * @returns 0 on success, -1 on failure
 */
int srp_compute_u(const unsigned char *a_padded, const unsigned char *b_padded,
                  unsigned char *u_out)
{
    return sha1_multi(u_out,
                      a_padded, (size_t)SRP_NBYTES,
                      b_padded, (size_t)SRP_NBYTES,
                      NULL);
}

/*!
 * @brief M1 = SHA1((H(N) XOR H(g)) | H(user) | salt | A | B | K)
 *
 * A and B are the stripped (leading zeros removed) forms, matching Tom Wu's
 * reference SRP-6a derivation and uams_srp.c.
 *
 * @param[out] m1_out  SRP_SHA1_LEN bytes
 *
 * @returns 0 on success, -1 on failure
 */
int srp_compute_proofs(const char *username, const unsigned char *salt,
                       const unsigned char *a_stripped, size_t a_len,
                       const unsigned char *b_stripped, size_t b_len,
                       const unsigned char *key, unsigned char *m1_out)
{
    unsigned char h_n[SRP_SHA1_LEN], h_g[SRP_SHA1_LEN];
    unsigned char xor_ng[SRP_SHA1_LEN], h_user[SRP_SHA1_LEN];
    size_t n_stripped_len;
    const unsigned char *n_stripped;
    size_t ulen = strnlen(username, SRP_USERNAME_MAX_LEN + 1);

    if (ulen > SRP_USERNAME_MAX_LEN) {
        return -1;
    }

    n_stripped = srp_strip_leading_zeros(srp_N_bytes, SRP_NBYTES,
                                         &n_stripped_len);

    if (sha1_multi(h_n, n_stripped, n_stripped_len, NULL) != 0
            || sha1_multi(h_g, &srp_g_byte, (size_t)1, NULL) != 0
            || sha1_multi(h_user, (const unsigned char *)username, ulen,
                          NULL) != 0) {
        return -1;
    }

    for (int i = 0; i < SRP_SHA1_LEN; i++) {
        xor_ng[i] = h_n[i] ^ h_g[i];
    }

    return sha1_multi(m1_out,
                      xor_ng, (size_t)SRP_SHA1_LEN,
                      h_user, (size_t)SRP_SHA1_LEN,
                      salt, (size_t)SRP_SALT_LEN,
                      a_stripped, a_len,
                      b_stripped, b_len,
                      key, (size_t)SRP_SESSION_KEY_LEN,
                      NULL);
}

/*!
 * @brief M2 = SHA1(A | M1 | K), the server's proof to the client
 *
 * @param[out] m2_out  SRP_SHA1_LEN bytes
 *
 * @returns 0 on success, -1 on failure
 */
int srp_compute_server_proof(const unsigned char *a_stripped, size_t a_len,
                             const unsigned char *m1, const unsigned char *key,
                             unsigned char *m2_out)
{
    return sha1_multi(m2_out,
                      a_stripped, a_len,
                      m1, (size_t)SRP_SHA1_LEN,
                      key, (size_t)SRP_SESSION_KEY_LEN,
                      NULL);
}
