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
 * Manages one SRP verifier file per numeric UID for use with the SRP UAM.
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
#include <inttypes.h>
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
#include <atalk/srp.h>
#include <atalk/uam.h>

#include "afppasswd_migrate.h"

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
#define OPT_MIGRATE (1 << 6)
#define OPT_DISABLE (1 << 7)

#define PASSWD_ILLEGAL '*'

/* RandNum format */
#define FORMAT  ":****************:****************:********\n"
#define FORMAT_LEN 44

#define SRP_PASSWDLEN     255

#define UID_START 100
#define HEXPASSWDLEN 16
#define PASSWDLEN 8

#define AFPPASSWD_OPTSTRING "cd:fmnra:p:u:w:"

static int unhex(unsigned char x)
{
    return isdigit(x) ? x - '0' : toupper(x) + 10 - 'A';
}

static char buf[MAXPATHLEN + 1];
static const unsigned char hextable[] = "0123456789ABCDEF";

/*
 * Libgcrypt requires gcry_check_version() before any other API call.  Keep
 * this separate so the standalone unit test, which exercises the internal
 * SRP helpers without entering this program's main(), can initialize it too.
 */
static int initialize_libgcrypt(void)
{
    gcry_control(GCRYCTL_SET_PREFERRED_RNG_TYPE, GCRY_RNG_TYPE_SYSTEM);

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        fprintf(stderr, "afppasswd: libgcrypt %s or later is required.\n",
                UAM_NEED_LIBGCRYPT_VERSION);
        return -1;
    }

    return 0;
}

static int parse_minimum_uid(const char *value, uid_t *uid)
{
    char *end;
    uintmax_t parsed;

    if (*value == '\0' || *value == '-' ||
            isspace((unsigned char) * value)) {
        return -1;
    }

    errno = 0;
    parsed = strtoumax(value, &end, 10);

    if (errno == ERANGE || end == value || *end != '\0' ||
            (uintmax_t)(uid_t)parsed != parsed) {
        return -1;
    }

    *uid = (uid_t)parsed;
    return 0;
}

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

static int open_credential_file(const char *path, int access_mode)
{
    int fd;
    fd = open(path, access_mode | O_CLOEXEC | O_NOFOLLOW);

    if (fd < 0) {
        fprintf(stderr, "afppasswd: can't open %s: %s\n", path,
                strerror(errno));
        return -1;
    }

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

static int randnum_open_keyfile(const char *path, int *keyfd_out)
{
    char keypath[MAXPATHLEN + 1];
    uint8_t key[DES_KEY_SZ];
    int keyfd;

    if (randnum_make_keypath(path, keypath, sizeof(keypath)) < 0) {
        return -1;
    }

    keyfd = open_credential_file(keypath, O_RDONLY);

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
    username_len = strnlen(username, SRP_USERNAME_MAX_LEN + 1);

    if (username_len > SRP_USERNAME_MAX_LEN) {
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
    g = gcry_mpi_set_ui(NULL, srp_g_byte);
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

static int open_srp_verifier_directory(const char *path)
{
    struct stat st;
    uid_t uid = getuid();
    int fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);

    if (fd < 0) {
        if (lstat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            fprintf(stderr,
                    "afppasswd: %s is a legacy flat SRP verifier file; stop afpd and run 'afppasswd -m'.\n",
                    path);
        } else {
            fprintf(stderr,
                    "afppasswd: can't open SRP verifier directory %s: %s\n",
                    path, strerror(errno));
        }

        return -1;
    }

    /* An unprivileged caller may also use the private directory it owns for
     * its own single-user server; root still requires a root-owned store. */
    if (fstat(fd, &st) < 0 || !S_ISDIR(st.st_mode) ||
            (st.st_uid != 0 && (uid == 0 || st.st_uid != uid)) ||
            (st.st_mode & (S_IWGRP | S_IWOTH))) {
        if (uid == 0) {
            fprintf(stderr,
                    "afppasswd: SRP verifier directory %s must be root-owned and not writable by group or other.\n",
                    path);
        } else {
            fprintf(stderr,
                    "afppasswd: SRP verifier directory %s must be owned by root or by uid %ju and not writable by group or other.\n",
                    path, (uintmax_t)uid);
        }

        close(fd);
        return -1;
    }

    return fd;
}

static int srp_uid_filename(uid_t uid, char *name, size_t size)
{
    int len = snprintf(name, size, "%ju", (uintmax_t)uid);
    return len < 0 || (size_t)len >= size ? -1 : 0;
}

static int validate_srp_verifier_file(int fd, uid_t uid, const char *path,
                                      int administrative)
{
    struct stat st;

    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode) ||
            (st.st_uid != uid && !(administrative && st.st_uid == 0)) ||
            (!administrative && !srp_verifier_mode_is_safe(st.st_mode)) ||
            st.st_nlink != 1) {
        fprintf(stderr,
                "afppasswd: verifier in %s must be a single-link regular file owned by uid %ju%s.\n",
                path, (uintmax_t)uid,
                administrative ? " or root" : " and accessible only by its owner");
        return -1;
    }

    return 0;
}

static int open_srp_verifier(int dirfd, const char *path, uid_t uid,
                             int create)
{
    char uid_name[3 * sizeof(uid_t) + 1];
    int fd;
    int created = 0;

    if (srp_uid_filename(uid, uid_name, sizeof(uid_name)) < 0) {
        return -1;
    }

    fd = openat(dirfd, uid_name, O_RDWR | O_CLOEXEC | O_NOFOLLOW |
                (create ? O_CREAT | O_EXCL : 0), 0600);

    if (fd < 0 && create && errno == EEXIST) {
        fd = openat(dirfd, uid_name, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    } else if (fd >= 0) {
        created = create;
    }

    if (fd < 0) {
        if (!create && errno == EACCES) {
            /* A root-owned placeholder denotes a user who is not enrolled. */
            fprintf(stderr,
                    "Your password is disabled. Please see your administrator.\n");
        } else {
            fprintf(stderr, "afppasswd: can't open verifier %s/%s: %s\n",
                    path, uid_name, strerror(errno));
        }

        return -1;
    }

    /* Administrative callers may repair permissions, but must first validate
     * the file type, ownership, and link count without changing metadata. */
    if (validate_srp_verifier_file(fd, created ? 0 : uid, path,
                                   create) < 0) {
        close(fd);
        return -1;
    }

    /* Root's -a and -d, as well as initialization, normalize permissions before
     * reading or writing credentials. New files remain root-owned until -a. */
    if (create && fchmod(fd, 0600) < 0) {
        fprintf(stderr, "afppasswd: can't prepare verifier %s/%s: %s\n",
                path, uid_name, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static int update_srp_passwd(const char *path, const char *name, uid_t uid,
                             int flags, const char *pass)
{
    char *passwd = NULL;
    char password[SRP_PASSWDLEN + 1] = {0};
    FILE *fp = NULL;
    int err = 0;
    int dirfd = -1, fd;
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
    char line[SRP_USERNAME_MAX_LEN + SRP_FORMAT_LEN + 1] = {0};
    /* Only a user's -c -p bootstrap of a private store arrives with OPT_CREATE. */
    const int bootstrap = !(flags & OPT_ISROOT) && (flags & OPT_CREATE);

    if ((flags & OPT_ADDUSER) && !(flags & OPT_ISROOT)) {
        fprintf(stderr, "afppasswd: only root can add a user.\n");
        return -1;
    }

    if (!srp_valid_username(name)) {
        fprintf(stderr, "afppasswd: invalid username.\n");
        return -1;
    }

    pass_len = strnlen(pass, SRP_PASSWDLEN + 1);

    if (pass_len > SRP_PASSWDLEN) {
        fprintf(stderr, "afppasswd: max SRP password length is %d.\n", SRP_PASSWDLEN);
        return -1;
    }

    name_len = strnlen(name, sizeof(line));
    dirfd = open_srp_verifier_directory(path);

    if (dirfd < 0) {
        return -1;
    }

    fd = open_srp_verifier(dirfd, path, uid,
                           (flags & OPT_ISROOT) && (flags & OPT_ADDUSER));

    if (fd < 0) {
        close(dirfd);
        return -1;
    }

    if ((fp = fdopen(fd, "r+")) == NULL) {
        fprintf(stderr, "afppasswd: can't open stream for %s: %s\n", path,
                strerror(errno));
        close(fd);
        close(dirfd);
        return -1;
    }

    /* A per-user verifier file contains at most one existing record. */
    if (fgets(line, sizeof(line), fp) != NULL && fgetc(fp) != EOF) {
        fprintf(stderr,
                "afppasswd: verifier file must contain exactly one record.\n");
        err = -1;
        goto done;
    }

    if (ferror(fp)) {
        fprintf(stderr, "afppasswd: can't read verifier in %s: %s\n", path,
                strerror(errno));
        err = -1;
        goto done;
    }

    p = strchr(line, ':');

    /* Root's add mode also permits empty files and stale usernames after uid
     * reuse; a user's bootstrap starts from the empty file it just created. */
    if (p && name_len == (size_t)(p - line) && strncmp(line, name, name_len) == 0) {
        p++;

        if (!srp_valid_fields(p)) {
            fprintf(stderr, "afppasswd: corrupt verifier file.\n");
            err = -1;
            goto done;
        }

        if (!(flags & OPT_ISROOT) && (*p == SRP_DISABLED_CHAR)) {
            fprintf(stderr, "Your password is disabled. Please see your administrator.\n");
            err = -1;
            goto done;
        }
    } else if (!(flags & OPT_ADDUSER) && !bootstrap) {
        fprintf(stderr, "afppasswd: can't find verifier for %s in %s\n", name,
                path);
        err = -1;
        goto done;
    }

    /* Verify old password for non-root users */
    if ((flags & OPT_ISROOT) == 0 && !bootstrap) {
        /* Recompute the verifier from the supplied old password. */
        passwd = getpass("Enter OLD AFP password: ");

        if (passwd == NULL || passwd[0] == '\0') {
            fprintf(stderr, "afppasswd: password input canceled.\n");
            err = -1;
            goto done;
        }

        if (*p == SRP_DISABLED_CHAR) {
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
    /* Replace the single record from the start of the file. */
    {
        struct flock lock = {0};
        int expected_len = (int)(name_len + 1 + sizeof(hex_buf) + 1);
        int written;
        lock.l_type = F_WRLCK;
        lock.l_start = 0;
        lock.l_len = 0;
        lock.l_whence = SEEK_SET;

        if (fcntl(fd, F_SETLK, &lock) < 0 || fseek(fp, 0, SEEK_SET) != 0) {
            fprintf(stderr, "afppasswd: can't lock or seek %s: %s\n", path,
                    strerror(errno));
            err = -1;
            goto done;
        }

        /* Write: username:hex_salt:hex_verifier\n */
        written = fprintf(fp, "%s:%.*s\n", name, (int)sizeof(hex_buf), hex_buf);

        if (written != expected_len || fflush(fp) != 0 ||
                ftruncate(fd, expected_len) < 0 || fsync(fd) < 0) {
            fprintf(stderr, "afppasswd: problem writing to %s: %s\n", path,
                    strerror(errno));
            err = -1;
        }

        /* Ownership grants SRP enrollment, only after the verifier is durable. */
        if (err == 0 && (flags & OPT_ISROOT) &&
                (fchown(fd, uid, (gid_t) -1) < 0 || fsync(fd) < 0)) {
            fprintf(stderr, "afppasswd: can't enable verifier in %s: %s\n",
                    path, strerror(errno));
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
        explicit_bzero(passwd, strnlen(passwd, SRP_PASSWDLEN + 1));
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
    close(dirfd);
    return err;
}

/* Disable an SRP account by installing a root-owned placeholder. */
static int disable_srp_verifier(const char *path, const char *name, uid_t uid)
{
    char line[SRP_USERNAME_MAX_LEN + SRP_FORMAT_LEN + 1];
    struct flock lock = {0};
    int dirfd = -1, fd = -1;
    size_t name_len;
    int length;
    int err = -1;

    if (!srp_valid_username(name)) {
        fprintf(stderr, "afppasswd: invalid username.\n");
        return -1;
    }

    name_len = strnlen(name, SRP_USERNAME_MAX_LEN + 1);
    length = snprintf(line, sizeof(line), "%s:", name);

    if (length < 0 || (size_t)length != name_len + 1 ||
            (size_t)length + SRP_FIELDS_LEN >
            sizeof(line)) {
        fprintf(stderr, "afppasswd: username is too long.\n");
        return -1;
    }

    memset(line + length, SRP_DISABLED_CHAR, SRP_HEX_SALT_LEN);
    length += SRP_HEX_SALT_LEN;
    line[length++] = ':';
    memset(line + length, SRP_DISABLED_CHAR, SRP_HEX_V_LEN);
    length += SRP_HEX_V_LEN;
    line[length++] = '\n';

    if ((dirfd = open_srp_verifier_directory(path)) < 0) {
        return -1;
    }

    /* Creation makes disable idempotent and handles uid-reuse cleanup. */
    if ((fd = open_srp_verifier(dirfd, path, uid, 1)) < 0) {
        goto done;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_whence = SEEK_SET;

    /* Do not let a user-held advisory lock block an administrator forever. */
    if (fcntl(fd, F_SETLK, &lock) < 0) {
        fprintf(stderr, "afppasswd: can't lock verifier in %s: %s\n", path,
                strerror(errno));
        goto done;
    }

    /* Revoke enrolment before replacing the record. */
    if (fchown(fd, 0, (gid_t) -1) < 0 || fchmod(fd, 0600) < 0 ||
            ftruncate(fd, 0) < 0 || lseek(fd, 0, SEEK_SET) < 0 ||
            write(fd, line, length) != length || fsync(fd) < 0 ||
            fsync(dirfd) < 0) {
        fprintf(stderr, "afppasswd: can't disable verifier in %s: %s\n", path,
                strerror(errno));
        goto unlock;
    }

    err = 0;
unlock:
    lock.l_type = F_UNLCK;

    if (fcntl(fd, F_SETLK, &lock) < 0) {
        fprintf(stderr, "afppasswd: can't unlock verifier in %s: %s\n", path,
                strerror(errno));
        err = -1;
    }

done:

    if (fd >= 0) {
        close(fd);
    }

    close(dirfd);
    explicit_bzero(line, sizeof(line));
    return err;
}

static int create_srp_directory(const char *path, uid_t minuid)
{
    struct passwd *pwd;
    int dirfd, err = 0;

    if (mkdir(path, 0755) < 0) {
        fprintf(stderr, "afppasswd: can't create SRP verifier directory %s: %s\n",
                path, strerror(errno));
        return -1;
    }

    if ((dirfd = open_srp_verifier_directory(path)) < 0) {
        return -1;
    }

    if (fchmod(dirfd, 0755) < 0) {
        fprintf(stderr,
                "afppasswd: can't set permissions on SRP verifier directory %s: %s\n",
                path, strerror(errno));
        close(dirfd);
        return -1;
    }

    setpwent();

    while ((pwd = getpwent())) {
        if (pwd->pw_uid < minuid) {
            continue;
        }

        int fd;
        /* username + ":" + placeholder salt + ":" + placeholder verifier + "\n" */
        size_t namelen = strnlen(pwd->pw_name, sizeof(buf));

        if (!srp_valid_username(pwd->pw_name) || namelen == sizeof(buf) ||
                namelen + SRP_FORMAT_LEN > sizeof(buf) - 1) {
            continue;
        }

        int n = snprintf(buf, sizeof(buf), "%s:", pwd->pw_name);

        /* Placeholder asterisks for salt */
        for (int i = 0; i < SRP_HEX_SALT_LEN; i++) {
            buf[n++] = SRP_DISABLED_CHAR;
        }

        buf[n++] = ':';

        /* Placeholder asterisks for verifier */
        for (int i = 0; i < SRP_HEX_V_LEN; i++) {
            buf[n++] = SRP_DISABLED_CHAR;
        }

        buf[n++] = '\n';
        fd = open_srp_verifier(dirfd, path, pwd->pw_uid, 1);

        if (fd < 0) {
            err = -1;
            break;
        }

        /* Reinitialization must revoke enrollment for existing files too. */
        if (fchown(fd, 0, (gid_t) -1) < 0 || ftruncate(fd, 0) < 0 ||
                lseek(fd, 0, SEEK_SET) < 0 || write(fd, buf, n) != n ||
                fsync(fd) < 0) {
            fprintf(stderr, "afppasswd: problem writing to %s: %s\n",
                    path, strerror(errno));
            err = -1;
            close(fd);
            break;
        }

        close(fd);
    }

    endpwent();
    close(dirfd);
    return err;
}

/* Bootstrap the private verifier directory of an unprivileged single-user
 * server: only the calling user's own uid file is created. */
static int create_private_srp_verifier(const char *path, uid_t uid, int flags,
                                       const char *pass)
{
    const struct passwd *pwd;
    struct stat st;
    char uid_name[3 * sizeof(uid_t) + 1];
    int dirfd, fd;

    if (mkdir(path, 0700) < 0 && errno != EEXIST) {
        fprintf(stderr, "afppasswd: can't create SRP verifier directory %s: %s\n",
                path, strerror(errno));
        return -1;
    }

    if ((dirfd = open_srp_verifier_directory(path)) < 0) {
        return -1;
    }

    /* An existing directory must already be the caller's private store. */
    if (fstat(dirfd, &st) < 0 || st.st_uid != uid ||
            (st.st_mode & (S_IRWXG | S_IRWXO))) {
        fprintf(stderr,
                "afppasswd: SRP verifier directory %s must be a mode-0700 directory owned by uid %ju.\n",
                path, (uintmax_t)uid);
        close(dirfd);
        return -1;
    }

    if (fchmod(dirfd, 0700) < 0) {
        fprintf(stderr,
                "afppasswd: can't set permissions on SRP verifier directory %s: %s\n",
                path, strerror(errno));
        close(dirfd);
        return -1;
    }

    if (srp_uid_filename(uid, uid_name, sizeof(uid_name)) < 0) {
        close(dirfd);
        return -1;
    }

    fd = openat(dirfd, uid_name, O_WRONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW |
                ((flags & OPT_FORCE) ? 0 : O_EXCL), 0600);

    if (fd < 0) {
        if (errno == EEXIST) {
            fprintf(stderr,
                    "afppasswd: verifier %s/%s already exists; use -f to replace it.\n",
                    path, uid_name);
        } else {
            fprintf(stderr, "afppasswd: can't create verifier %s/%s: %s\n",
                    path, uid_name, strerror(errno));
        }

        close(dirfd);
        return -1;
    }

    if (validate_srp_verifier_file(fd, uid, path, 0) < 0) {
        close(fd);
        close(dirfd);
        return -1;
    }

    /* With -f, the caller's existing verifier is emptied so that the update
     * below proceeds without an old-password proof. */
    if (ftruncate(fd, 0) < 0 || fsync(fd) < 0 || fsync(dirfd) < 0) {
        fprintf(stderr, "afppasswd: can't prepare verifier %s/%s: %s\n",
                path, uid_name, strerror(errno));
        close(fd);
        close(dirfd);
        return -1;
    }

    close(fd);
    close(dirfd);

    if ((pwd = getpwuid(uid)) == NULL) {
        fprintf(stderr, "afppasswd: can't get password entry.\n");
        return -1;
    }

    return update_srp_passwd(path, pwd->pw_name, uid, flags, pass);
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

    if (!(flags & OPT_ISROOT)) {
        fprintf(stderr, "afppasswd: only root can manage RandNum passwords.\n");
        return -1;
    }

    if (!srp_valid_username(name)) {
        fprintf(stderr, "afppasswd: invalid username.\n");
        return -1;
    }

    if (randnum_open_keyfile(path, &keyfd) < 0) {
        return -1;
    }

    fd = open_credential_file(path, O_RDWR);

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

        if (!srp_valid_username(pwd->pw_name) || name_len == sizeof(buf) ||
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
            "Usage (root): afppasswd [-cfmrn] [-a username | -d username] [-p directory] [-u minuid] [-w string]\n");
#else
    fprintf(stderr,
            "Usage (root): afppasswd [-cfmr] [-a username | -d username] [-p directory] [-u minuid] [-w string]\n");
#endif
    fprintf(stderr,
            "Usage (user): afppasswd [-c -p directory] [-f] [-w string]\n");
    fprintf(stderr, "  -a user   add or reset password for the named user\n");
    fprintf(stderr,
            "  -d user   disable the named user's SRP verifier\n");
    fprintf(stderr,
            "  -c        create and initialize the credential store\n");
    fprintf(stderr,
            "  -f        replace an existing Randnum credential file with -r -c, or your own verifier with -c -p\n");
    fprintf(stderr, "  -m        migrate a legacy flat SRP verifier file\n");
    fprintf(stderr, "  -r        use legacy RandNum mode (default is SRP)\n");
#ifdef USE_CRACKLIB
    fprintf(stderr, "  -n        disable password strength check\n");
#endif
    fprintf(stderr, "  -u uid    minimum uid to use, defaults to 100\n");
    fprintf(stderr,
            "  -p path   path to SRP verifier directory (or Randnum password file with -r)\n");
    fprintf(stderr, "  -w string use string as password\n");
}

int main(int argc, char **argv)
{
    struct stat st;
    int flags;
    uid_t uid_min = UID_START, uid;
    const char *path = NULL;
    int adduser_seen = 0, disable_seen = 0, path_seen = 0;
    int password_seen = 0, uid_seen = 0;
    const char *pass = "";
    const char *add_username = NULL;
    const char *disable_username = NULL;
    int i, err = 0;
    extern char *optarg;
    extern int optind;
    uid = getuid();

    /* Permanently discard an inherited setuid-root installation during an
     * upgrade before parsing even a path option. New installations are 0755. */
    if (uid != 0 && geteuid() != uid &&
            (setuid(uid) < 0 || geteuid() != uid)) {
        fprintf(stderr, "afppasswd: can't drop obsolete elevated privileges: %s\n",
                strerror(errno));
        return -1;
    }

    if (initialize_libgcrypt() != 0) {
        return -1;
    }

    flags = (uid == 0) ? OPT_ISROOT : 0;

    while ((i = getopt(argc, argv, AFPPASSWD_OPTSTRING)) != EOF) {
        switch (i) {
        case 'c': /* create and initialize the credential store */
            flags |= OPT_CREATE;
            break;

        case 'a': /* add a new user */
            flags |= OPT_ADDUSER;
            add_username = optarg;
            adduser_seen = 1;
            break;

        case 'd': /* disable an SRP verifier */
            flags |= OPT_DISABLE;
            disable_username = optarg;
            disable_seen = 1;
            break;

        case 'f': /* force an action */
            flags |= OPT_FORCE;
            break;

        case 'm': /* migrate the legacy flat SRP verifier file */
            flags |= OPT_MIGRATE;
            break;

        case 'r': /* legacy RandNum mode */
            flags |= OPT_RANDNUM;
            break;

        case 'u':  /* minimum uid to use. default is 100 */
            if (parse_minimum_uid(optarg, &uid_min) < 0) {
                fprintf(stderr, "afppasswd: invalid minimum uid: %s\n", optarg);
                err++;
            } else {
                uid_seen = 1;
            }

            break;
#ifdef USE_CRACKLIB

        case 'n': /* disable CRACKLIB check */
            flags |= OPT_NOCRACK;
            break;
#endif /* USE_CRACKLIB */

        case 'p': /* path to SRP verifier directory or Randnum password file */
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
     * comes from -a (root adding/updating a user), -d (root disabling a user),
     * or getuid() (a regular user changing their own password). */
    if (err || optind != argc) {
        print_usage();
        return -1;
    }

    /* A regular user may only update that user's own SRP verifier, or
     * bootstrap the private verifier directory of a single-user server with
     * -c -p, where -f and -w are also accepted. The path names no privilege
     * boundary now that inherited setuid privileges are discarded above; the
     * filesystem validates access to it. */
    if (!(flags & OPT_ISROOT) &&
            ((flags & (OPT_ADDUSER | OPT_NOCRACK | OPT_MIGRATE | OPT_DISABLE)) ||
             (flags & OPT_RANDNUM) || adduser_seen || disable_seen || uid_seen ||
             (!(flags & OPT_CREATE) && ((flags & OPT_FORCE) || password_seen)))) {
        fprintf(stderr,
                "ERROR: non-root users may update only their own SRP verifier.\n\n");
        print_usage();
        return -1;
    }

    if (!(flags & OPT_ISROOT) && (flags & OPT_CREATE) && !path_seen) {
        fprintf(stderr,
                "afppasswd: non-root -c requires -p with a private verifier directory; see afppasswd(1).\n");
        return -1;
    }

    if ((flags & OPT_MIGRATE) &&
            ((flags & ~(OPT_ISROOT | OPT_MIGRATE)) ||
             uid_seen || password_seen)) {
        fprintf(stderr,
                "afppasswd: -m accepts only -p; stop afpd before migration.\n");
        print_usage();
        return -1;
    }

    if ((flags & OPT_ADDUSER) && (flags & OPT_DISABLE)) {
        fprintf(stderr, "afppasswd: -a and -d cannot be combined.\n");
        print_usage();
        return -1;
    }

    /* A user's bootstrap keeps -w so that it can be scripted. */
    if ((flags & OPT_CREATE) &&
            ((flags & (OPT_ADDUSER | OPT_DISABLE)) ||
             (password_seen && (flags & OPT_ISROOT)))) {
        fprintf(stderr, "afppasswd: -c cannot be combined with -a, -d, or -w.\n");
        print_usage();
        return -1;
    }

    if (uid_seen && !(flags & OPT_CREATE)) {
        fprintf(stderr, "afppasswd: -u is valid only with -c.\n");
        print_usage();
        return -1;
    }

    if ((flags & OPT_FORCE) && (flags & OPT_ISROOT) &&
            (!(flags & OPT_RANDNUM) || !(flags & OPT_CREATE))) {
        fprintf(stderr, "afppasswd: -f is valid only with -r -c.\n");
        print_usage();
        return -1;
    }

    if (flags & OPT_DISABLE) {
        if ((flags & (OPT_CREATE | OPT_FORCE | OPT_MIGRATE | OPT_RANDNUM |
                      OPT_NOCRACK)) || uid_seen || password_seen) {
            fprintf(stderr, "afppasswd: -d accepts only -p.\n");
            print_usage();
            return -1;
        }
    }

    /* Root running an update must specify the user via -a or -d. */
    if ((flags & OPT_ISROOT) && !(flags & OPT_CREATE) &&
            !(flags & OPT_ADDUSER) && !(flags & OPT_DISABLE) &&
            !(flags & OPT_MIGRATE)) {
        fprintf(stderr,
                "ERROR: root must specify a user with -a or -d username.\n");
        print_usage();
        return -1;
    }

    if (!path_seen) {
        path = (flags & OPT_RANDNUM) ? _PATH_AFPDPWFILE :
               _PATH_AFPSRPVERIFIERPATH;
    }

    /* Validate password length for RandNum mode */
    if ((flags & OPT_RANDNUM) && strnlen(pass, PASSWDLEN + 1) > PASSWDLEN) {
        fprintf(stderr, "afppasswd: max RandNum password length is %d.\n", PASSWDLEN);
        return -1;
    }

    if (flags & OPT_MIGRATE) {
        return afppasswd_migrate_srp(path, 0);
    }

    if (flags & OPT_CREATE) {
        if ((flags & OPT_ISROOT) == 0) {
            return create_private_srp_verifier(path, uid, flags, pass);
        }

        i = lstat(path, &st);

        if (!i && (!(flags & OPT_RANDNUM) || !(flags & OPT_FORCE))) {
            if (!(flags & OPT_RANDNUM) && S_ISREG(st.st_mode)) {
                fprintf(stderr,
                        "afppasswd: %s is a legacy flat SRP verifier file; stop afpd and run 'afppasswd -m -p %s'.\n",
                        path, path);
            } else {
                fprintf(stderr,
                        "afppasswd: credential path already exists.\n");
            }

            return -1;
        }

        if (flags & OPT_RANDNUM) {
            if (randnum_ensure_keyfile(path, flags) < 0) {
                return -1;
            }

            return create_file(path, uid_min);
        } else {
            return create_srp_directory(path, uid_min);
        }
    } else {
        struct passwd *pwd = NULL;
        /* Root specifies the user with -a; non-root users operate on themselves. */
        pwd = (flags & OPT_ISROOT) ?
              getpwnam((flags & OPT_DISABLE) ? disable_username : add_username) :
              getpwuid(uid);

        if (pwd) {
            if (flags & OPT_RANDNUM) {
                return update_passwd(path, pwd->pw_name, flags, pass);
            } else if (flags & OPT_DISABLE) {
                return disable_srp_verifier(path, pwd->pw_name, pwd->pw_uid);
            } else {
                return update_srp_passwd(path, pwd->pw_name, pwd->pw_uid,
                                         flags, pass);
            }
        }

        fprintf(stderr, "afppasswd: can't get password entry.\n");
        return -1;
    }
}
