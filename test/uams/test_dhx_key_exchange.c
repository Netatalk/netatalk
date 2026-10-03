/*
 * Regression tests for the initial DHX key exchange.
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gcrypt.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/afp.h>
#include <atalk/uam.h>

#if defined(TEST_DHX_PASSWD) && defined(SHADOWPW)
#include <shadow.h>
static struct spwd *test_getspnam(const char *name);
#define getspnam test_getspnam
#endif

/* Exercise the real key exchange and CAST5 encryption, with daemon account
 * lookup and challenge generation supplied by the stubs below. */
#ifdef TEST_DHX_PASSWD
#include "../../etc/uams/uams_dhx_passwd.c"
#define CALL_KEY_EXCHANGE(obj, pwd, ibuf, ibuflen, rbuf, rbuflen) \
    pwd_login((obj), test_username, sizeof(test_username) - 1, (pwd), \
              (char *)(ibuf), (ibuflen), (char *)(rbuf), (rbuflen))
#ifdef SHADOWPW
#undef getspnam
#endif
#elif defined(TEST_DHX_PAM)
#include "../../etc/uams/uams_dhx_pam.c"
#define CALL_KEY_EXCHANGE(obj, pwd, ibuf, ibuflen, rbuf, rbuflen) \
    login((obj), (unsigned char *)test_username, sizeof(test_username) - 1, \
          (pwd), (ibuf), (ibuflen), (rbuf), (rbuflen))
#else
#error "Select one DHX implementation to test"
#endif

static char test_username[] = "dhx-key-exchange-test";
static char test_password_hash[] = "!";
static struct passwd test_user;
static const unsigned char test_challenge[16] = {
    0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
    0x98, 0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f,
};

int uam_register(const int type _U_, const char *path _U_, const char *name _U_,
                 ...)
{
    return 0;
}

void uam_unregister(const int type _U_, const char *name _U_)
{
    /* Test stub: no UAM registration state is maintained. */
}

struct passwd *uam_getname(void *private _U_, char *name, const int len)
{
    if (len != sizeof(test_username) - 1 || strcmp(name, test_username) != 0) {
        return NULL;
    }

    return &test_user;
}

int uam_checkuser(void *private _U_, const struct passwd *pwd)
{
    return pwd == &test_user ? 0 : -1;
}

int uam_afpserver_option(void *private _U_, const int what,
                         void *option, size_t *len)
{
    if (what != UAM_OPTION_RANDNUM || len == NULL ||
            *len != sizeof(test_challenge)) {
        return -1;
    }

    memcpy(option, test_challenge, sizeof(test_challenge));
    return 0;
}

#if defined(TEST_DHX_PASSWD) && defined(SHADOWPW)
static struct spwd *test_getspnam(const char *name)
{
    static struct spwd shadow_user;

    if (strcmp(name, test_username) != 0) {
        return NULL;
    }

    shadow_user.sp_namp = test_username;
    shadow_user.sp_pwdp = test_password_hash;
    return &shadow_user;
}
#endif

static int exercise_key_exchange(unsigned long private_value,
                                 unsigned char public_value)
{
    /* DHX uses generator 7. Private exponents 1 and 2 give public values
     * 7 and 49, respectively, so the request has fifteen leading zeroes. */
    static const unsigned char prime_bytes[16] = {
        0xba, 0x28, 0x73, 0xdf, 0xb0, 0x60, 0x57, 0xd4,
        0x3f, 0x20, 0x24, 0x74, 0x4c, 0xee, 0xe7, 0x5b,
    };
    static const unsigned char iv[8] = "CJalbert";
    unsigned char request[KEYSIZE] = {0};
    unsigned char reply[sizeof(uint16_t) + KEYSIZE + CRYPTBUFLEN] = {0};
    unsigned char key[KEYSIZE] = {0};
    unsigned char key_bytes[KEYSIZE];
    unsigned char expected_plaintext[CRYPTBUFLEN] = {0};
    struct passwd *authenticated_user = NULL;
    gcry_mpi_t server_public = NULL, prime = NULL;
    gcry_mpi_t client_private = NULL, client_key = NULL;
    gcry_cipher_hd_t cipher = NULL;
    uint16_t session_id;
    char object;
    size_t reply_length = 0, key_length;
    int status, result = -1;
    request[sizeof(request) - 1] = public_value;
    status = CALL_KEY_EXCHANGE(&object, &authenticated_user, request,
                               sizeof(request), reply, &reply_length);

    if (status != AFPERR_AUTHCONT || reply_length != sizeof(reply) ||
            authenticated_user != NULL || K == NULL) {
        fprintf(stderr, "key exchange: status=%d reply=%zu user=%p key=%p\n",
                status, reply_length, (void *)authenticated_user, (void *)K);
        goto cleanup;
    }

    memcpy(&session_id, reply, sizeof(session_id));

    if (session_id != dhxhash(&object)) {
        fprintf(stderr, "key exchange returned an incorrect session ID\n");
        goto cleanup;
    }

    if (gcry_mpi_scan(&server_public, GCRYMPI_FMT_USG,
                      reply + sizeof(session_id), KEYSIZE, NULL) != 0 ||
            gcry_mpi_scan(&prime, GCRYMPI_FMT_USG,
                          prime_bytes, sizeof(prime_bytes), NULL) != 0) {
        fprintf(stderr, "failed to decode key exchange parameters\n");
        goto cleanup;
    }

    client_private = gcry_mpi_new(0);
    client_key = gcry_mpi_new(0);
    gcry_mpi_set_ui(client_private, private_value);
    gcry_mpi_powm(client_key, server_public, client_private, prime);

    if (gcry_mpi_cmp(client_key, K) != 0 ||
            gcry_mpi_print(GCRYMPI_FMT_USG, key_bytes, sizeof(key_bytes),
                           &key_length, client_key) != 0) {
        fprintf(stderr, "client and server derived different shared keys\n");
        goto cleanup;
    }

    memcpy(key + sizeof(key) - key_length, key_bytes, key_length);

    if (gcry_cipher_open(&cipher, GCRY_CIPHER_CAST5,
                         GCRY_CIPHER_MODE_CBC, 0) != 0 ||
            gcry_cipher_setkey(cipher, key, sizeof(key)) != 0 ||
            gcry_cipher_setiv(cipher, iv, sizeof(iv)) != 0 ||
            gcry_cipher_decrypt(cipher, reply + sizeof(session_id) + KEYSIZE,
                                CRYPTBUFLEN, NULL, 0) != 0) {
        fprintf(stderr, "failed to decrypt key exchange challenge\n");
        goto cleanup;
    }

    memcpy(expected_plaintext, test_challenge, sizeof(test_challenge));

    if (memcmp(reply + sizeof(session_id) + KEYSIZE,
               expected_plaintext, sizeof(expected_plaintext)) != 0) {
        fprintf(stderr, "key exchange returned an incorrect challenge or signature\n");
        goto cleanup;
    }

    result = 0;
cleanup:

    if (cipher != NULL) {
        gcry_cipher_close(cipher);
    }

    gcry_mpi_release(client_key);
    gcry_mpi_release(client_private);
    gcry_mpi_release(prime);
    gcry_mpi_release(server_public);
    return result;
}

int main(void)
{
    int result;
#if GCRYPT_VERSION_NUMBER >= 0x010600

    /* The default RNG hashes struct rusage, including fields left unset
     * by musl's getrusage(), causing Valgrind errors on a second exchange.
     * Use the system RNG while still exercising real random generation. */
    if (gcry_control(GCRYCTL_SET_PREFERRED_RNG_TYPE, GCRY_RNG_TYPE_SYSTEM) != 0) {
        fprintf(stderr, "failed to select the system RNG\n");
        return EXIT_FAILURE;
    }

#endif

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        fprintf(stderr, "libgcrypt version mismatch\n");
        return EXIT_FAILURE;
    }

    test_user.pw_name = test_username;
    test_user.pw_passwd = test_password_hash;
    result = exercise_key_exchange(1, 7);

    if (result == 0) {
        /* A second login must replace the unfinished exchange. Retain K
         * here so Valgrind also checks that the previous key is released. */
        result = exercise_key_exchange(2, 49);
    }

    dhx_release_key();
    explicit_bzero(randbuf, sizeof(randbuf));
    dhxpwd = NULL;
#ifdef TEST_DHX_PAM
    PAM_username = NULL;
#endif
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
