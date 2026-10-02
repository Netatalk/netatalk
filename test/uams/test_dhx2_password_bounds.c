/*
 * Regression tests for bounded DHX2 password handling.
 *
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

#include <atalk/afp.h>
#include <atalk/uam.h>
#include <atalk/unicode.h>

#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

#define DHX2_NONCE_LEN 16
#define DHX2_PASSWORD_LEN 256
#define DHX2_PACKET_LEN (2 + DHX2_NONCE_LEN + DHX2_PASSWORD_LEN)

/*
 * Use deterministic stand-ins for the unbounded string consumers.  Both
 * stand-ins walk a sentinel-terminated input exactly as their production
 * counterparts do.  Placing the password against a PROT_NONE page makes an
 * unterminated field fail at its protocol boundary instead of depending on
 * allocator contents.
 */
#ifdef TEST_DHX2_PASSWD
static char *test_crypt(const char *key, const char *setting);
#define crypt test_crypt
#include "../../etc/uams/uams_dhx2_passwd.c"
#undef crypt
#elif defined(TEST_DHX2_PAM)
static size_t test_convert_string_allocate(charset_t from, charset_t to,
                                           const void *src, size_t srclen, char **dest);
#define convert_string_allocate test_convert_string_allocate
#include "../../etc/uams/uams_dhx2_pam.c"
#undef convert_string_allocate
#else
#error "Select one DHX2 implementation to test"
#endif

/* The implementation is included directly above. These callbacks are not
 * exercised by logincont2(), but other functions in the translation unit
 * refer to them. */
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
                         void *option, size_t *len _U_)
{
#ifdef TEST_DHX2_PAM
    static const char hostname[] = "127.0.0.1";

    if (what == UAM_OPTION_CLIENTNAME) {
        *(const char **)option = hostname;
        return 0;
    }

#else
    (void)what;
    (void)option;
#endif
    return -1;
}

static void consume_c_string(const char *string)
{
    const volatile unsigned char *cursor =
        (const volatile unsigned char *)string;

    while (*cursor != '\0') {
        cursor++;
    }
}

#ifdef TEST_DHX2_PASSWD
static char *test_crypt(const char *key, const char *setting _U_)
{
    consume_c_string(key);
    return "*";
}
#else
static size_t test_convert_string_allocate(charset_t from _U_, charset_t to _U_,
                                           const void *src, size_t srclen, char **dest)
{
    *dest = NULL;

    if (srclen == (size_t) -1) {
        consume_c_string(src);
    }

    return (size_t) -1;
}
#endif

static int encrypt_continuation(char *packet, const unsigned char *key)
{
    gcry_cipher_hd_t ctx = NULL;
    gcry_error_t error;
    error = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5, GCRY_CIPHER_MODE_CBC, 0);

    if (error != GPG_ERR_NO_ERROR) {
        return -1;
    }

    error = gcry_cipher_setkey(ctx, key, 16);

    if (error == GPG_ERR_NO_ERROR) {
        error = gcry_cipher_setiv(ctx, dhx_c2siv, sizeof(dhx_c2siv));
    }

    if (error == GPG_ERR_NO_ERROR) {
        error = gcry_cipher_encrypt(ctx, packet + 2,
                                    DHX2_NONCE_LEN + DHX2_PASSWORD_LEN,
                                    NULL, 0);
    }

    gcry_cipher_close(ctx);
    return error == GPG_ERR_NO_ERROR ? 0 : -1;
}

static int verify_continuation_nonce(const char *packet,
                                     const unsigned char *key)
{
    unsigned char nonce[DHX2_NONCE_LEN];
    unsigned char expected_nonce[DHX2_NONCE_LEN] = {0};
    gcry_cipher_hd_t ctx = NULL;
    gcry_error_t error;
    expected_nonce[DHX2_NONCE_LEN - 1] = 1; /* serverNonce + 1 */
    memcpy(nonce, packet + 2, sizeof(nonce));
    error = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5, GCRY_CIPHER_MODE_CBC, 0);

    if (error != GPG_ERR_NO_ERROR) {
        return -1;
    }

    error = gcry_cipher_setkey(ctx, key, 16);

    if (error == GPG_ERR_NO_ERROR) {
        error = gcry_cipher_setiv(ctx, dhx_c2siv, sizeof(dhx_c2siv));
    }

    if (error == GPG_ERR_NO_ERROR) {
        error = gcry_cipher_decrypt(ctx, nonce, sizeof(nonce), NULL, 0);
    }

    gcry_cipher_close(ctx);
    return error == GPG_ERR_NO_ERROR &&
           memcmp(nonce, expected_nonce, sizeof(nonce)) == 0 ? 0 : -1;
}

int main(void)
{
    const long page_size = sysconf(_SC_PAGESIZE);
    unsigned char key[16] = {0};
    struct passwd password_entry = {0};
    struct passwd *authenticated_user = NULL;
    char test_username[] = "dhx2-bounds-test";
    char test_password_hash[] = "!";
    char reply[1];
    size_t reply_len = sizeof(reply);
    char *mapping;
    char *packet;
    int result;

    if (page_size <= 0 || (size_t)page_size < DHX2_PACKET_LEN) {
        fprintf(stderr, "invalid page size\n");
        return EXIT_FAILURE;
    }

    mapping = mmap(NULL, (size_t)page_size * 2,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mapping == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    if (mprotect(mapping + page_size, (size_t)page_size, PROT_NONE) != 0) {
        perror("mprotect");
        munmap(mapping, (size_t)page_size * 2);
        return EXIT_FAILURE;
    }

    packet = mapping + page_size - DHX2_PACKET_LEN;
    memset(packet, 0, DHX2_PACKET_LEN);
    packet[2 + DHX2_NONCE_LEN - 1] = 1;
    memset(packet + 2 + DHX2_NONCE_LEN, 'A', DHX2_PASSWORD_LEN);

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        fprintf(stderr, "libgcrypt version mismatch\n");
        munmap(mapping, (size_t)page_size * 2);
        return EXIT_FAILURE;
    }

    if (encrypt_continuation(packet, key) != 0) {
        fprintf(stderr, "failed to encrypt test continuation\n");
        munmap(mapping, (size_t)page_size * 2);
        return EXIT_FAILURE;
    }

    if (verify_continuation_nonce(packet, key) != 0) {
        fprintf(stderr, "test continuation has an invalid nonce\n");
        munmap(mapping, (size_t)page_size * 2);
        return EXIT_FAILURE;
    }

    K_MD5hash = malloc(sizeof(key));

    if (K_MD5hash == NULL) {
        munmap(mapping, (size_t)page_size * 2);
        return EXIT_FAILURE;
    }

    memcpy(K_MD5hash, key, sizeof(key));
    K_hash_len = sizeof(key);
    serverNonce = gcry_mpi_new(0);
    dhx2_state = DHX2_STATE_EXPECT_CONT2;
    password_entry.pw_name = test_username;
    password_entry.pw_passwd = test_password_hash;
    dhxpwd = &password_entry;
#ifdef TEST_DHX2_PAM
    PAM_username = password_entry.pw_name;
#endif
    result = logincont2(NULL, &authenticated_user, packet, DHX2_PACKET_LEN,
                        reply, &reply_len);
    dhxpwd = NULL;
#ifdef TEST_DHX2_PAM
    PAM_username = NULL;
#endif
    munmap(mapping, (size_t)page_size * 2);

    if (result != AFPERR_NOTAUTH || authenticated_user != NULL) {
        fprintf(stderr, "unterminated password returned %d with user %p\n",
                result, (void *)authenticated_user);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
