/*
 * Regression tests for bounded DHX login continuation parsing.
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef TEST_DHX_PAM
/* Load the system declaration before wrapping pam_strerror(): Linux PAM and
 * OpenPAM differ in whether its handle parameter is const. */
#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif
#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#endif

#include <atalk/afp.h>
#include <atalk/uam.h>

static int decrypt_calls;
static int password_checks;
static int password_mismatch;
static gcry_error_t test_gcry_cipher_decrypt(gcry_cipher_hd_t handle,
                                             void *output, size_t output_size, const void *input,
                                             size_t input_size);

/* Include each implementation directly so the test exercises its static
 * login-continuation handler. The wrapper records whether parsing reached
 * the fixed-size decrypt operation. */
#define gcry_cipher_decrypt test_gcry_cipher_decrypt
#ifdef TEST_DHX_PASSWD
#ifdef HAVE_CRYPT_CHECKPASS
static int test_crypt_checkpass(const char *password, const char *hash);
#else
static char *test_crypt(const char *password, const char *hash);
#endif
#define crypt test_crypt
#define crypt_checkpass test_crypt_checkpass
#include "../../etc/uams/uams_dhx_passwd.c"
#undef crypt
#undef crypt_checkpass
#define CALL_LOGINCONT(obj, pwd, ibuf, ibuflen, rbuf, rbuflen) \
    passwd_logincont((obj), (pwd), (char *)(ibuf), (ibuflen), \
                     (char *)(rbuf), (rbuflen))
#elif defined(TEST_DHX_PAM)
int test_pam_start(const char *service, const char *user,
                   const struct pam_conv *conversation, pam_handle_t **handle);
int test_pam_end(pam_handle_t *handle, int status);
static const char *test_pam_strerror(const pam_handle_t *handle, int status);
#define pam_start test_pam_start
#define pam_end test_pam_end
#define pam_strerror test_pam_strerror
#include "../../etc/uams/uams_dhx_pam.c"
#undef pam_start
#undef pam_end
#undef pam_strerror
#define CALL_LOGINCONT(obj, pwd, ibuf, ibuflen, rbuf, rbuflen) \
    pam_logincont((obj), (pwd), (ibuf), (ibuflen), (rbuf), (rbuflen))
#else
#error "Select one DHX implementation to test"
#endif
#undef gcry_cipher_decrypt

static char expected_password[PASSWDLEN + 1];

#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

/* Check the string consumed by authentication, then force its failure path. */
#ifdef TEST_DHX_PASSWD
#ifdef HAVE_CRYPT_CHECKPASS
static int test_crypt_checkpass(const char *password, const char *hash _U_)
{
    password_checks++;
    password_mismatch = strcmp(password, expected_password) != 0;
    return -1;
}
#else
static char *test_crypt(const char *password, const char *hash _U_)
{
    password_checks++;
    password_mismatch = strcmp(password, expected_password) != 0;
    return "*";
}
#endif
#else
int test_pam_start(const char *service _U_, const char *user _U_,
                   const struct pam_conv *conversation, pam_handle_t **handle _U_)
{
    struct pam_message message = {PAM_PROMPT_ECHO_OFF, "Password:"};
    struct pam_response *response = NULL;
    int result;
#ifdef HAVE_PAM_CONV_CONST_PAM_MESSAGE
    const struct pam_message *const_message_ptr = &message;
    result = conversation->conv(1, &const_message_ptr, &response,
                                conversation->appdata_ptr);
#else
    struct pam_message *message_ptr = &message;
    result = conversation->conv(1, &message_ptr, &response,
                                conversation->appdata_ptr);
#endif
    password_checks++;
    password_mismatch = result != PAM_SUCCESS || response == NULL ||
                        response->resp == NULL ||
                        strcmp(response->resp, expected_password) != 0;

    if (response != NULL) {
        free(response->resp);
        free(response);
    }

    return PAM_AUTH_ERR;
}

int test_pam_end(pam_handle_t *handle _U_, int status _U_)
{
    return PAM_SUCCESS;
}

/* pam_start() deliberately fails, so its error log also needs a stub. */
static const char *test_pam_strerror(const pam_handle_t *handle _U_,
                                     int status _U_)
{
    return "Test authentication failure";
}
#endif

/* The implementation is included directly above. These daemon callbacks are
 * not relevant to the length check, but other functions in the translation
 * unit refer to them. */
int uam_register(const int type _U_, const char *path _U_, const char *name _U_,
                 ...)
{
    return 0;
}

void uam_unregister(const int type _U_, const char *name _U_)
{
    /* Test stub: no UAM registration state is maintained. */
}

struct passwd *uam_getname(void *private _U_, char *name _U_, const int len _U_)
{
    return NULL;
}

int uam_checkuser(void *private _U_, const struct passwd *pwd _U_)
{
    return -1;
}

int uam_afpserver_option(void *private _U_, const int what _U_,
                         void *option _U_, size_t *len _U_)
{
    return -1;
}

static gcry_error_t test_gcry_cipher_decrypt(gcry_cipher_hd_t handle,
                                             void *output, size_t output_size, const void *input,
                                             size_t input_size)
{
    decrypt_calls++;
    return gcry_cipher_decrypt(handle, output, output_size, input, input_size);
}

static int exercise_length(size_t declared_length, int expected_decrypt_calls)
{
    /* afpd passes a pointer into a reusable command buffer. Keep accessible
     * bytes after the logical packet so this test detects stale-byte parsing,
     * rather than manufacturing an allocation-boundary crash. */
    unsigned char command_buffer[1024] = {0};
    unsigned char reply[CRYPT2BUFLEN] = {0};
    struct passwd *authenticated_user = NULL;
    char object;
    void *obj = &object;
    uint16_t sessid = dhxhash(obj);
    size_t reply_length = sizeof(reply);
    int result;
    memcpy(command_buffer + 2, &sessid, sizeof(sessid));
    dhx_release_key();
    K = gcry_mpi_new(0);

    if (K == NULL) {
        return -1;
    }

    gcry_mpi_set_ui(K, 1);
    decrypt_calls = 0;
    result = CALL_LOGINCONT(obj, &authenticated_user, command_buffer + 2,
                            declared_length, reply, &reply_length);

    if (result != AFPERR_PARAM || reply_length != 0 ||
            authenticated_user != NULL ||
            decrypt_calls != expected_decrypt_calls || K != NULL) {
        fprintf(stderr,
                "length %zu: result=%d reply=%zu user=%p decrypts=%d key=%p\n",
                declared_length, result, reply_length,
                (void *)authenticated_user, decrypt_calls, (void *)K);
        dhx_release_key();
        return -1;
    }

    return 0;
}

static int exercise_password(size_t password_length)
{
    const long page_size = sysconf(_SC_PAGESIZE);
    unsigned char packet[sizeof(uint16_t) + CRYPT2BUFLEN] = {0};
    unsigned char key[KEYSIZE] = {0};
    struct passwd password_entry = {0};
    struct passwd *authenticated_user = NULL;
    char username[] = "dhx-bounds-test";
    char hash[] = "!";
    char object;
    uint16_t sessid = dhxhash(&object);
    size_t reply_length = CRYPT2BUFLEN;
    unsigned char *mapping, *reply;
    gcry_cipher_hd_t ctx;
    gcry_error_t error;
    int result;

    if (page_size <= 0 || (size_t)page_size < CRYPT2BUFLEN) {
        return -1;
    }

    /* An exact-size reply ends at an inaccessible page, so both a trailing
     * terminator and an oversized cleanup cause a deterministic failure. */
    mapping = mmap(NULL, (size_t)page_size * 2, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mapping == MAP_FAILED) {
        return -1;
    }

    if (mprotect(mapping + page_size, (size_t)page_size, PROT_NONE) != 0) {
        munmap(mapping, (size_t)page_size * 2);
        return -1;
    }

    reply = mapping + page_size - CRYPT2BUFLEN;
    memcpy(packet, &sessid, sizeof(sessid));
    packet[sizeof(sessid) + KEYSIZE - 1] = 1; /* server nonce (zero) + 1 */
    memset(packet + sizeof(sessid) + KEYSIZE, 'A', PASSWDLEN);

    if (password_length < PASSWDLEN) {
        packet[sizeof(sessid) + KEYSIZE + password_length] = '\0';
    }

    memset(expected_password, 'A', password_length);
    expected_password[password_length] = '\0';
    key[KEYSIZE - 1] = 1;
    error = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5, GCRY_CIPHER_MODE_CBC, 0);

    if (error != GPG_ERR_NO_ERROR) {
        munmap(mapping, (size_t)page_size * 2);
        return -1;
    }

    error = gcry_cipher_setkey(ctx, key, sizeof(key));

    if (error == GPG_ERR_NO_ERROR) {
        error = gcry_cipher_setiv(ctx, "LWallace", 8);
    }

    if (error == GPG_ERR_NO_ERROR) {
        error = gcry_cipher_encrypt(ctx, packet + sizeof(sessid),
                                    CRYPT2BUFLEN, NULL, 0);
    }

    gcry_cipher_close(ctx);

    if (error != GPG_ERR_NO_ERROR) {
        munmap(mapping, (size_t)page_size * 2);
        return -1;
    }

    dhx_release_key();
    K = gcry_mpi_new(0);

    if (K == NULL) {
        munmap(mapping, (size_t)page_size * 2);
        return -1;
    }

    gcry_mpi_set_ui(K, 1);
    memset(randbuf, 0, sizeof(randbuf));
    password_entry.pw_name = username;
    password_entry.pw_passwd = hash;
    dhxpwd = &password_entry;
#ifdef TEST_DHX_PAM
    PAM_username = (unsigned char *)username;
#endif
    decrypt_calls = password_checks = password_mismatch = 0;
    result = CALL_LOGINCONT(&object, &authenticated_user, packet,
                            sizeof(packet), reply, &reply_length);

    if (result != AFPERR_NOTAUTH || reply_length != 0 ||
            authenticated_user != NULL || decrypt_calls != 1 ||
            password_checks != 1 || password_mismatch || K != NULL) {
        fprintf(stderr, "password length %zu: authentication path failed\n",
                password_length);
        goto fail;
    }

    for (size_t i = 0; i < CRYPT2BUFLEN; i++) {
        if (reply[i] != 0) {
            fprintf(stderr, "password length %zu: reply was not cleared\n",
                    password_length);
            goto fail;
        }
    }

#ifdef TEST_DHX_PAM

    if (PAM_password != NULL) {
        fprintf(stderr, "PAM password pointer was not cleared\n");
        goto fail;
    }

#endif
    dhxpwd = NULL;
    munmap(mapping, (size_t)page_size * 2);
    return 0;
fail:
    dhx_release_key();
    dhxpwd = NULL;
    munmap(mapping, (size_t)page_size * 2);
    return -1;
}

int main(void)
{
    static const size_t short_lengths[] = {
        0,
        1,
        sizeof(uint16_t),
        sizeof(uint16_t) + CRYPT2BUFLEN - 1,
    };

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        fprintf(stderr, "libgcrypt version mismatch\n");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < sizeof(short_lengths) / sizeof(short_lengths[0]); i++) {
        if (exercise_length(short_lengths[i], 0) != 0) {
            return EXIT_FAILURE;
        }
    }

    /* The exact protocol-sized input must still reach the decrypt step. */
    if (exercise_length(sizeof(uint16_t) + CRYPT2BUFLEN, 1) != 0) {
        return EXIT_FAILURE;
    }

    static const size_t password_lengths[] = {0, 8, PASSWDLEN - 1, PASSWDLEN};

    for (size_t i = 0; i < sizeof(password_lengths) / sizeof(password_lengths[0]);
            i++) {
        if (exercise_password(password_lengths[i]) != 0) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
