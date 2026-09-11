/*
 * Copyright 1999 (c) Adrian Sun (asun@u.washington.edu)
 * Copyright 2026 (c) Daniel Markstedt <daniel@mindani.net>
 * All Rights Reserved. See COPYRIGHT.
 */

/*!
 * @file
 * @brief AFP user password utility
 *
 * Supports two modes:
 *
 * **SRP mode** (default):
 * Manages SRP verifier file for use with the SRP UAM.
 * Format: username:hex_salt(32):hex_verifier(384)
 *
 * **RandNum mode** (-r flag):
 * Manages legacy password file for use with the RandNum UAM.
 * Format: username:hex_password(16):last_login(16):fail_count(8)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <gcrypt.h>

#include <atalk/compat.h>
#include <atalk/constant_time.h>

#ifndef DES_KEY_SZ
#define DES_KEY_SZ 8
#endif

#ifdef USE_CRACKLIB
#include <crack.h>
#endif /* USE_CRACKLIB */

#define OPT_ISROOT  (1 << 0)
#define OPT_CREATE  (1 << 1)
#define OPT_FORCE   (1 << 2)
#define OPT_ADDUSER (1 << 3)
#define OPT_NOCRACK (1 << 4)
#define OPT_RANDNUM (1 << 5)

#define PASSWD_ILLEGAL '*'

/* RandNum format */
#define FORMAT  ":****************:****************:********\n"
#define FORMAT_LEN 44

/* SRP format: :hex_salt(32):hex_verifier(384) */
#define SRP_SALT_LEN      16
#define SRP_NBYTES        192   /* 1536-bit prime */
#define SRP_HEX_SALT_LEN  (SRP_SALT_LEN * 2)
#define SRP_HEX_V_LEN     (SRP_NBYTES * 2)
#define SRP_FORMAT_LEN    (1 + SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN + 1)
#define SRP_SHA1_LEN      20
#define SRP_PASSWDLEN     255
#define USERNAME_MAX_LEN  255

#define UID_START 100
#define HEXPASSWDLEN 16
#define PASSWDLEN 8

static int unhex(unsigned char x)
{
    return isdigit(x) ? x - '0' : toupper(x) + 10 - 'A';
}

/*
 * RFC 5054 group #2: 1536-bit MODP group. N is the safe prime, g = 2.
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

static char buf[MAXPATHLEN + 1];
static const unsigned char hextable[] = "0123456789ABCDEF";

static int validate_opened_file(int fd, const char *path)
{
    struct stat st;

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "afppasswd: can't inspect %s: %s\n", path,
                strerror(errno));
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "afppasswd: %s is not a regular file.\n", path);
        return -1;
    }

    if (st.st_uid != geteuid()) {
        fprintf(stderr, "afppasswd: %s is not owned by the administrator.\n",
                path);
        return -1;
    }

    if (st.st_mode & (S_IRWXG | S_IRWXO)) {
        fprintf(stderr,
                "afppasswd: %s must not be accessible by group or other.\n",
                path);
        return -1;
    }

    if (st.st_nlink != 1) {
        fprintf(stderr, "afppasswd: %s must have exactly one hard link.\n",
                path);
        return -1;
    }

    return 0;
}

static int validate_trusted_directory(int fd, const char *path)
{
    struct stat st;

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "afppasswd: can't inspect directory for %s: %s\n",
                path, strerror(errno));
        return -1;
    }

    if (!S_ISDIR(st.st_mode) || st.st_uid != geteuid() ||
            (st.st_mode & (S_IWGRP | S_IWOTH))) {
        fprintf(stderr,
                "afppasswd: every directory containing %s must be administrator-owned and not writable by group or other.\n",
                path);
        return -1;
    }

    return 0;
}

/*
 * Open an existing credential file without following symlinks. For a setuid
 * invocation, also walk the complete path beneath administrator-controlled
 * directory descriptors so an unprivileged caller cannot redirect an
 * intermediate component.
 */
static int open_credential_file(const char *path, int access_mode, int flags)
{
    char pathbuf[MAXPATHLEN + 1];
    char *component, *next;
    int dirfd = -1, fd = -1;

    if ((flags & OPT_ISROOT) || getuid() == geteuid()) {
        fd = open(path, access_mode | O_CLOEXEC | O_NOFOLLOW);

        if (fd < 0) {
            fprintf(stderr, "afppasswd: can't open %s: %s\n", path,
                    strerror(errno));
            return -1;
        }

        goto validate;
    }

    if (path[0] != '/') {
        fprintf(stderr,
                "afppasswd: the credential path must be absolute for non-root users.\n");
        return -1;
    }

    if (strlcpy(pathbuf, path, sizeof(pathbuf)) >= sizeof(pathbuf)) {
        fprintf(stderr, "afppasswd: credential path is too long.\n");
        return -1;
    }

    dirfd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);

    if (dirfd < 0 || validate_trusted_directory(dirfd, path) < 0) {
        if (dirfd >= 0) {
            close(dirfd);
        }

        return -1;
    }

    component = pathbuf;

    while (*component == '/') {
        component++;
    }

    if (*component == '\0') {
        fprintf(stderr, "afppasswd: credential path names a directory.\n");
        close(dirfd);
        return -1;
    }

    for (;;) {
        int componentfd;
        next = strchr(component, '/');

        if (next != NULL) {
            *next = '\0';
        }

        if (strcmp(component, ".") == 0 || strcmp(component, "..") == 0 ||
                *component == '\0') {
            fprintf(stderr,
                    "afppasswd: credential path contains an unsafe component.\n");
            close(dirfd);
            return -1;
        }

        if (next == NULL) {
            fd = openat(dirfd, component,
                        access_mode | O_CLOEXEC | O_NOFOLLOW);
            close(dirfd);

            if (fd < 0) {
                fprintf(stderr, "afppasswd: can't open %s: %s\n", path,
                        strerror(errno));
                return -1;
            }

            break;
        }

        componentfd = openat(dirfd, component,
                             O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);

        if (componentfd < 0 ||
                validate_trusted_directory(componentfd, path) < 0) {
            if (componentfd >= 0) {
                close(componentfd);
            }

            close(dirfd);
            return -1;
        }

        close(dirfd);
        dirfd = componentfd;
        component = next + 1;

        while (*component == '/') {
            component++;
        }
    }

validate:

    if (validate_opened_file(fd, path) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static int find_append_position(FILE *fp, const char *path, off_t *pos)
{
    int last;
    clearerr(fp);

    if (fseek(fp, 0, SEEK_END) < 0 || (*pos = ftell(fp)) < 0) {
        fprintf(stderr, "afppasswd: can't seek %s: %s\n", path,
                strerror(errno));
        return -1;
    }

    if (*pos == 0) {
        return 0;
    }

    if (fseek(fp, -1, SEEK_END) < 0 || (last = fgetc(fp)) == EOF) {
        fprintf(stderr, "afppasswd: can't inspect the end of %s: %s\n", path,
                strerror(errno));
        return -1;
    }

    if (last != '\n') {
        fprintf(stderr,
                "afppasswd: can't append to %s: final record is incomplete.\n",
                path);
        return -1;
    }

    return 0;
}

static int open_credential_for_replacement(const char *path)
{
    int fd = open(path, O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);

    if (fd < 0) {
        fprintf(stderr, "afppasswd: can't create %s: %s\n", path,
                strerror(errno));
        return -1;
    }

    if (validate_opened_file(fd, path) < 0) {
        close(fd);
        return -1;
    }

    if (fchmod(fd, 0600) < 0 || ftruncate(fd, 0) < 0) {
        fprintf(stderr, "afppasswd: can't safely replace %s: %s\n", path,
                strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static int randnum_make_keypath(const char *path, char *keypath,
                                size_t keypath_size)
{
    size_t path_len = strnlen(path, keypath_size);

    if (path_len > keypath_size - sizeof(".key")) {
        fprintf(stderr,
                "afppasswd: Randnum password file path is too long to locate companion key file.\n");
        return -1;
    }

    strlcpy(keypath, path, keypath_size);
    strlcat(keypath, ".key", keypath_size);
    return 0;
}

static int randnum_read_keyfd(int keyfd, uint8_t key[DES_KEY_SZ],
                              const char *keypath)
{
    uint8_t encoded[HEXPASSWDLEN + 2];
    ssize_t keylen;

    if (lseek(keyfd, 0, SEEK_SET) < 0) {
        fprintf(stderr, "afppasswd: could not seek Randnum key file%s%s: %s\n",
                keypath ? " " : "", keypath ? keypath : "", strerror(errno));
        explicit_bzero(encoded, sizeof(encoded));
        return -1;
    }

    keylen = read(keyfd, encoded, sizeof(encoded));

    if (keylen < 0) {
        fprintf(stderr, "afppasswd: could not read Randnum key file%s%s: %s\n",
                keypath ? " " : "", keypath ? keypath : "", strerror(errno));
        explicit_bzero(encoded, sizeof(encoded));
        return -1;
    }

    if (keylen != HEXPASSWDLEN &&
            (keylen != HEXPASSWDLEN + 1 || encoded[HEXPASSWDLEN] != '\n')) {
        fprintf(stderr,
                "afppasswd: invalid Randnum key file%s%s: expected 16 hexadecimal characters with an optional trailing newline.\n",
                keypath ? " " : "", keypath ? keypath : "");
        explicit_bzero(encoded, sizeof(encoded));
        return -1;
    }

    for (int i = 0; i < HEXPASSWDLEN; i++) {
        if (!isxdigit(encoded[i])) {
            fprintf(stderr,
                    "afppasswd: invalid Randnum key file%s%s: expected hexadecimal characters only.\n",
                    keypath ? " " : "", keypath ? keypath : "");
            explicit_bzero(encoded, sizeof(encoded));
            return -1;
        }
    }

    for (int i = 0, j = 0; i < HEXPASSWDLEN; i += 2, j++) {
        key[j] = (uint8_t)((unhex(encoded[i]) << 4) | unhex(encoded[i + 1]));
    }

    explicit_bzero(encoded, sizeof(encoded));
    return 0;
}

static int randnum_open_keyfile(const char *path, int flags, int *keyfd_out)
{
    char keypath[MAXPATHLEN + 1];
    uint8_t key[DES_KEY_SZ];
    int keyfd;

    if (randnum_make_keypath(path, keypath, sizeof(keypath)) < 0) {
        return -1;
    }

    keyfd = open_credential_file(keypath, O_RDONLY, flags);

    if (keyfd < 0) {
        fprintf(stderr, "afppasswd: required Randnum key file is unavailable.\n");
        return -1;
    }

    if (randnum_read_keyfd(keyfd, key, keypath) < 0) {
        close(keyfd);
        explicit_bzero(key, sizeof(key));
        return -1;
    }

    explicit_bzero(key, sizeof(key));
    *keyfd_out = keyfd;
    return 0;
}

static int randnum_write_keyfile(const char *keypath)
{
    uint8_t key[DES_KEY_SZ];
    char encoded[HEXPASSWDLEN + 1];
    int fd;
    gcry_randomize(key, sizeof(key), GCRY_STRONG_RANDOM);

    for (int i = 0, j = 0; i < DES_KEY_SZ; i++, j += 2) {
        encoded[j] = hextable[(key[i] & 0xF0) >> 4];
        encoded[j + 1] = hextable[key[i] & 0x0F];
    }

    encoded[HEXPASSWDLEN] = '\n';
    fd = open_credential_for_replacement(keypath);

    if (fd < 0) {
        explicit_bzero(key, sizeof(key));
        explicit_bzero(encoded, sizeof(encoded));
        return -1;
    }

    if (write(fd, encoded, sizeof(encoded)) != (ssize_t)sizeof(encoded)) {
        fprintf(stderr, "afppasswd: problem writing Randnum key file %s: %s\n",
                keypath, strerror(errno));
        close(fd);
        explicit_bzero(key, sizeof(key));
        explicit_bzero(encoded, sizeof(encoded));
        return -1;
    }

    close(fd);
    explicit_bzero(key, sizeof(key));
    explicit_bzero(encoded, sizeof(encoded));
    return 0;
}

static int randnum_ensure_keyfile(const char *path, int flags)
{
    char keypath[MAXPATHLEN + 1];
    uint8_t key[DES_KEY_SZ];
    int keyfd;

    if (randnum_make_keypath(path, keypath, sizeof(keypath)) < 0) {
        return -1;
    }

    keyfd = open(keypath, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);

    if (keyfd < 0) {
        if (errno != ENOENT) {
            fprintf(stderr, "afppasswd: can't open Randnum key file %s: %s\n",
                    keypath, strerror(errno));
            return -1;
        }

        return randnum_write_keyfile(keypath);
    }

    if (validate_opened_file(keyfd, keypath) < 0) {
        close(keyfd);
        return -1;
    }

    if (randnum_read_keyfd(keyfd, key, keypath) == 0) {
        close(keyfd);
        explicit_bzero(key, sizeof(key));
        return 0;
    }

    close(keyfd);
    explicit_bzero(key, sizeof(key));

    if (!(flags & OPT_FORCE)) {
        fprintf(stderr,
                "afppasswd: use -f with -r -c to replace invalid Randnum key file %s.\n",
                keypath);
        return -1;
    }

    return randnum_write_keyfile(keypath);
}

/* if newpwd is null, convert passwd_buf from hex to binary. if newpwd isn't
 * null, convert newpwd to hex and save it in passwd_buf. */
static int convert_passwd(char *passwd_buf, char *newpwd, const int keyfd)
{
    uint8_t key[DES_KEY_SZ];
    unsigned int i, j;
    gcry_cipher_hd_t ctx = NULL;
    gcry_error_t ctxerror;

    if (!newpwd) {
        /* convert to binary */
        for (i = j = 0; i < HEXPASSWDLEN; i += 2, j++) {
            passwd_buf[j] = (char)(uint8_t)((unhex(passwd_buf[i]) << 4) |
                                            unhex(passwd_buf[i + 1]));
        }

        if (j <= DES_KEY_SZ) {
            memset(passwd_buf + j, 0, HEXPASSWDLEN - j);
        }
    }

    if (keyfd < 0 || randnum_read_keyfd(keyfd, key, NULL) < 0) {
        return -1;
    }

    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);

    if (ctxerror) {
        fprintf(stderr, "afppasswd: gcry_cipher_open failed: %s\n",
                gcry_strerror(ctxerror));
        explicit_bzero(key, sizeof(key));
        return -1;
    }

    ctxerror = gcry_cipher_setkey(ctx, key, DES_KEY_SZ);
    explicit_bzero(key, sizeof(key));

    if (ctxerror) {
        fprintf(stderr, "afppasswd: gcry_cipher_setkey failed: %s\n",
                gcry_strerror(ctxerror));
        gcry_cipher_close(ctx);
        return -1;
    }

    if (newpwd) {
        ctxerror = gcry_cipher_encrypt(ctx, newpwd, DES_KEY_SZ, NULL, 0);
    } else {
        /* decrypt the password */
        ctxerror = gcry_cipher_decrypt(ctx, passwd_buf, DES_KEY_SZ, NULL, 0);
    }

    if (ctxerror) {
        fprintf(stderr, "afppasswd: Randnum password conversion failed: %s\n",
                gcry_strerror(ctxerror));
        gcry_cipher_close(ctx);
        return -1;
    }

    gcry_cipher_close(ctx);

    if (newpwd) {
        /* convert to hex */
        for (i = j = 0; i < DES_KEY_SZ; i++, j += 2) {
            passwd_buf[j] = hextable[(newpwd[i] & 0xF0) >> 4];
            passwd_buf[j + 1] = hextable[newpwd[i] & 0x0F];
        }
    }

    return 0;
}

/* -------------------- SRP verifier functions -------------------- */

/*
 * Compute SRP verifier: x = SHA1(salt | SHA1(username | ":" | password)),
 * then v = g^x mod N.
 */
static int srp_compute_verifier(const char *username, const char *password,
                                const unsigned char *salt,
                                unsigned char *v_out)
{
    gcry_md_hd_t hd;
    unsigned char inner_hash[SRP_SHA1_LEN];
    unsigned char x_hash[SRP_SHA1_LEN];
    gcry_mpi_t x = NULL, v = NULL, g = NULL, N = NULL;
    size_t nwritten;
    size_t username_len;
    size_t password_len;
    username_len = strnlen(username, USERNAME_MAX_LEN + 1);

    if (username_len > USERNAME_MAX_LEN) {
        return -1;
    }

    password_len = strnlen(password, SRP_PASSWDLEN + 1);

    if (password_len > SRP_PASSWDLEN) {
        return -1;
    }

    /* inner = SHA1(username | ":" | password) */
    if (gcry_md_open(&hd, GCRY_MD_SHA1, 0) != 0) {
        return -1;
    }

    gcry_md_write(hd, username, username_len);
    gcry_md_write(hd, ":", 1);
    gcry_md_write(hd, password, password_len);
    memcpy(inner_hash, gcry_md_read(hd, GCRY_MD_SHA1), SRP_SHA1_LEN);
    gcry_md_close(hd);

    /* x = SHA1(salt | inner) */
    if (gcry_md_open(&hd, GCRY_MD_SHA1, 0) != 0) {
        return -1;
    }

    gcry_md_write(hd, salt, SRP_SALT_LEN);
    gcry_md_write(hd, inner_hash, SRP_SHA1_LEN);
    memcpy(x_hash, gcry_md_read(hd, GCRY_MD_SHA1), SRP_SHA1_LEN);
    gcry_md_close(hd);
    /* v = g^x mod N */
    gcry_mpi_scan(&x, GCRYMPI_FMT_USG, x_hash, SRP_SHA1_LEN, NULL);
    gcry_mpi_scan(&N, GCRYMPI_FMT_USG, srp_N_bytes, SRP_NBYTES, NULL);
    g = gcry_mpi_set_ui(NULL, 2);
    v = gcry_mpi_new(0);
    gcry_mpi_powm(v, g, x, N);
    /* Write v as SRP_NBYTES big-endian, zero-padded */
    memset(v_out, 0, SRP_NBYTES);
    gcry_mpi_print(GCRYMPI_FMT_USG, v_out, SRP_NBYTES, &nwritten, v);

    if (nwritten < SRP_NBYTES) {
        memmove(v_out + SRP_NBYTES - nwritten, v_out, nwritten);
        memset(v_out, 0, SRP_NBYTES - nwritten);
    }

    explicit_bzero(inner_hash, sizeof(inner_hash));
    explicit_bzero(x_hash, sizeof(x_hash));
    gcry_mpi_release(x);
    gcry_mpi_release(v);
    gcry_mpi_release(g);
    gcry_mpi_release(N);
    return 0;
}

/*
 * Write hex-encoded salt and verifier to a buffer.
 * out_hex must have room for SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN bytes.
 */
static void srp_encode_hex(char *out_hex, const unsigned char *salt,
                           const unsigned char *verifier)
{
    for (int i = 0; i < SRP_SALT_LEN; i++) {
        out_hex[i * 2]     = hextable[(salt[i] >> 4) & 0x0F];
        out_hex[i * 2 + 1] = hextable[salt[i] & 0x0F];
    }

    out_hex[SRP_HEX_SALT_LEN] = ':';

    for (int i = 0; i < SRP_NBYTES; i++) {
        out_hex[SRP_HEX_SALT_LEN + 1 + i * 2] =
            hextable[(verifier[i] >> 4) & 0x0F];
        out_hex[SRP_HEX_SALT_LEN + 1 + i * 2 + 1] =
            hextable[verifier[i] & 0x0F];
    }
}

static int valid_username(const char *name)
{
    size_t name_len = strnlen(name, USERNAME_MAX_LEN + 1);
    return name_len > 0 && name_len <= USERNAME_MAX_LEN &&
           strchr(name, ':') == NULL && strchr(name, '\n') == NULL &&
           strchr(name, '\r') == NULL;
}

static int valid_srp_record(const char *fields)
{
    size_t fields_len = SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN + 1;
    int salt_disabled = 1, verifier_disabled = 1;

    if (strlen(fields) != fields_len || fields[SRP_HEX_SALT_LEN] != ':' ||
            fields[fields_len - 1] != '\n') {
        return 0;
    }

    for (int i = 0; i < SRP_HEX_SALT_LEN; i++) {
        salt_disabled &= fields[i] == PASSWD_ILLEGAL;
    }

    for (int i = 0; i < SRP_HEX_V_LEN; i++) {
        verifier_disabled &=
            fields[SRP_HEX_SALT_LEN + 1 + i] == PASSWD_ILLEGAL;
    }

    if (salt_disabled || verifier_disabled) {
        return salt_disabled && verifier_disabled;
    }

    for (int i = 0; i < SRP_HEX_SALT_LEN; i++) {
        if (!isxdigit((unsigned char)fields[i])) {
            return 0;
        }
    }

    for (int i = 0; i < SRP_HEX_V_LEN; i++) {
        if (!isxdigit((unsigned char)fields[SRP_HEX_SALT_LEN + 1 + i])) {
            return 0;
        }
    }

    return 1;
}

static int update_srp_passwd(const char *path, const char *name, int flags,
                             const char *pass)
{
    const char *passwd = NULL;
    char password[SRP_PASSWDLEN + 1] = {0};
    FILE *fp = NULL;
    off_t pos = 0;
    int err = 0;
    int fd;
    const char *p = NULL;
    size_t pass_len;
    size_t name_len;
    unsigned char old_salt[SRP_SALT_LEN] = {0};
    unsigned char old_v[SRP_NBYTES] = {0};
    unsigned char check_v[SRP_NBYTES] = {0};
    unsigned char new_salt[SRP_SALT_LEN] = {0};
    unsigned char new_v[SRP_NBYTES] = {0};
    char hex_buf[SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN] = {0};
    /* line buffer: username + ":" + hex_salt + ":" + hex_verifier + "\n" + NUL */
    char line[255 + 1 + SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN + 2] = {0};

    if ((flags & OPT_ADDUSER) && !(flags & OPT_ISROOT)) {
        fprintf(stderr, "afppasswd: only root can add a user.\n");
        return -1;
    }

    if (!valid_username(name)) {
        fprintf(stderr, "afppasswd: invalid username.\n");
        return -1;
    }

    pass_len = strnlen(pass, SRP_PASSWDLEN + 1);

    if (pass_len > SRP_PASSWDLEN) {
        fprintf(stderr, "afppasswd: max SRP password length is %d.\n", SRP_PASSWDLEN);
        return -1;
    }

    name_len = strnlen(name, sizeof(line));
    fd = open_credential_file(path, O_RDWR, flags);

    if (fd < 0) {
        return -1;
    }

    if ((fp = fdopen(fd, "r+")) == NULL) {
        fprintf(stderr, "afppasswd: can't open stream for %s: %s\n", path,
                strerror(errno));
        close(fd);
        return -1;
    }

    /* Search for existing entry */
    pos = ftell(fp);

    while (fgets(line, sizeof(line), fp)) {
        p = strchr(line, ':');

        if (p && name_len == (size_t)(p - line) && strncmp(line, name, name_len) == 0) {
            p++;

            if (!valid_srp_record(p)) {
                fprintf(stderr, "afppasswd: corrupt verifier file.\n");
                err = -1;
                goto done;
            }

            if (!(flags & OPT_ISROOT) && (*p == PASSWD_ILLEGAL)) {
                fprintf(stderr, "Your password is disabled. Please see your administrator.\n");
                err = -1;
                goto done;
            }

            goto found_entry;
        }

        pos = ftell(fp);
    }

    if (flags & OPT_ADDUSER) {
        /* Keep a new entry entirely in memory until its password is ready. */
        if (find_append_position(fp, path, &pos) < 0) {
            err = -1;
            goto done;
        }
    } else {
        fprintf(stderr, "afppasswd: can't find %s in %s\n", name, path);
        err = -1;
        goto done;
    }

found_entry:

    /* Verify old password for non-root users */
    if ((flags & OPT_ISROOT) == 0) {
        /* For SRP, we can't verify old password from verifier alone.
         * We would need to recompute the verifier from the old password
         * and compare. */
        passwd = getpass("Enter OLD AFP password: ");

        if (passwd == NULL || passwd[0] == '\0') {
            fprintf(stderr, "afppasswd: password input canceled.\n");
            err = -1;
            goto done;
        }

        if (*p == PASSWD_ILLEGAL) {
            fprintf(stderr, "afppasswd: no existing password set.\n");
            err = -1;
            goto done;
        }

        for (int i = 0; i < SRP_SALT_LEN; i++) {
            if (!isxdigit(p[i * 2]) || !isxdigit(p[i * 2 + 1])) {
                fprintf(stderr, "afppasswd: corrupt verifier file.\n");
                err = -1;
                goto done;
            }

            old_salt[i] = (unsigned char)((unhex(p[i * 2]) << 4) | unhex(p[i * 2 + 1]));
        }

        /* Parse existing verifier */
        const char *vp = p + SRP_HEX_SALT_LEN + 1; /* skip salt + colon */

        for (int i = 0; i < SRP_NBYTES; i++) {
            if (!isxdigit(vp[i * 2]) || !isxdigit(vp[i * 2 + 1])) {
                fprintf(stderr, "afppasswd: corrupt verifier file.\n");
                err = -1;
                goto done;
            }

            old_v[i] = (unsigned char)((unhex(vp[i * 2]) << 4) | unhex(vp[i * 2 + 1]));
        }

        /* Recompute verifier from entered password and compare */
        if (srp_compute_verifier(name, passwd, old_salt, check_v) != 0) {
            fprintf(stderr, "afppasswd: internal error computing verifier.\n");
            err = -1;
            goto done;
        }

        if (atalk_ct_memcmp(check_v, old_v, SRP_NBYTES) != 0) {
            fprintf(stderr, "afppasswd: invalid password.\n");
            explicit_bzero(check_v, sizeof(check_v));
            err = -1;
            goto done;
        }

        explicit_bzero(check_v, sizeof(check_v));
    }

    /* Get new password */
    if (pass_len < 1) {
        passwd = getpass("Enter NEW AFP password: ");

        if (passwd == NULL || passwd[0] == '\0') {
            fprintf(stderr, "afppasswd: password input canceled.\n");
            err = -1;
            goto done;
        }

        size_t passwd_len = strnlen(passwd, SRP_PASSWDLEN + 1);

        if (passwd_len > SRP_PASSWDLEN) {
            fprintf(stderr, "afppasswd: max SRP password length is %d.\n", SRP_PASSWDLEN);
            err = -1;
            goto done;
        }

        memcpy(password, passwd, passwd_len + 1);
    } else {
        strlcpy(password, pass, sizeof(password));
    }

#ifdef USE_CRACKLIB

    if (!(flags & OPT_NOCRACK)) {
        const char *pwcheck = FascistCheck(password, _PATH_CRACKLIB);

        if (pwcheck) {
            fprintf(stderr, "Error: %s\n", pwcheck);
            err = -1;
            goto done;
        }
    }

#endif

    if (pass_len < 1) {
        passwd = getpass("Enter NEW AFP password again: ");

        if (passwd == NULL || passwd[0] == '\0' ||
                strcmp(passwd, password) != 0) {
            fprintf(stderr, "afppasswd: passwords don't match!\n");
            err = -1;
            goto done;
        }
    }

    /* Generate new salt and compute verifier */
    gcry_randomize(new_salt, SRP_SALT_LEN, GCRY_STRONG_RANDOM);

    if (srp_compute_verifier(name, password, new_salt, new_v) != 0) {
        fprintf(stderr, "afppasswd: failed to compute verifier.\n");
        err = -1;
        goto done;
    }

    /* Encode as hex */
    srp_encode_hex(hex_buf, new_salt, new_v);
    /* Write to file at the correct offset */
    {
        struct flock lock = {0};
        int expected_len = (int)(name_len + 1 + sizeof(hex_buf) + 1);
        int written;
        lock.l_type = F_WRLCK;
        lock.l_start = 0;
        lock.l_len = 0;
        lock.l_whence = SEEK_SET;

        if (fcntl(fd, F_SETLKW, &lock) < 0 || fseek(fp, pos, SEEK_SET) < 0) {
            fprintf(stderr, "afppasswd: can't lock or seek %s: %s\n", path,
                    strerror(errno));
            err = -1;
            goto done;
        }

        /* Write: username:hex_salt:hex_verifier\n */
        written = fprintf(fp, "%s:%.*s\n", name, (int)sizeof(hex_buf), hex_buf);

        if (written != expected_len || fflush(fp) != 0) {
            fprintf(stderr, "afppasswd: problem writing to %s: %s\n", path,
                    strerror(errno));
            err = -1;
        }

        lock.l_type = F_UNLCK;

        if (fcntl(fd, F_SETLK, &lock) < 0) {
            fprintf(stderr, "afppasswd: can't unlock %s: %s\n", path,
                    strerror(errno));
            err = -1;
        }
    }

    if (err == 0) {
        printf("afppasswd: updated SRP verifier.\n");
    }

done:

    if (passwd != NULL) {
        explicit_bzero((char *)passwd,
                       strnlen(passwd, SRP_PASSWDLEN + 1));
    }

    explicit_bzero(new_salt, sizeof(new_salt));
    explicit_bzero(new_v, sizeof(new_v));
    explicit_bzero(old_salt, sizeof(old_salt));
    explicit_bzero(old_v, sizeof(old_v));
    explicit_bzero(check_v, sizeof(check_v));
    explicit_bzero(hex_buf, sizeof(hex_buf));
    explicit_bzero(password, sizeof(password));
    explicit_bzero(line, sizeof(line));
    fclose(fp);
    return err;
}

static int create_srp_file(const char *path, uid_t minuid)
{
    struct passwd *pwd;
    int fd, err = 0;

    if ((fd = open_credential_for_replacement(path)) < 0) {
        return -1;
    }

    setpwent();

    while ((pwd = getpwent())) {
        if (pwd->pw_uid < minuid) {
            continue;
        }

        /* username + ":" + placeholder salt + ":" + placeholder verifier + "\n" */
        size_t namelen = strnlen(pwd->pw_name, sizeof(buf));

        if (!valid_username(pwd->pw_name) || namelen == sizeof(buf) ||
                namelen + SRP_FORMAT_LEN > sizeof(buf) - 1) {
            continue;
        }

        int n = snprintf(buf, sizeof(buf), "%s:", pwd->pw_name);

        /* Placeholder asterisks for salt */
        for (int i = 0; i < SRP_HEX_SALT_LEN; i++) {
            buf[n++] = PASSWD_ILLEGAL;
        }

        buf[n++] = ':';

        /* Placeholder asterisks for verifier */
        for (int i = 0; i < SRP_HEX_V_LEN; i++) {
            buf[n++] = PASSWD_ILLEGAL;
        }

        buf[n++] = '\n';

        if (write(fd, buf, n) != n) {
            fprintf(stderr, "afppasswd: problem writing to %s: %s\n",
                    path, strerror(errno));
            err = -1;
            break;
        }
    }

    endpwent();
    close(fd);
    return err;
}

/* -------------------- RandNum (legacy) functions -------------------- */

static int valid_hex_or_disabled(const char *field, size_t len)
{
    int disabled = 1;

    for (size_t i = 0; i < len; i++) {
        disabled &= field[i] == PASSWD_ILLEGAL;
    }

    if (disabled) {
        return 1;
    }

    for (size_t i = 0; i < len; i++) {
        if (!isxdigit((unsigned char)field[i])) {
            return 0;
        }
    }

    return 1;
}

static int valid_randnum_record(const char *fields)
{
    size_t fields_len = FORMAT_LEN - 1;
    return strlen(fields) == fields_len && fields[HEXPASSWDLEN] == ':' &&
           fields[HEXPASSWDLEN * 2 + 1] == ':' &&
           fields[fields_len - 1] == '\n' &&
           valid_hex_or_disabled(fields, HEXPASSWDLEN) &&
           valid_hex_or_disabled(fields + HEXPASSWDLEN + 1, HEXPASSWDLEN) &&
           valid_hex_or_disabled(fields + HEXPASSWDLEN * 2 + 2, 8);
}

/* this matches the code in uam_randnum.c */
static int update_passwd(const char *path, const char *name, int flags,
                         const char *pass)
{
    char password[PASSWDLEN + 1] = {0}, *p = NULL, *passwd = "";
    FILE *fp = NULL;
    off_t pos = 0;
    int fd, keyfd = -1, err = 0, new_entry = 0;
    size_t pass_len;
    size_t name_len;

    if ((flags & OPT_ADDUSER) && !(flags & OPT_ISROOT)) {
        fprintf(stderr, "afppasswd: only root can add a user.\n");
        return -1;
    }

    if (!valid_username(name)) {
        fprintf(stderr, "afppasswd: invalid username.\n");
        return -1;
    }

    if (randnum_open_keyfile(path, flags, &keyfd) < 0) {
        return -1;
    }

    fd = open_credential_file(path, O_RDWR, flags);

    if (fd < 0) {
        close(keyfd);
        return -1;
    }

    if ((fp = fdopen(fd, "r+")) == NULL) {
        fprintf(stderr, "afppasswd: can't open stream for %s: %s\n", path,
                strerror(errno));
        close(fd);
        close(keyfd);
        return -1;
    }

    pass_len = strnlen(pass, PASSWDLEN + 1);
    name_len = strnlen(name, sizeof(buf));
    pos = ftell(fp);
    memset(buf, 0, sizeof(buf));

    while (fgets(buf, sizeof(buf), fp)) {
        p = strchr(buf, ':');

        /* check for a match */
        if (p && name_len == (size_t)(p - buf) && strncmp(buf, name, name_len) == 0) {
            p++;

            if (!valid_randnum_record(p)) {
                fprintf(stderr, "afppasswd: corrupt Randnum password file.\n");
                err = -1;
                goto update_done;
            }

            if (!(flags & OPT_ISROOT) && (*p == PASSWD_ILLEGAL)) {
                fprintf(stderr, "Your password is disabled. Please see your administrator.\n");
                err = -1;
                goto update_done;
            }

            goto found_entry;
        }

        pos = ftell(fp);
        memset(buf, 0, sizeof(buf));
    }

    if (flags & OPT_ADDUSER) {
        /* Build the complete new record without changing the file. */
        strlcpy(buf, name, sizeof(buf));
        strlcat(buf, FORMAT, sizeof(buf));
        p = strchr(buf, ':') + 1;

        if (find_append_position(fp, path, &pos) < 0) {
            err = -1;
            goto update_done;
        }

        new_entry = 1;
    } else {
        fprintf(stderr, "afppasswd: can't find %s in %s\n", name, path);
        err = -1;
        goto update_done;
    }

found_entry:

    /* need to verify against old password */
    if ((flags & OPT_ISROOT) == 0) {
        passwd = getpass("Enter OLD AFP password: ");

        if (passwd == NULL || passwd[0] == '\0') {
            fprintf(stderr, "afppasswd: password input canceled.\n");
            err = -1;
            goto update_done;
        }

        if (convert_passwd(p, NULL, keyfd) < 0) {
            err = -1;
            goto update_done;
        }

        if (strncmp(passwd, p, PASSWDLEN)) {
            fprintf(stderr, "afppasswd: invalid password.\n");
            err = -1;
            goto update_done;
        }
    }

    /* new password */
    if (pass_len < 1) {
        passwd = getpass("Enter NEW AFP password: ");

        if (passwd == NULL || passwd[0] == '\0') {
            fprintf(stderr, "afppasswd: password input canceled.\n");
            err = -1;
            goto update_done;
        }

        size_t passwd_len = strnlen(passwd, PASSWDLEN + 1);

        if (passwd_len > PASSWDLEN) {
            fprintf(stderr, "afppasswd: max RandNum password length is %d.\n", PASSWDLEN);
            err = -1;
            goto update_done;
        }

        /* Make sure we null out any remaining bytes of the input string */
        if (passwd_len < PASSWDLEN) {
            for (int s = (int) passwd_len; s <= PASSWDLEN; s++) {
                passwd[s] = '\0';
            }
        }

        memcpy(password, passwd, sizeof(password));
    } else {
        memcpy(password, pass, sizeof(password));

        if (pass_len < PASSWDLEN) {
            for (int i = (int) pass_len; i <= PASSWDLEN; i++) {
                password[i] = '\0';
            }
        }
    }

    password[PASSWDLEN] = '\0';
#ifdef USE_CRACKLIB

    if (!(flags & OPT_NOCRACK)) {
        const char *pwcheck = FascistCheck(password, _PATH_CRACKLIB);

        if (pwcheck) {
            fprintf(stderr, "Error: %s\n", pwcheck);
            err = -1;
            goto update_done;
        }
    }

#endif /* USE_CRACKLIB */

    if (pass_len < 1) {
        passwd = getpass("Enter NEW AFP password again: ");
    }

    if ((passwd != NULL && passwd[0] != '\0' &&
            strcmp(passwd, password) == 0) || pass_len > 0) {
        struct flock lock = {0};
        size_t write_len;

        if (convert_passwd(p, password, keyfd) < 0) {
            err = -1;
            goto update_done;
        }

        lock.l_type = F_WRLCK;
        lock.l_start = 0;
        lock.l_len = 0;
        lock.l_whence = SEEK_SET;

        if (fcntl(fd, F_SETLKW, &lock) < 0 || fseek(fp, pos, SEEK_SET) < 0) {
            fprintf(stderr, "afppasswd: can't lock or seek %s: %s\n", path,
                    strerror(errno));
            err = -1;
            goto update_done;
        }

        write_len = new_entry ? strnlen(buf, sizeof(buf)) :
                    (size_t)(p - buf) + HEXPASSWDLEN;

        if (fwrite(buf, 1, write_len, fp) != write_len || fflush(fp) != 0) {
            fprintf(stderr, "afppasswd: problem writing to %s: %s\n", path,
                    strerror(errno));
            err = -1;
        }

        lock.l_type = F_UNLCK;

        if (fcntl(fd, F_SETLK, &lock) < 0) {
            fprintf(stderr, "afppasswd: can't unlock %s: %s\n", path,
                    strerror(errno));
            err = -1;
        }

        if (err == 0) {
            printf("afppasswd: updated Randnum password.\n");
        }
    } else {
        fprintf(stderr, "afppasswd: passwords don't match!\n");
        err = -1;
    }

update_done:

    if (passwd != NULL) {
        explicit_bzero(passwd, strnlen(passwd, PASSWDLEN + 1));
    }

    explicit_bzero(password, sizeof(password));
    explicit_bzero(buf, sizeof(buf));

    if (keyfd > -1) {
        close(keyfd);
    }

    fclose(fp);
    return err;
}


/* creates a file with all the password entries */
static int create_file(const char *path, uid_t minuid)
{
    struct passwd *pwd;
    int fd, len, err = 0;

    if ((fd = open_credential_for_replacement(path)) < 0) {
        return -1;
    }

    setpwent();

    while ((pwd = getpwent())) {
        if (pwd->pw_uid < minuid) {
            continue;
        }

        /* a little paranoia */
        size_t name_len = strnlen(pwd->pw_name, sizeof(buf));

        if (!valid_username(pwd->pw_name) || name_len == sizeof(buf) ||
                name_len + FORMAT_LEN > sizeof(buf) - 1) {
            continue;
        }

        strlcpy(buf, pwd->pw_name, sizeof(buf));
        strlcat(buf, FORMAT, sizeof(buf));
        len = (int)strnlen(buf, sizeof(buf));

        if (write(fd, buf, len) != len) {
            fprintf(stderr, "afppasswd: problem writing to %s: %s\n", path,
                    strerror(errno));
            err = -1;
            break;
        }
    }

    endpwent();
    close(fd);
    return err;
}


static void print_usage(void)
{
    fprintf(stderr, "afppasswd (Netatalk %s)\n", VERSION);
#ifdef USE_CRACKLIB
    fprintf(stderr,
            "Usage (root): afppasswd [-cfrn] [-a username] [-u minuid] [-p path] [-w string]\n");
#else
    fprintf(stderr,
            "Usage (root): afppasswd [-cfr] [-a username] [-u minuid] [-p path] [-w string]\n");
#endif
    fprintf(stderr,
            "Usage (user): afppasswd [-r]\n");
    fprintf(stderr, "  -a user   add or update the named user\n");
    fprintf(stderr,
            "  -c        create and initialize password file or specific user\n");
    fprintf(stderr, "  -f        force an action\n");
    fprintf(stderr, "  -r        use legacy RandNum mode (default is SRP)\n");
#ifdef USE_CRACKLIB
    fprintf(stderr, "  -n        disable cracklib checking of passwords\n");
#endif
    fprintf(stderr, "  -u uid    minimum uid to use, defaults to 100\n");
    fprintf(stderr, "  -p path   path to password/verifier file\n");
    fprintf(stderr, "  -w string use string as password\n");
}

int main(int argc, char **argv)
{
    struct stat st;
    int flags;
    uid_t uid_min = UID_START, uid;
    char *path = NULL;
    int adduser_seen = 0, path_seen = 0, password_seen = 0, uid_seen = 0;
    const char *pass = "";
    const char *add_username = NULL;
    int i, err = 0;
    extern char *optarg;
    extern int optind;
    flags = ((uid = getuid()) == 0) ? OPT_ISROOT : 0;

    while ((i = getopt(argc, argv, "cfnra:u:p:w:")) != EOF) {
        switch (i) {
        case 'c': /* create and initialize password file or specific user */
            flags |= OPT_CREATE;
            break;

        case 'a': /* add a new user */
            flags |= OPT_ADDUSER;
            add_username = optarg;
            adduser_seen = 1;
            break;

        case 'f': /* force an action */
            flags |= OPT_FORCE;
            break;

        case 'r': /* legacy RandNum mode */
            flags |= OPT_RANDNUM;
            break;

        case 'u':  /* minimum uid to use. default is 100 */
            uid_min = atoi(optarg);
            uid_seen = 1;
            break;
#ifdef USE_CRACKLIB

        case 'n': /* disable CRACKLIB check */
            flags |= OPT_NOCRACK;
            break;
#endif /* USE_CRACKLIB */

        case 'p': /* path to password/verifier file */
            path = optarg;
            path_seen = 1;
            break;

        case 'w': /* password string */
            pass = optarg;
            password_seen = 1;
            break;

        default:
            err++;
            break;
        }
    }

    /* No positional arguments are accepted: the username, when needed,
     * comes from -a (root adding/updating a user) or from getuid() (a
     * regular user changing their own password). */
    if (err || optind != argc) {
        print_usage();
        return -1;
    }

    /*
     * A setuid invocation by a regular user has one narrow interface: change
     * that user's own password, optionally selecting RandNum with -r. Reject
     * every administrative option before choosing or touching a path, looking
     * up an account, or prompting for a password.
     */
    if (!(flags & OPT_ISROOT) &&
            ((flags & (OPT_CREATE | OPT_FORCE | OPT_ADDUSER | OPT_NOCRACK)) ||
             adduser_seen || uid_seen || path_seen || password_seen)) {
        fprintf(stderr,
                "afppasswd: non-root users may specify only the -r option.\n");
        print_usage();
        return -1;
    }

    /* Root running an update must specify the user via -a. */
    if ((flags & OPT_ISROOT) && !(flags & OPT_CREATE) && !(flags & OPT_ADDUSER)) {
        fprintf(stderr,
                "afppasswd: root must specify a user with -a username.\n");
        print_usage();
        return -1;
    }

    /* Set default path based on mode */
    if (!path_seen) {
        if (flags & OPT_RANDNUM) {
            path = _PATH_AFPDPWFILE;
        } else {
            path = _PATH_AFPDSRPPWFILE;
        }
    }

    /* Validate password length for RandNum mode */
    if ((flags & OPT_RANDNUM) && strnlen(pass, PASSWDLEN + 1) > PASSWDLEN) {
        fprintf(stderr, "afppasswd: max RandNum password length is %d.\n", PASSWDLEN);
        return -1;
    }

    if (flags & OPT_CREATE) {
        if ((flags & OPT_ISROOT) == 0) {
            fprintf(stderr, "afppasswd: only root can create the RandNum password file.\n");
            return -1;
        }

        i = lstat(path, &st);

        if (!i && ((flags & OPT_FORCE) == 0)) {
            fprintf(stderr, "afppasswd: RandNum password file already exists.\n");
            return -1;
        }

        if (flags & OPT_RANDNUM) {
            if (randnum_ensure_keyfile(path, flags) < 0) {
                return -1;
            }

            return create_file(path, uid_min);
        } else {
            return create_srp_file(path, uid_min);
        }
    } else {
        struct passwd *pwd = NULL;
        /* Root specifies the user with -a; non-root users operate on themselves. */
        pwd = (flags & OPT_ISROOT) ? getpwnam(add_username) : getpwuid(uid);

        if (pwd) {
            if (flags & OPT_RANDNUM) {
                return update_passwd(path, pwd->pw_name, flags, pass);
            } else {
                return update_srp_passwd(path, pwd->pw_name, flags, pass);
            }
        }

        fprintf(stderr, "afppasswd: can't get password entry.\n");
        return -1;
    }
}
