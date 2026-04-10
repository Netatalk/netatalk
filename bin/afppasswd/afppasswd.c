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
 * Format: name:hex_password(16):last_login(16):fail_count(8)
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

/* if newpwd is null, convert buf from hex to binary. if newpwd isn't
 * null, convert newpwd to hex and save it in buf. */
#define unhex(x)  (isdigit(x) ? (x) - '0' : toupper(x) + 10 - 'A')
static void convert_passwd(char *buf, char *newpwd, const int keyfd)
{
    uint8_t key[HEXPASSWDLEN];
    unsigned int i, j;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;

    if (!newpwd) {
        /* convert to binary */
        for (i = j = 0; i < sizeof(key); i += 2, j++) {
            buf[j] = (unhex(buf[i]) << 4) | unhex(buf[i + 1]);
        }

        if (j <= DES_KEY_SZ) {
            memset(buf + j, 0, sizeof(key) - j);
        }
    }

    if (keyfd > -1) {
        lseek(keyfd, 0, SEEK_SET);

        if (read(keyfd, key, sizeof(key)) < 0) {
            fprintf(stderr, "could not read key (%s)\n", strerror(errno));
        }

        /* convert to binary */
        for (i = j = 0; i < sizeof(key); i += 2, j++) {
            key[j] = (unhex(key[i]) << 4) | unhex(key[i + 1]);
        }

        if (j <= DES_KEY_SZ) {
            memset(key + j, 0, sizeof(key) - j);
        }

        ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
        ctxerror = gcry_cipher_setkey(ctx, key, DES_KEY_SZ);
        explicit_bzero(key, sizeof(key));

        if (newpwd) {
            ctxerror = gcry_cipher_encrypt(ctx, newpwd, DES_KEY_SZ, NULL, 0);
        } else {
            /* decrypt the password */
            ctxerror = gcry_cipher_decrypt(ctx, buf, DES_KEY_SZ, NULL, 0);
        }

        gcry_cipher_close(ctx);
    }

    if (newpwd) {
        const unsigned char hextable[] = "0123456789ABCDEF";

        /* convert to hex */
        for (i = j = 0; i < DES_KEY_SZ; i++, j += 2) {
            buf[j] = hextable[(newpwd[i] & 0xF0) >> 4];
            buf[j + 1] = hextable[newpwd[i] & 0x0F];
        }
    }
}

/* -------------------- SRP verifier functions -------------------- */

static const unsigned char hextable[] = "0123456789ABCDEF";

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

static int update_srp_passwd(const char *path, const char *name, int flags,
                             const char *pass)
{
    const char *passwd = NULL;
    char password[SRP_PASSWDLEN + 1];
    FILE *fp;
    off_t pos;
    int err = 0;
    const char *p;
    size_t pass_len;
    size_t name_len;
    /* line buffer: username + ":" + hex_salt + ":" + hex_verifier + "\n" + NUL */
    char line[255 + 1 + SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN + 2];
    pass_len = strnlen(pass, SRP_PASSWDLEN + 1);

    if (pass_len > SRP_PASSWDLEN) {
        fprintf(stderr, "afppasswd: max password length is %d.\n", SRP_PASSWDLEN);
        return -1;
    }

    name_len = strnlen(name, sizeof(line));

    if ((fp = fopen(path, "r+")) == NULL) {
        fprintf(stderr, "afppasswd: can't open %s: %s\n", path, strerror(errno));
        return -1;
    }

    /* Search for existing entry */
    pos = ftell(fp);

    while (fgets(line, sizeof(line), fp)) {
        p = strchr(line, ':');

        if (p && name_len == (size_t)(p - line) && strncmp(line, name, name_len) == 0) {
            p++;

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
        /* Append a new placeholder entry */
        fseek(fp, 0, SEEK_END);
        pos = ftell(fp);
        fprintf(fp, "%s:", name);

        /* Write placeholder asterisks */
        for (int i = 0; i < SRP_HEX_SALT_LEN; i++) {
            fputc(PASSWD_ILLEGAL, fp);
        }

        fputc(':', fp);

        for (int i = 0; i < SRP_HEX_V_LEN; i++) {
            fputc(PASSWD_ILLEGAL, fp);
        }

        fputc('\n', fp);
        fflush(fp);
        /* Re-read the line we just wrote */
        fseek(fp, pos, SEEK_SET);

        if (fgets(line, sizeof(line), fp) == NULL) {
            err = -1;
            goto done;
        }

        p = strchr(line, ':');

        if (p == NULL) {
            err = -1;
            goto done;
        }

        p++;
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
        /* Parse existing salt from file */
        unsigned char old_salt[SRP_SALT_LEN];

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
        unsigned char old_v[SRP_NBYTES];

        for (int i = 0; i < SRP_NBYTES; i++) {
            if (!isxdigit(vp[i * 2]) || !isxdigit(vp[i * 2 + 1])) {
                fprintf(stderr, "afppasswd: corrupt verifier file.\n");
                err = -1;
                goto done;
            }

            old_v[i] = (unsigned char)((unhex(vp[i * 2]) << 4) | unhex(vp[i * 2 + 1]));
        }

        /* Recompute verifier from entered password and compare */
        unsigned char check_v[SRP_NBYTES];

        if (srp_compute_verifier(name, passwd, old_salt, check_v) != 0) {
            fprintf(stderr, "afppasswd: internal error computing verifier.\n");
            err = -1;
            goto done;
        }

        if (memcmp(check_v, old_v, SRP_NBYTES) != 0) {
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
        size_t passwd_len = strnlen(passwd, SRP_PASSWDLEN + 1);

        if (passwd_len > SRP_PASSWDLEN) {
            fprintf(stderr, "afppasswd: max password length is %d.\n", SRP_PASSWDLEN);
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

        if (strcmp(passwd, password) != 0) {
            fprintf(stderr, "afppasswd: passwords don't match!\n");
            err = -1;
            goto done;
        }
    }

    /* Generate new salt and compute verifier */
    unsigned char new_salt[SRP_SALT_LEN];
    unsigned char new_v[SRP_NBYTES];
    gcry_randomize(new_salt, SRP_SALT_LEN, GCRY_STRONG_RANDOM);

    if (srp_compute_verifier(name, password, new_salt, new_v) != 0) {
        fprintf(stderr, "afppasswd: failed to compute verifier.\n");
        err = -1;
        goto done;
    }

    /* Encode as hex */
    char hex_buf[SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN];
    srp_encode_hex(hex_buf, new_salt, new_v);
    /* Write to file at the correct offset */
    {
        struct flock lock;
        int fd = fileno(fp);
        lock.l_type = F_WRLCK;
        lock.l_start = pos;
        lock.l_len = 1;
        lock.l_whence = SEEK_SET;
        fseek(fp, pos, SEEK_SET);
        fcntl(fd, F_SETLKW, &lock);
        /* Write: username:hex_salt:hex_verifier\n */
        fprintf(fp, "%s:%.*s\n", name,
                (SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN), hex_buf);
        fflush(fp);
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
    }
    printf("afppasswd: updated SRP verifier.\n");
    explicit_bzero(new_salt, sizeof(new_salt));
    explicit_bzero(new_v, sizeof(new_v));
    explicit_bzero(password, sizeof(password));
done:
    fclose(fp);
    return err;
}

static int create_srp_file(const char *path, uid_t minuid)
{
    struct passwd *pwd;
    int fd, err = 0;

    if ((fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0600)) < 0) {
        fprintf(stderr, "afppasswd: can't create %s\n", path);
        return -1;
    }

    setpwent();

    while ((pwd = getpwent())) {
        if (pwd->pw_uid < minuid) {
            continue;
        }

        /* username + ":" + placeholder salt + ":" + placeholder verifier + "\n" */
        size_t namelen = strnlen(pwd->pw_name, sizeof(buf));

        if (namelen == sizeof(buf) || namelen + SRP_FORMAT_LEN > sizeof(buf) - 1) {
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

/* this matches the code in uam_randnum.c */
static int update_passwd(const char *path, const char *name, int flags,
                         const char *pass)
{
    char password[PASSWDLEN + 1], *p, *passwd = "";
    FILE *fp;
    off_t pos;
    int keyfd = -1, err = 0;
    size_t pass_len;
    size_t name_len;

    if ((fp = fopen(path, "r+")) == NULL) {
        fprintf(stderr, "afppasswd: can't open %s: %s\n", path, strerror(errno));
        return -1;
    }

    /* open the key file if it exists */
    strlcpy(buf, path, sizeof(buf));
    size_t path_len = strnlen(path, sizeof(buf));

    if (path_len < sizeof(buf) - 5) {
        strlcat(buf, ".key", sizeof(buf));
        keyfd = open(buf, O_RDONLY);
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

            if (!(flags & OPT_ISROOT) && (*p == PASSWD_ILLEGAL)) {
                fprintf(stderr, "Your password is disabled. Please see your administrator.\n");
                break;
            }

            goto found_entry;
        }

        pos = ftell(fp);
        memset(buf, 0, sizeof(buf));
    }

    if (flags & OPT_ADDUSER) {
        strlcpy(buf, name, sizeof(buf));
        strlcat(buf, FORMAT, sizeof(buf));
        p = strchr(buf, ':') + 1;
        fwrite(buf, strnlen(buf, sizeof(buf)), 1, fp);
    } else {
        fprintf(stderr, "afppasswd: can't find %s in %s\n", name, path);
        err = -1;
        goto update_done;
    }

found_entry:

    /* need to verify against old password */
    if ((flags & OPT_ISROOT) == 0) {
        passwd = getpass("Enter OLD AFP password: ");
        convert_passwd(p, NULL, keyfd);

        if (strncmp(passwd, p, PASSWDLEN)) {
            fprintf(stderr, "afppasswd: invalid password.\n");
            err = -1;
            goto update_done;
        }
    }

    /* new password */
    if (pass_len < 1) {
        passwd = getpass("Enter NEW AFP password: ");
        size_t passwd_len = strnlen(passwd, PASSWDLEN + 1);

        if (passwd_len > PASSWDLEN) {
            fprintf(stderr, "afppasswd: max password length is %d.\n", PASSWDLEN);
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

    if (strcmp(passwd, password) == 0 || pass_len > 0) {
        struct flock lock;
        int fd = fileno(fp);
        convert_passwd(p, password, keyfd);
        lock.l_type = F_WRLCK;
        lock.l_start = pos;
        lock.l_len = 1;
        lock.l_whence = SEEK_SET;
        fseek(fp, pos, SEEK_SET);
        fcntl(fd, F_SETLKW, &lock);
        fwrite(buf, p - buf + HEXPASSWDLEN, 1, fp);
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        printf("afppasswd: updated password.\n");
    } else {
        fprintf(stderr, "afppasswd: passwords don't match!\n");
        err = -1;
    }

update_done:

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

    if ((fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0600)) < 0) {
        fprintf(stderr, "afppasswd: can't create %s\n", path);
        return -1;
    }

    setpwent();

    while ((pwd = getpwent())) {
        if (pwd->pw_uid < minuid) {
            continue;
        }

        /* a little paranoia */
        size_t name_len = strnlen(pwd->pw_name, sizeof(buf));

        if (name_len == sizeof(buf) || name_len + FORMAT_LEN > sizeof(buf) - 1) {
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
    int path_set = 0;
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
            break;

        case 'f': /* force an action */
            flags |= OPT_FORCE;
            break;

        case 'r': /* legacy RandNum mode */
            flags |= OPT_RANDNUM;
            break;

        case 'u':  /* minimum uid to use. default is 100 */
            uid_min = atoi(optarg);
            break;
#ifdef USE_CRACKLIB

        case 'n': /* disable CRACKLIB check */
            flags |= OPT_NOCRACK;
            break;
#endif /* USE_CRACKLIB */

        case 'p': /* path to password/verifier file */
            path = optarg;
            path_set = 1;
            break;

        case 'w': /* password string */
            pass = optarg;
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

    /* Root running an update must specify the user via -a. */
    if ((flags & OPT_ISROOT) && !(flags & OPT_CREATE) && !(flags & OPT_ADDUSER)) {
        fprintf(stderr,
                "afppasswd: root must specify a user with -a username.\n");
        print_usage();
        return -1;
    }

    /* Set default path based on mode */
    if (!path_set) {
        if (flags & OPT_RANDNUM) {
            path = _PATH_AFPDPWFILE;
        } else {
            path = _PATH_AFPDSRPPWFILE;
        }
    }

    /* Validate password length for RandNum mode */
    if ((flags & OPT_RANDNUM) && strnlen(pass, PASSWDLEN + 1) > PASSWDLEN) {
        fprintf(stderr, "afppasswd: max password length is %d.\n", PASSWDLEN);
        return -1;
    }

    i = stat(path, &st);

    if (flags & OPT_CREATE) {
        if ((flags & OPT_ISROOT) == 0) {
            fprintf(stderr, "afppasswd: only root can create the password file.\n");
            return -1;
        }

        if (!i && ((flags & OPT_FORCE) == 0)) {
            fprintf(stderr, "afppasswd: password file already exists.\n");
            return -1;
        }

        if (flags & OPT_RANDNUM) {
            return create_file(path, uid_min);
        } else {
            return create_srp_file(path, uid_min);
        }
    } else {
        struct passwd *pwd = NULL;

        if (i < 0) {
            fprintf(stderr, "afppasswd: %s doesn't exist.\n", path);
            return -1;
        }

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
