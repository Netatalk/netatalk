/*
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
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
#include <unistd.h>

#ifdef USE_CRACKLIB
#include <crack.h>
#endif /* USE_CRACKLIB */

#include <gcrypt.h>

#ifndef DES_KEY_SZ
#define DES_KEY_SZ 8
#endif

#include <atalk/logger.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/uam.h>
#include <atalk/constant_time.h>

#define PASSWDLEN 8

static unsigned char seskey[8];
static struct passwd	*randpwd;
static uint8_t         randbuf[8];

/*! hash to a 16-bit number. this will generate completely harmless
 * warnings on 64-bit machines. */
#define randhash(a) (((((unsigned long) a) >> 8) ^ \
		      ((unsigned long)a)) & 0xffff)


#define PASSWD_ILLEGAL '*'
#define HEXPASSWDLEN (DES_KEY_SZ * 2)

/*!
 * @brief Return the value of a hex digit, in either case
 *
 * The call does not check its input, so any other byte gives a meaningless
 * value.
 *
 * @param[in] x  hex digit
 * @returns 0 to 15 for a hex digit
 */
static int unhex(unsigned char x)
{
    return isdigit(x) ? x - '0' : toupper(x) + 10 - 'A';
}

/*!
 * @brief Log a failed libgcrypt call of the RandNum UAM
 *
 * @param[in] op   name of the call
 * @param[in] err  its error code
 * @returns 0 for no error, -1 after logging one
 */
static int randnum_cipher_check(const char *op, gcry_error_t err)
{
    if (!err) {
        return 0;
    }

    LOG(log_error, logtype_uams, "UAM RandNum: %s failed: %s", op,
        gcry_strerror(err));
    return -1;
}

/*!
 * @brief Open the companion file of an afppasswd file, its path plus `.key`
 *
 * A missing `.key` file logs a warning, since the passwords of the file
 * depend on it.
 *
 * @param[in] path     path of the afppasswd file
 * @param[in] pathlen  length of path
 * @returns the descriptor, or -1 for a path too long for the suffix or a failed
 *          open
 */
static int afppasswd_open_keyfile(const char *path, const int pathlen)
{
    char keypath[MAXPATHLEN + 1];
    int keyfd;

    if (pathlen > (int) sizeof(keypath) - 5) {
        LOG(log_warning, logtype_uams,
            "UAM RandNum: afppasswd path \"%s\" is too long to locate "
            "the required companion key file.",
            path);
        return -1;
    }

    strlcpy(keypath, path, sizeof(keypath));
    strlcat(keypath, ".key", sizeof(keypath));
    keyfd = open(keypath, O_RDONLY);

    if (keyfd < 0) {
        LOG(log_warning, logtype_uams,
            "UAM RandNum: required afppasswd key file \"%s\" is unavailable "
            "(%s); passwords in \"%s\" cannot be used securely without it.",
            keypath, strerror(errno), path);
    }

    return keyfd;
}

/*!
 * @brief Read the DES secret from the `.key` file of an afppasswd file
 *
 * The file holds 16 hex digits, with an optional newline. The call wipes its
 * read buffer.
 *
 * @param[in]  keyfd  descriptor of the `.key` file
 * @param[out] key    receives the 8-byte DES secret
 * @returns 0, or -1 for a read error, another length, or a byte that is not hex
 */
static int afppasswd_read_keyfile(int keyfd, uint8_t key[DES_KEY_SZ])
{
    uint8_t encoded[HEXPASSWDLEN + 2];
    ssize_t keylen;

    if (lseek(keyfd, 0, SEEK_SET) < 0) {
        LOG(log_info, logtype_uams, "lseek(keyfd) failed (%s)", strerror(errno));
        explicit_bzero(encoded, sizeof(encoded));
        return -1;
    }

    keylen = read(keyfd, encoded, sizeof(encoded));

    if (keylen < 0) {
        LOG(log_info, logtype_uams, "read(keyfd) failed (%s)", strerror(errno));
        explicit_bzero(encoded, sizeof(encoded));
        return -1;
    }

    if (keylen != HEXPASSWDLEN &&
            (keylen != HEXPASSWDLEN + 1 || encoded[HEXPASSWDLEN] != '\n')) {
        LOG(log_info, logtype_uams,
            "invalid key length in afppasswd key file");
        explicit_bzero(encoded, sizeof(encoded));
        return -1;
    }

    for (int i = 0; i < HEXPASSWDLEN; i++) {
        if (!isxdigit(encoded[i])) {
            LOG(log_info, logtype_uams, "invalid character in afppasswd key file");
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

/*!
 * @brief Warn at setup when the afppasswd file or its `.key` file cannot serve
 * RandNum logins
 *
 * The UAM registers anyway, and its logins fail until the files are in place.
 * A home password file cannot carry a `.key` file, so it always warns.
 *
 * @param[in] obj  AFP object with the passwd file option
 */
static void randnum_warn_passwdfile_key(void *obj)
{
    char *passwdfile = NULL;
    uint8_t key[DES_KEY_SZ];
    int keyfd;
    size_t len = UAM_PASSWD_FILENAME;

    if (uam_afpserver_option(obj, UAM_OPTION_PASSWDOPT,
                             (void *) &passwdfile, &len) < 0) {
        LOG(log_warning, logtype_uams,
            "UAM RandNum: failed to get afppasswd file option; Randnum will register but authentication will fail until the password file and companion key are available.");
        return;
    }

    if (!passwdfile || len == 0) {
        LOG(log_warning, logtype_uams,
            "UAM RandNum: afppasswd file is not configured; Randnum will register but authentication will fail until the password file and companion key are available.");
        return;
    }

    if (*passwdfile == '~') {
        LOG(log_warning, logtype_uams,
            "UAM RandNum: home password files cannot be protected by the "
            "required afppasswd key file; Randnum will register but authentication will fail.");
        return;
    }

    keyfd = afppasswd_open_keyfile(passwdfile, (int) len);

    if (keyfd < 0) {
        LOG(log_warning, logtype_uams,
            "UAM RandNum: Randnum will register but authentication will fail until the required key file is available.");
        return;
    }

    if (afppasswd_read_keyfile(keyfd, key) < 0) {
        LOG(log_warning, logtype_uams,
            "UAM RandNum: Randnum will register but authentication will fail until the required key file is valid.");
    }

    explicit_bzero(key, sizeof(key));
    close(keyfd);
}

/*!
 * @brief Read or write the DES-encrypted password of a user in an afppasswd
 * file
 *
 * The password field holds the hex of the encrypted password. The DES secret
 * comes from the `.key` file, and a star in the field disables the entry.
 * Reading decrypts the password into passwd. Writing encrypts passwd and
 * writes its hex back under a write lock on the line. The call writes the
 * entry itself, without the passwd functions of libc and their static state.
 *
 * Formats:
 * @code
 * password file:  username:password:last login date:failedcount
 * key file:       key (in hex)
 * @endcode
 *
 * @param[in]     pwd      user entry
 * @param[in]     path     afppasswd file
 * @param[in]     pathlen  length of path
 * @param[in,out] passwd   receives the password, or holds the new one to write
 * @param[in]     len      size of passwd
 * @param[in]     set      nonzero to write, 0 to read
 * @returns 0, AFPERR_PARAM for a user without an entry, AFPERR_ACCESS for a
 *          disabled entry or a missing or bad file, or AFPERR_MISC
 */
static int afppasswd(const struct passwd *pwd,
                     const char *path, const int pathlen,
                     unsigned char *passwd, int len,
                     const int set)
{
    uint8_t key[DES_KEY_SZ];
    char buf[MAXPATHLEN + 1], *p;
    FILE *fp;
    unsigned int i, j;
    int keyfd = -1, err = 0;
    off_t pos;
    gcry_cipher_hd_t ctx = NULL;
    gcry_error_t ctxerror;

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        LOG(log_error, logtype_uams,
            "UAM RandNum: libgcrypt versions mismatch. Needs: %s Has: %s",
            UAM_NEED_LIBGCRYPT_VERSION, gcry_check_version(NULL));
        return AFPERR_MISC;
    }

    if ((fp = fopen(path, set ? "r+" : "r")) == NULL) {
        LOG(log_error, logtype_uams, "Failed to open %s", path);
        return AFPERR_ACCESS;
    }

    keyfd = afppasswd_open_keyfile(path, pathlen);

    if (keyfd < 0) {
        err = AFPERR_ACCESS;
        goto afppasswd_done;
    }

    pos = ftell(fp);
    explicit_bzero(buf, sizeof(buf));

    while (fgets(buf, sizeof(buf), fp)) {
        if ((p = strchr(buf, ':'))) {
            if (strlen(pwd->pw_name) == (p - buf) &&
                    strncmp(buf, pwd->pw_name, p - buf) == 0) {
                p++;

                if (*p == PASSWD_ILLEGAL) {
                    LOG(log_info, logtype_uams, "invalid password entry for %s", pwd->pw_name);
                    err = AFPERR_ACCESS;
                    goto afppasswd_done;
                }

                goto afppasswd_found;
            }
        }

        pos = ftell(fp);
        explicit_bzero(buf, sizeof(buf));
    }

    err = AFPERR_PARAM;
    goto afppasswd_done;
afppasswd_found:

    if (!set) {
        /* convert to binary. */
        for (i = j = 0; i < HEXPASSWDLEN; i += 2, j++) {
            p[j] = (unhex(p[i]) << 4) | unhex(p[i + 1]);
        }

        if (j <= DES_KEY_SZ) {
            memset(p + j, 0, HEXPASSWDLEN - j);
        }
    }

    if (afppasswd_read_keyfile(keyfd, key) < 0) {
        err = AFPERR_ACCESS;
        goto afppasswd_done;
    }

    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);

    if (randnum_cipher_check("gcry_cipher_open", ctxerror) < 0) {
        err = AFPERR_ACCESS;
        goto afppasswd_done;
    }

    ctxerror = gcry_cipher_setkey(ctx, key, DES_KEY_SZ);
    explicit_bzero(key, sizeof(key));

    if (randnum_cipher_check("gcry_cipher_setkey", ctxerror) < 0) {
        err = AFPERR_ACCESS;
        goto afppasswd_close_cipher;
    }

    if (set) {
        /* NOTE: this takes advantage of the fact that passwd doesn't
         *       get used after this call if it's being set. */
        ctxerror = gcry_cipher_encrypt(ctx, passwd, len, NULL, 0);

        if (randnum_cipher_check("gcry_cipher_encrypt", ctxerror) < 0) {
            err = AFPERR_ACCESS;
            goto afppasswd_close_cipher;
        }
    } else {
        /* decrypt the password */
        ctxerror = gcry_cipher_decrypt(ctx, p, DES_KEY_SZ, NULL, 0);

        if (randnum_cipher_check("gcry_cipher_decrypt", ctxerror) < 0) {
            err = AFPERR_ACCESS;
            goto afppasswd_close_cipher;
        }
    }

afppasswd_close_cipher:
    gcry_cipher_close(ctx);

    if (err) {
        goto afppasswd_done;
    }

    if (set) {
        const unsigned char hextable[] = "0123456789ABCDEF";
        uint8_t encoded[HEXPASSWDLEN];
        struct flock lock;
        int fd = fileno(fp);

        /* convert to hex password */
        for (i = j = 0; i < DES_KEY_SZ; i++, j += 2) {
            encoded[j] = hextable[(passwd[i] & 0xF0) >> 4];
            encoded[j + 1] = hextable[passwd[i] & 0x0F];
        }

        memcpy(p, encoded, sizeof(encoded));
        /* get exclusive access to the user's password entry. we don't
         * worry so much on reads. in the worse possible case there, the
         * user will just need to re-enter their password. */
        lock.l_type = F_WRLCK;
        lock.l_start = pos;
        lock.l_len = 1;
        lock.l_whence = SEEK_SET;
        fseek(fp, pos, SEEK_SET);
        fcntl(fd, F_SETLKW, &lock);
        fwrite(buf, p - buf + sizeof(encoded), 1, fp);
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        explicit_bzero(encoded, sizeof(encoded));
    } else {
        memcpy(passwd, p, len);
    }

    explicit_bzero(buf, sizeof(buf));
afppasswd_done:

    if (keyfd > -1) {
        close(keyfd);
    }

    fclose(fp);
    return err;
}


/*!
 * @brief Read or write an afppasswd entry as root
 *
 * The call raises the euid to root around afppasswd and drops it again.
 *
 * @param[in]     pwd     user entry
 * @param[in]     file    afppasswd file
 * @param[in,out] passwd  password buffer for afppasswd
 * @param[in]     len     size of passwd
 * @param[in]     set     nonzero to write, 0 to read
 * @returns the result of afppasswd, AFPERR_PARAM for a path over MAXPATHLEN, or
 *          AFPERR_MISC when seteuid fails
 */
static int randpass(const struct passwd *pwd, const char *file,
                    unsigned char *passwd, const int len, const int set)
{
    int i;
    uid_t uid = geteuid();
    i = strlen(file);

    if (i > MAXPATHLEN) {
        return AFPERR_PARAM;
    }

    /* handle afppasswd file. we need to make sure that we're root
     * when we do this. */
    if (uid && (seteuid(0) < 0)) {
        LOG(log_error, logtype_uams, "seteuid(%i) failed (%s)", 0, strerror(errno));
        return AFPERR_MISC;
    }

    i = afppasswd(pwd, file, i, passwd, len, set);

    if (uid && (seteuid(uid) < 0)) {
        LOG(log_error, logtype_uams, "seteuid(%i) failed (%s)", uid, strerror(errno));
        return AFPERR_MISC;
    }

    return i;
}

/*!
 * @brief Start a RandNum login with an 8-byte challenge
 *
 * The user has to pass uam_checkuser, and the call reads the password of the
 * user, the DES secret, from the afppasswd file. The reply holds the session
 * ID and the challenge, which the client encrypts with its password.
 *
 * @param[in]     obj       session object
 * @param[in,out] username  user name from the request, receives the account
 *                          name of a match
 * @param[in]     ulen      size of the username buffer
 * @param[out]    uam_pwd   unused
 * @param[in]     ibuf      UAM data of the request, unused
 * @param[in]     ibuflen   length of ibuf, unused
 * @param[out]    rbuf      reply block
 * @param[out]    rbuflen   reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown or refused user, an
 *          afppasswd error, or AFPERR_PARAM
 */
static int rand_login(void *obj, char *username, int ulen,
                      struct passwd **uam_pwd _U_,
                      char *ibuf _U_, size_t ibuflen _U_,
                      char *rbuf, size_t *rbuflen)
{
    char *passwdfile;
    uint16_t sessid;
    size_t len;
    int err;

    if ((randpwd = uam_getname(obj, username, ulen)) == NULL) {
        /* unknown user */
        return AFPERR_NOTAUTH;
    }

    LOG(log_info, logtype_uams, "randnum/rand2num login: %s", username);

    if (uam_checkuser(obj, randpwd) < 0) {
        return AFPERR_NOTAUTH;
    }

    len = UAM_PASSWD_FILENAME;

    if (uam_afpserver_option(obj, UAM_OPTION_PASSWDOPT,
                             (void *) &passwdfile, &len) < 0) {
        return AFPERR_PARAM;
    }

    if ((err = randpass(randpwd, passwdfile, seskey,
                        sizeof(seskey), 0)) != AFP_OK) {
        return err;
    }

    /* get a random number */
    len = sizeof(randbuf);

    if (uam_afpserver_option(obj, UAM_OPTION_RANDNUM,
                             (void *) randbuf, &len) < 0) {
        return AFPERR_PARAM;
    }

    /* session id is a hashed version of the obj pointer */
    sessid = randhash(obj);
    memcpy(rbuf, &sessid, sizeof(sessid));
    rbuf += sizeof(sessid);
    *rbuflen = sizeof(sessid);
    /* send the random number off */
    memcpy(rbuf, randbuf, sizeof(randbuf));
    *rbuflen += sizeof(randbuf);
    return AFPERR_AUTHCONT;
}


/*!
 * @brief Check the encrypted challenge of a RandNum login
 *
 * The call encrypts the challenge with the password of the user as the DES
 * secret and compares it with the client reply in constant time. The 2-way
 * variant starts the same way.
 *
 * @param[in]  obj      session object
 * @param[out] uam_pwd  receives the user entry after a match
 * @param[in]  ibuf     UAM data of the request, the session ID and the
 *                      encrypted challenge
 * @param[in]  ibuflen  length of ibuf, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns AFP_OK, AFPERR_NOTAUTH for a wrong answer, or AFPERR_PARAM for
 *          another session ID
 */
static int randnum_logincont(void *obj, struct passwd **uam_pwd,
                             char *ibuf, size_t ibuflen _U_,
                             char *rbuf _U_, size_t *rbuflen)
{
    uint16_t sessid;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    *rbuflen = 0;
    memcpy(&sessid, ibuf, sizeof(sessid));

    if (sessid != randhash(obj)) {
        return AFPERR_PARAM;
    }

    ibuf += sizeof(sessid);
    /* encrypt. this saves a little space by using the fact that
     * des can encrypt in-place without side-effects. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    ctxerror = gcry_cipher_setkey(ctx, seskey, DES_KEY_SZ);
    ctxerror = gcry_cipher_encrypt(ctx, randbuf, sizeof(randbuf), NULL, 0);
    gcry_cipher_close(ctx);

    /* test against what the client sent */
    if (atalk_ct_memcmp(randbuf, ibuf, sizeof(randbuf))) {
        /* != */
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_NOTAUTH;
    }

    explicit_bzero(randbuf, sizeof(randbuf));
    *uam_pwd = randpwd;
    return AFP_OK;
}


/*!
 * @brief Check the encrypted challenge of a 2-Way RandNum login and answer the
 * challenge of the client
 *
 * Unlike RandNum, each byte of the DES secret shifts left 1 bit first. After a
 * match the call encrypts the 8-byte challenge of the client and sends it
 * back.
 *
 * @param[in]  obj      session object
 * @param[out] uam_pwd  receives the user entry after a match
 * @param[in]  ibuf     UAM data of the request, the session ID, the encrypted
 *                      challenge, and the challenge of the client
 * @param[in]  ibuflen  length of ibuf, unused
 * @param[out] rbuf     reply block, receives the encrypted challenge of the
 *                      client
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK, AFPERR_NOTAUTH for a wrong answer, or AFPERR_PARAM for
 *          another session ID
 */
static int rand2num_logincont(void *obj, struct passwd **uam_pwd,
                              char *ibuf, size_t ibuflen _U_,
                              char *rbuf, size_t *rbuflen)
{
    uint16_t sessid;
    unsigned int i;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    *rbuflen = 0;
    /* compare session id */
    memcpy(&sessid, ibuf, sizeof(sessid));

    if (sessid != randhash(obj)) {
        return AFPERR_PARAM;
    }

    ibuf += sizeof(sessid);

    /* shift key elements left one bit */
    for (i = 0; i < sizeof(seskey); i++) {
        seskey[i] <<= 1;
    }

    /* encrypt randbuf */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    ctxerror = gcry_cipher_setkey(ctx, seskey, DES_KEY_SZ);
    ctxerror = gcry_cipher_encrypt(ctx, randbuf, sizeof(randbuf), NULL, 0);

    /* test against client's reply */
    if (atalk_ct_memcmp(randbuf, ibuf, sizeof(randbuf))) {
        /* != */
        explicit_bzero(randbuf, sizeof(randbuf));
        gcry_cipher_close(ctx);
        return AFPERR_NOTAUTH;
    }

    ibuf += sizeof(randbuf);
    explicit_bzero(randbuf, sizeof(randbuf));
    /* encrypt client's challenge and send back */
    ctxerror = gcry_cipher_encrypt(ctx, rbuf, sizeof(randbuf), ibuf,
                                   sizeof(randbuf));
    gcry_cipher_close(ctx);
    *rbuflen = sizeof(randbuf);
    *uam_pwd = randpwd;
    return AFP_OK;
}

/*!
 * @brief Change the RandNum password of a logged-in user in the afppasswd file
 *
 * The request holds the old password under DES with the new one as the
 * secret, and the new password under DES with the old one. The call reads the
 * old password from the file, checks both in constant time, and writes the
 * new password. With cracklib a weak new password fails with AFPERR_PWDPOLCY.
 *
 * @param[in]     obj       session object
 * @param[in]     username  user name, unused
 * @param[in]     pwd       user entry
 * @param[in,out] ibuf      the 2 encrypted passwords, decrypted in place and
 *                          wiped
 * @param[in]     ibuflen   length of ibuf, unused
 * @param[out]    rbuf      reply block, unused
 * @param[out]    rbuflen   unused
 * @returns AFP_OK, AFPERR_NOTAUTH for a wrong old password, AFPERR_PWDSAME,
 *          AFPERR_PWDPOLCY, AFPERR_ACCESS, or an afppasswd error
 */
static int randnum_changepw(void *obj, const char *username _U_,
                            struct passwd *pwd, char *ibuf,
                            size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen _U_)
{
    char *passwdfile;
    int err;
    size_t len;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;

    if (!gcry_check_version(GCRYPT_VERSION)) {
        LOG(log_info, logtype_uams, "RandNum: libgcrypt versions mismatch. Need: %s",
            GCRYPT_VERSION);
    }

    if (uam_checkuser(obj, pwd) < 0) {
        return AFPERR_ACCESS;
    }

    len = UAM_PASSWD_FILENAME;

    if (uam_afpserver_option(obj, UAM_OPTION_PASSWDOPT,
                             (void *) &passwdfile, &len) < 0) {
        return AFPERR_PARAM;
    }

    /* old password is encrypted with new password and new password is
     * encrypted with old. */
    if ((err = randpass(pwd, passwdfile, seskey,
                        sizeof(seskey), 0)) != AFP_OK) {
        return err;
    }

    /* use old passwd to decrypt new passwd */
    ibuf += PASSWDLEN; /* new passwd */
    ibuf[PASSWDLEN] = '\0';
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    ctxerror = gcry_cipher_setkey(ctx, seskey, DES_KEY_SZ);
    ctxerror = gcry_cipher_decrypt(ctx, ibuf, PASSWDLEN, NULL, 0);
    gcry_cipher_close(ctx);
    /* now use new passwd to decrypt old passwd */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    ctxerror = gcry_cipher_setkey(ctx, ibuf, DES_KEY_SZ);
    ibuf -= PASSWDLEN; /* old passwd */
    ctxerror = gcry_cipher_decrypt(ctx, ibuf, PASSWDLEN, NULL, 0);
    gcry_cipher_close(ctx);

    if (atalk_ct_memcmp(seskey, ibuf, sizeof(seskey))) {
        err = AFPERR_NOTAUTH;
    } else if (atalk_ct_memcmp(seskey, ibuf + PASSWDLEN, sizeof(seskey)) == 0) {
        err = AFPERR_PWDSAME;
    }

#ifdef USE_CRACKLIB
    else if (FascistCheck(ibuf + PASSWDLEN, _PATH_CRACKLIB)) {
        err = AFPERR_PWDPOLCY;
    }

#endif /* USE_CRACKLIB */

    if (!err) {
        err = randpass(pwd, passwdfile, (unsigned char *)ibuf + PASSWDLEN,
                       sizeof(seskey), 1);
    }

    /* zero out some fields */
    explicit_bzero(seskey, sizeof(seskey));
    explicit_bzero(ibuf, sizeof(seskey)); /* old passwd */
    explicit_bzero(ibuf + PASSWDLEN, sizeof(seskey)); /* new passwd */

    if (err) {
        return err;
    }

    return AFP_OK;
}

/*!
 * @brief Read the user name of an FPLogin request and start a RandNum login
 *
 * The request holds the name as a Pascal string, padded to an even address.
 *
 * @param[in]  obj      session object
 * @param[out] uam_pwd  unused until the continuation
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns the result of rand_login, AFPERR_PARAM for an empty or oversized
 *          name, or AFPERR_MISC
 */
static int randnum_login(void *obj, struct passwd **uam_pwd,
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t len, ulen;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *) &username, &ulen) < 0) {
        return AFPERR_MISC;
    }

    if (ibuflen < 2) {
        return AFPERR_PARAM;
    }

    len = (unsigned char) * ibuf++;
    ibuflen--;

    if (!len || len > ibuflen || len > ulen) {
        return AFPERR_PARAM;
    }

    memcpy(username, ibuf, len);
    ibuf += len;
    ibuflen -= len;
    username[len] = '\0';

    if ((unsigned long) ibuf & 1) { /* pad character */
        ++ibuf;
        ibuflen--;
    }

    return rand_login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Read the user name of an FPLoginExt request and start a RandNum login
 *
 * The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte
 * length.
 *
 * @param[in]  obj      session object
 * @param[in]  uname    user name field of the request
 * @param[out] uam_pwd  unused until the continuation
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns the result of rand_login, AFPERR_PARAM for another name type or a
 *          bad length, or AFPERR_MISC
 */
static int randnum_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
                             char *ibuf, size_t ibuflen,
                             char *rbuf, size_t *rbuflen)
{
    char       *username;
    size_t     len, ulen;
    uint16_t  temp16;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *) &username, &ulen) < 0) {
        return AFPERR_MISC;
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
    return rand_login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Register the 2 RandNum logins and the RandNum password change
 *
 * The call first checks the afppasswd file and its `.key` file through
 * randnum_warn_passwdfile_key. A failed registration removes the earlier
 * ones.
 *
 * @param[in] obj   AFP object with the passwd file option
 * @param[in] path  path of the module
 * @returns 0, or -1 when a registration fails
 */
static int uam_setup(void *obj, const char *path)
{
    randnum_warn_passwdfile_key(obj);

    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "Randnum exchange",
                     randnum_login, randnum_logincont, NULL, randnum_login_ext) < 0) {
        return -1;
    }

    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "2-Way Randnum exchange",
                     randnum_login, rand2num_logincont, NULL, randnum_login_ext) < 0) {
        uam_unregister(UAM_SERVER_LOGIN, "Randnum exchange");
        return -1;
    }

    if (uam_register(UAM_SERVER_CHANGEPW, path, "Randnum Exchange",
                     randnum_changepw) < 0) {
        uam_unregister(UAM_SERVER_LOGIN, "Randnum exchange");
        uam_unregister(UAM_SERVER_LOGIN, "2-Way Randnum exchange");
        return -1;
    }

#if 0
    uam_register(UAM_SERVER_PRINTAUTH, path, "Randnum Exchange", pam_printer);
#endif
    return 0;
}

/*!
 * @brief Unregister the 2 RandNum logins and the RandNum password change
 */
static void uam_cleanup(void)
{
    uam_unregister(UAM_SERVER_LOGIN, "Randnum exchange");
    uam_unregister(UAM_SERVER_LOGIN, "2-Way Randnum exchange");
    uam_unregister(UAM_SERVER_CHANGEPW, "Randnum Exchange");
#if 0
    uam_unregister(UAM_SERVER_PRINTAUTH, "Randnum Exchange");
#endif
}

UAM_MODULE_EXPORT struct uam_export uams_randnum = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};
