/*
 * Native DHCAST128 and DHX2 authentication for the afptest client.
 * Derived from afpfs-ng's AFP client UAM implementations.
 *
 * Copyright (C) 2006 Alex deVries <alexthepuffin@gmail.com>
 * Copyright (C) 2007 Derrik Pates <dpates@dsdk12.net>
 * Copyright (C) 2025-2026 Daniel Markstedt <daniel@mindani.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <gcrypt.h>

#include <atalk/afp.h>

#include "afpclient.h"
#include "afptest_uam.h"

#define AFP_MAX_USERNAME_LEN 127
#define UAM_NEED_LIBGCRYPT_VERSION "1.4.0"
#define kFPAuthContinue AFPERR_AUTHCONT

struct afptest_uam_server {
    CONN *conn;
    const char *vers;
};

struct afp_rx_buffer {
    char *data;
    size_t maxsize;
    size_t size;
};

static const unsigned char dhx_c2siv[] =
{ 'L', 'W', 'a', 'l', 'l', 'a', 'c', 'e' };
static const unsigned char dhx_s2civ[] =
{ 'C', 'J', 'a', 'l', 'b', 'e', 'r', 't' };
static const unsigned char p_binary[] = {
    0xba, 0x28, 0x73, 0xdf, 0xb0, 0x60, 0x57, 0xd4,
    0x3f, 0x20, 0x24, 0x74, 0x4c, 0xee, 0xe7, 0x5b,
};
static const unsigned char g_binary[] = { 0x07 };

static unsigned char copy_to_pascal(char *dest, const char *src)
{
    unsigned char len = (unsigned char)strnlen(src, AFP_MAX_USERNAME_LEN);
    dest[0] = len;
    memcpy(dest + 1, src, len);
    return len;
}

static void afptest_copy_string(char *dest, size_t size, const char *src)
{
    size_t len;

    if (size == 0) {
        return;
    }

    len = strnlen(src, size - 1);
    memcpy(dest, src, len);
}

static uint16_t afptest_read_be16(const char *src)
{
    uint16_t value;
    memcpy(&value, src, sizeof(value));
    return ntohs(value);
}

static int afptest_uam_copy_reply(struct afptest_uam_server *server,
                                  struct afp_rx_buffer *reply)
{
    const CONN *conn = server->conn;

    if (!reply) {
        return 0;
    }

    if (reply->maxsize < sizeof(uint16_t) ||
            conn->login_cont_len > reply->maxsize - sizeof(uint16_t)) {
        return AFPERR_PARAM;
    }

    {
        uint16_t id_be = htons(conn->login_cont_id);
        memcpy(reply->data, &id_be, sizeof(id_be));
        memcpy(reply->data + sizeof(id_be), conn->login_cont_data,
               conn->login_cont_len);
    }

    reply->size = sizeof(uint16_t) + conn->login_cont_len;
    return 0;
}

static int afptest_uam_login_initial(struct afptest_uam_server *server,
                                     const char *uam, char *auth_info,
                                     unsigned int auth_info_len,
                                     struct afp_rx_buffer *reply)
{
    unsigned int result = AFPopenLoginAuth(server->conn, server->vers, uam,
                                           auth_info, auth_info_len);
    int error = (int)ntohl(result);

    if (error == AFPERR_AUTHCONT) {
        int copy_error = afptest_uam_copy_reply(server, reply);

        if (copy_error != 0) {
            return copy_error;
        }
    }

    return error;
}

static int afptest_uam_login_cont(struct afptest_uam_server *server,
                                  unsigned short id, char *auth_info,
                                  unsigned int auth_info_len,
                                  struct afp_rx_buffer *reply)
{
    unsigned int result;
    int error;
    server->conn->login_cont_id = id;
    result = AFPLoginCont(server->conn, auth_info, auth_info_len);
    error = (int)ntohl(result);

    if (error == AFPERR_AUTHCONT) {
        int copy_error = afptest_uam_copy_reply(server, reply);

        if (copy_error != 0) {
            return copy_error;
        }
    }

    return error;
}

static int dhx_login(struct afptest_uam_server *server, const char *username,
                     const char *passwd)
{
    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        return AFPERR_MISC;
    }

    char *ai = NULL;
    char *d = NULL;
    unsigned char Ra_binary[32], K_binary[16];
    int ai_len, ret;
    const int Ma_len = 16, Mb_len = 16, nonce_len = 16;
    gcry_mpi_t p = NULL, g = NULL, Ra = NULL;
    gcry_mpi_t Ma, Mb = NULL, K, nonce = NULL, new_nonce;
    size_t len;
    struct afp_rx_buffer rbuf;
    unsigned short ID;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    rbuf.data = NULL;
    /* Allocate MPIs which are written in place; gcry_mpi_scan() creates
     * the remaining MPIs. */
    Ma = gcry_mpi_new(0);
    K = gcry_mpi_new(0);
    new_nonce = gcry_mpi_new(0);
    /* Get p and g into a form that libgcrypt can use */
    gcry_mpi_scan(&p, GCRYMPI_FMT_USG, p_binary, sizeof(p_binary), NULL);
    gcry_mpi_scan(&g, GCRYMPI_FMT_USG, g_binary, sizeof(g_binary), NULL);
    /* Get random bytes for Ra. */
    gcry_randomize(Ra_binary, sizeof(Ra_binary), GCRY_STRONG_RANDOM);
    /* Translate the binary form of Ra into libgcrypt's preferred form */
    gcry_mpi_scan(&Ra, GCRYMPI_FMT_USG, Ra_binary, sizeof(Ra_binary), NULL);
    /* Ma = g^Ra mod p <- This is our "public" key, which we exchange
     * with the remote server to help make K, the session key. */
    gcry_mpi_powm(Ma, g, Ra, p);
    /* The first authinfo block contains the username followed by Ma on an
     * even AFP-packet offset. Unlike the standalone client, calculate this
     * from the actual FPLogin prefix rather than the address malloc chose. */
    ai_len = 1 + (int)strnlen(username, AFP_MAX_USERNAME_LEN);

    if ((1 + 1 + (int)strnlen(server->vers, UINT8_MAX) +
            1 + (int)strlen("DHCAST128") + ai_len) & 1) {
        ai_len++;
    }

    ai_len += Ma_len;
    ai = calloc(1, ai_len);
    d = ai;

    if (ai == NULL) {
        goto dhx_noctx_fail;
    }

    d += copy_to_pascal(ai, username) + 1;

    if ((1 + 1 + (int)strnlen(server->vers, UINT8_MAX) +
            1 + (int)strlen("DHCAST128") + (d - ai)) & 1) {
        d++;
    }

    /* Extract Ma to send to the server for the exchange. */
    gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *) d, Ma_len, &len, Ma);

    if (len < (size_t) Ma_len) {
        memmove(d + Ma_len - len, d, len);
        memset(d, 0, Ma_len - len);
    }

    /* 2 bytes for id, 16 bytes for Mb, 32 bytes of crypted message text */
    rbuf.maxsize = 2 + Mb_len + 32;
    rbuf.data = calloc(1, rbuf.maxsize);
    d = rbuf.data;

    if (rbuf.data == NULL) {
        goto dhx_noctx_fail;
    }

    rbuf.size = 0;
    /* Send the first FPLogin request, and see what happens. */
    ret = afptest_uam_login_initial(server, "DHCAST128", ai, ai_len, &rbuf);
    free(ai);
    ai = NULL;

    if (ret != kFPAuthContinue) {
        goto dhx_noctx_cleanup;
    }

    /* The block returned from the server should always be 50 bytes.
     * If it's not, for now, choke and die loudly so we know it. */
    if (rbuf.size != rbuf.maxsize) {
        goto dhx_noctx_fail;
    }

    /* Extract the transaction ID from the server's reply block. */
    ID = afptest_read_be16(d);
    d += sizeof(ID);
    /* Now, extract Mb (the server's "public key" part) directly into
     * a gcry_mpi_t. */
    gcry_mpi_scan(&Mb, GCRYMPI_FMT_USG, d, Mb_len, NULL);
    d += Mb_len;
    /* d now points to the ciphertext, which we'll decrypt in a bit. */
    /* K = Mb^Ra mod p <- This nets us the "session key", which we
     * actually use to encrypt and decrypt data. */
    gcry_mpi_powm(K, Mb, Ra, p);
    gcry_mpi_print(GCRYMPI_FMT_USG, K_binary, sizeof(K_binary), &len, K);

    if (len < sizeof(K_binary)) {
        memmove(K_binary + (sizeof(K_binary) - len), K_binary, len);
        memset(K_binary, 0, sizeof(K_binary) - len);
    }

    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
                                GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx_noctx_fail;
    }

    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx_fail;
    }

    /* Set the initialization vector for server->client transfer. */
    ctxerror = gcry_cipher_setiv(ctx, dhx_s2civ, sizeof(dhx_s2civ));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx_fail;
    }

    /* The plaintext will hold the nonce (16 bytes) and the server's
     * signature (16 bytes - we don't actually look at it though). */
    len = nonce_len + 16;
    /* Decrypt the ciphertext from the server. */
    ctxerror = gcry_cipher_decrypt(ctx, d, len, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx_fail;
    }

    /* Pull the binary form of the nonce into a form that libgcrypt can
     * deal with. */
    gcry_mpi_scan(&nonce, GCRYMPI_FMT_USG, d, nonce_len, NULL);
    /* NOTE: The following 16 bytes of plaintext, which the docs indicate
     * as the server signature, will always contain just 0 values - Apple's
     * docs claim that due to an error in an early implementation, it will
     * always be that way. No point in looking at that. */
    /* d still points into rbuf.data, which is no longer needed. */
    free(rbuf.data);
    rbuf.data = NULL;
    /* Increment the nonce by 1 for sending back to the server. */
    gcry_mpi_add_ui(new_nonce, nonce, 1);
    /* New plaintext is 16 bytes of nonce, and (up to) 64 bytes of
     * password (filled out with NULL values). */
    ai_len = nonce_len + 64;
    ai = calloc(1, ai_len);
    d = ai;

    if (ai == NULL) {
        goto dhx_fail;
    }

    /* Pull the incremented nonce value back out into binary form. */
    gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *) d, nonce_len, &len,
                   new_nonce);

    if (len < (size_t) nonce_len) {
        memmove(d + nonce_len - len, d, len);
        memset(d, 0, nonce_len - len);
    }

    d += nonce_len;
    /* Copy the user's password into the plaintext. */
    afptest_copy_string(d, 64, passwd);
    /* Set the initialization vector for client->server transfer. */
    ctxerror = gcry_cipher_setiv(ctx, dhx_c2siv, sizeof(dhx_c2siv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx_fail;
    }

    /* Encrypt the plaintext to create our new authinfo block. */
    ctxerror = gcry_cipher_encrypt(ctx, ai, ai_len, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx_fail;
    }

    /* Send the FPLoginCont with the new authinfo block, sit back,
     * cross fingers... */
    ret = afptest_uam_login_cont(server, ID, ai, ai_len, NULL);
    goto dhx_cleanup;
dhx_noctx_fail:
    ret = -1;
    goto dhx_noctx_cleanup;
dhx_fail:
    ret = -1;
dhx_cleanup:
    gcry_cipher_close(ctx);
dhx_noctx_cleanup:
    gcry_mpi_release(p);
    gcry_mpi_release(g);
    gcry_mpi_release(Ra);
    gcry_mpi_release(Ma);
    gcry_mpi_release(Mb);
    gcry_mpi_release(K);
    gcry_mpi_release(nonce);
    gcry_mpi_release(new_nonce);
    free(ai);
    free(rbuf.data);
    return ret;
}

static int dhx2_login(struct afptest_uam_server *server, const char *username,
                      const char *passwd)
{
    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        return AFPERR_MISC;
    }

    gcry_mpi_t p = NULL, g = NULL, Mb = NULL, Ra = NULL, nonce = NULL;
    gcry_mpi_t Ma, K, new_nonce;
    char *ai = NULL, *d, *Ra_binary = NULL, *K_binary = NULL;
    char *K_hash = NULL, nonce_binary[16];
    int ai_len, hash_len, ret;
    const int g_len = 4;
    size_t len;
    struct afp_rx_buffer rbuf;
    unsigned short ID, bignum_len;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    rbuf.data = NULL;
    /* Allocate MPIs which are written in place; gcry_mpi_scan() creates
     * the remaining MPIs. */
    Ma = gcry_mpi_new(0);
    K = gcry_mpi_new(0);
    new_nonce = gcry_mpi_new(0);
    /* DHX2 requires the Pascal username to end on an even AFP-packet
     * boundary.  Include the actual FPLogin prefix so the padding remains
     * correct for every AFP version string. */
    ai_len = (int)strnlen(username, AFP_MAX_USERNAME_LEN) + 1;

    if ((1 + 1 + (int)strnlen(server->vers, UINT8_MAX) +
            1 + (int)strlen("DHX2") + ai_len) & 1) {
        ai_len++;
    }

    ai = calloc(1, ai_len);

    if (ai == NULL) {
        goto dhx2_noctx_fail;
    }

    copy_to_pascal(ai, username);
    /* Reply block will contain:
     *   Transaction ID (2 bytes, MSB)
     *   g (4 bytes, MSB)
     *   length of large values in bytes (2 bytes, MSB)
     *   p (minimum 64 bytes, indicated by length value, MSB)
     *   Mb (minimum 64 bytes, indicated by length value, MSB)
     * We'll reserve 256 bytes for each of p and Mb, which covers
     * primes up to 2048 bits. Known servers use 512 or 1024 bits. */
    rbuf.maxsize = 2 + 4 + 2 + 256 * 2;
    rbuf.data = calloc(1, rbuf.maxsize);
    d = rbuf.data;

    if (rbuf.data == NULL) {
        goto dhx2_noctx_fail;
    }

    rbuf.size = 0;
    /* Send the initial request in the login sequence. */
    ret = afptest_uam_login_initial(server, "DHX2", ai, ai_len, &rbuf);
    free(ai);
    ai = NULL;

    if (ret != kFPAuthContinue) {
        goto dhx2_noctx_cleanup;
    }

    if (rbuf.size < 8) {
        goto dhx2_noctx_fail;
    }

    /* Pull the transaction ID out of the reply block. */
    ID = afptest_read_be16(d);
    d += sizeof(ID);
    /* Pull the value of g out of the reply block and directly into an
     * gcry_mpi_t container for later use with libgcrypt. */
    gcry_mpi_scan(&g, GCRYMPI_FMT_USG, d, g_len, NULL);
    d += g_len;
    bignum_len = afptest_read_be16(d);
    d += sizeof(bignum_len);

    if (bignum_len < 64 || bignum_len > 256 ||
            rbuf.size != 8 + 2 * bignum_len) {
        goto dhx2_noctx_fail;
    }

    /* Extract p into an gcry_mpi_t. */
    gcry_mpi_scan(&p, GCRYMPI_FMT_USG, d, bignum_len, NULL);
    d += bignum_len;
    /* Extract Mb into an gcry_mpi_t. */
    gcry_mpi_scan(&Mb, GCRYMPI_FMT_USG, d, bignum_len, NULL);
    free(rbuf.data);
    rbuf.data = NULL;
    Ra_binary = calloc(1, bignum_len);

    if (Ra_binary == NULL) {
        goto dhx2_noctx_fail;
    }

    /* Get random bytes for Ra. */
    gcry_randomize(Ra_binary, bignum_len, GCRY_STRONG_RANDOM);
    /* Pull the random value we just read into an gcry_mpi_t so we can do
     * large-value exponentiation, and generate our Ma. */
    gcry_mpi_scan(&Ra, GCRYMPI_FMT_USG, Ra_binary, bignum_len, NULL);
    free(Ra_binary);
    Ra_binary = NULL;
    /* Ma = g^Ra mod p <- This is our "public" key, which we exchange
     * with the remote server to help make K, the session key. */
    gcry_mpi_powm(Ma, g, Ra, p);
    /* K = Mb^Ra mod p <- This nets us the "session key", which we
     * actually use to encrypt and decrypt data. */
    gcry_mpi_powm(K, Mb, Ra, p);
    K_binary = calloc(1, bignum_len);

    if (K_binary == NULL) {
        goto dhx2_noctx_fail;
    }

    gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *) K_binary, bignum_len, &len,
                   K);

    if (len < bignum_len) {
        memmove(K_binary + bignum_len - len, K_binary, len);
        memset(K_binary, 0, bignum_len - len);
    }

    /* Use a one-shot hash function to generate the MD5 hash of K. */
    hash_len = gcry_md_get_algo_dlen(GCRY_MD_MD5);
    K_hash = calloc(1, hash_len);

    if (K_hash == NULL) {
        goto dhx2_noctx_fail;
    }

    gcry_md_hash_buffer(GCRY_MD_MD5, K_hash, K_binary, bignum_len);
    /* Use an internal gcrypt function to create the random number, so
     * we can do things (more) portably... */
    gcry_create_nonce(nonce_binary, sizeof(nonce_binary));
    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
                                GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx2_noctx_fail;
    }

    /* Set the hashed form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_hash, hash_len);
    free(K_hash);
    K_hash = NULL;

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx2_fail;
    }

    /* Set the initialization vector for client->server transfer. */
    ctxerror = gcry_cipher_setiv(ctx, dhx_c2siv, sizeof(dhx_c2siv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx2_fail;
    }

    /* The new authinfo block will contain Ma (our "public" key part) and
     * the encrypted form of our nonce. */
    ai_len = bignum_len + sizeof(nonce_binary);
    ai = calloc(1, ai_len);
    d = ai;

    if (ai == NULL) {
        goto dhx2_fail;
    }

    gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *) d, bignum_len, &len, Ma);

    if (len < bignum_len) {
        memmove(d + bignum_len - len, d, len);
        memset(d, 0, bignum_len - len);
    }

    d += bignum_len;
    /* Encrypt our nonce into the new authinfo block. */
    ctxerror = gcry_cipher_encrypt(ctx, d, sizeof(nonce_binary),
                                   nonce_binary, sizeof(nonce_binary));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx2_fail;
    }

    /* Reply block will contain ID, then the encrypted form of our
     * nonce + 1 and the server's nonce. */
    rbuf.maxsize = sizeof(ID) + sizeof(nonce_binary) * 2;
    rbuf.data = calloc(1, rbuf.maxsize);
    d = rbuf.data;

    if (rbuf.data == NULL) {
        goto dhx2_fail;
    }

    rbuf.size = 0;
    ret = afptest_uam_login_cont(server, ID, ai, ai_len, &rbuf);
    free(ai);
    ai = NULL;

    if (ret != kFPAuthContinue || rbuf.size != rbuf.maxsize) {
        ret = -1;
        goto dhx2_cleanup;
    }

    /* Get the new transaction ID for the last portion of the exchange. */
    ID = afptest_read_be16(d);
    d += sizeof(ID);
    /* Set the initialization vector for server->client transfer. */
    ctxerror = gcry_cipher_setiv(ctx, dhx_s2civ, sizeof(dhx_s2civ));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx2_fail;
    }

    len = rbuf.maxsize - sizeof(ID);
    /* Decrypt the ciphertext from the server. */
    ctxerror = gcry_cipher_decrypt(ctx, d, len, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx2_fail;
    }

    /* Pull our nonce into an gcry_mpi_t so we can operate. */
    gcry_mpi_scan(&nonce, GCRYMPI_FMT_USG, nonce_binary, sizeof(nonce_binary),
                  NULL);
    /* Increment our nonce by one. */
    gcry_mpi_add_ui(new_nonce, nonce, 1);
    /* Pull the incremented nonce back out into binary form. */
    gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *) nonce_binary,
                   sizeof(nonce_binary), &len,
                   new_nonce);

    if (len < sizeof(nonce_binary)) {
        memmove(nonce_binary + sizeof(nonce_binary) - len,
                nonce_binary, len);
        memset(nonce_binary, 0, sizeof(nonce_binary) - len);
    }

    /* Compare our incremented nonce to the server's incremented copy
     * of our original nonce value; if they don't match, something
     * terrible has happened. */
    if (memcmp(nonce_binary, d, 16) != 0) {
        goto dhx2_fail;
    }

    d += sizeof(nonce_binary);
    /* Pull the server's nonce value into an gcry_mpi_t. */
    gcry_mpi_scan(&nonce, GCRYMPI_FMT_USG, d, sizeof(nonce_binary), NULL);
    /* d still points into rbuf.data; let's dispose of it safely. */
    free(rbuf.data);
    rbuf.data = NULL;
    /* Increment the server's nonce by one. */
    gcry_mpi_add_ui(new_nonce, nonce, 1);
    /* The new plaintext will need 16 bytes for the server nonce (after
     * incrementing), followed by 256 bytes of null-filled space for the
     * user's password. */
    ai_len = sizeof(nonce_binary) + 256;
    ai = calloc(1, ai_len);
    d = ai;

    if (ai == NULL) {
        goto dhx2_fail;
    }

    /* Extract the binary form of the incremented server nonce into
     * the plaintext buffer. */
    gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *) d, sizeof(nonce_binary), &len,
                   new_nonce);

    if (len < sizeof(nonce_binary)) {
        memmove(d + sizeof(nonce_binary) - len, d, len);
        memset(d, 0, sizeof(nonce_binary) - len);
    }

    d += sizeof(nonce_binary);
    /* Copy the user's password into the plaintext buffer. */
    afptest_copy_string(d, 256, passwd);
    /* Set the initialization vector for client->server transfer. */
    ctxerror = gcry_cipher_setiv(ctx, dhx_c2siv, sizeof(dhx_c2siv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx2_fail;
    }

    /* Encrypt our nonce into the new authinfo block. */
    ctxerror = gcry_cipher_encrypt(ctx, ai, ai_len, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto dhx2_fail;
    }

    /* Send the FPLoginCont with the new authinfo block, sit back,
     * cross fingers... */
    ret = afptest_uam_login_cont(server, ID, ai, ai_len, NULL);
    goto dhx2_cleanup;
dhx2_noctx_fail:
    ret = -1;
    goto dhx2_noctx_cleanup;
dhx2_fail:
    ret = -1;
dhx2_cleanup:
    gcry_cipher_close(ctx);
dhx2_noctx_cleanup:
    gcry_mpi_release(p);
    gcry_mpi_release(g);
    gcry_mpi_release(Ra);
    gcry_mpi_release(Ma);
    gcry_mpi_release(Mb);
    gcry_mpi_release(K);
    gcry_mpi_release(nonce);
    gcry_mpi_release(new_nonce);
    free(Ra_binary);
    free(K_binary);
    free(K_hash);
    free(ai);
    free(rbuf.data);
    return ret;
}

int afptest_uam_uses_legacy_login(const char *uam)
{
    return uam && (strcasecmp(uam, "clrtxt") == 0 ||
                   strcasecmp(uam, "Cleartxt Passwrd") == 0);
}

unsigned int afptest_uam_login(CONN *conn, const char *vers,
                               const char *uam, const char *username,
                               const char *password)
{
    struct afptest_uam_server server = { .conn = conn, .vers = vers };
    const struct passwd *entry;
    int result;

    if (!conn || !vers || !uam) {
        return htonl((uint32_t)AFPERR_PARAM);
    }

    if (!username) {
        entry = getpwuid(getuid());

        if (!entry) {
            return htonl((uint32_t)AFPERR_PARAM);
        }

        username = entry->pw_name;
    }

    if (!password) {
        password = "";
    }

    if (strcasecmp(uam, "dhx") == 0 || strcasecmp(uam, "DHCAST128") == 0) {
        result = dhx_login(&server, username, password);
    } else if (strcasecmp(uam, "dhx2") == 0 || strcasecmp(uam, "DHX2") == 0) {
        result = dhx2_login(&server, username, password);
    } else {
        return htonl((uint32_t)AFPERR_BADUAM);
    }

    return htonl((uint32_t)result);
}
