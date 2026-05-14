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
#include <atalk/uam.h>

#include "uam_common.h"

#define PASSWDLEN 8

static unsigned char seskey[8];

static struct passwd	*randpwd;
static uint8_t         randbuf[8];

/*! hash to a 16-bit number. this will generate completely harmless
 * warnings on 64-bit machines. */
#define randhash(a) (((((unsigned long) a) >> 8) ^ \
		      ((unsigned long)a)) & 0xffff)


#define PASSWD_ILLEGAL '*'
#define unhex(x)  (isdigit(x) ? (x) - '0' : toupper(x) + 10 - 'A')

static int randnum_cipher_check(const char *op, gcry_error_t err)
{
    if (!err) {
        return 0;
    }

    LOG(log_error, logtype_uams, "UAM RandNum: %s failed: %s", op,
        gcry_strerror(err));
    return -1;
}

static int afppasswd_open_keyfile(const char *path, const int pathlen)
{
    char keypath[MAXPATHLEN + 1];
    int keyfd;

    if (pathlen > (int) sizeof(keypath) - 5) {
        LOG(log_error, logtype_uams,
            "UAM RandNum: afppasswd path \"%s\" is too long to locate "
            "the required companion key file; refusing to use Randnum.",
            path);
        return -1;
    }

    strlcpy(keypath, path, sizeof(keypath));
    strlcat(keypath, ".key", sizeof(keypath));
    keyfd = open(keypath, O_RDONLY);

    if (keyfd < 0) {
        LOG(log_error, logtype_uams,
            "UAM RandNum: required afppasswd key file \"%s\" is unavailable "
            "(%s); refusing to use Randnum because passwords in \"%s\" "
            "would otherwise be stored and used in clear text.",
            keypath, strerror(errno), path);
    }

    return keyfd;
}

static int randnum_check_passwdfile_key(void *obj)
{
    char *passwdfile = NULL;
    int keyfd;
    size_t len = UAM_PASSWD_FILENAME;

    if (uam_afpserver_option(obj, UAM_OPTION_PASSWDOPT,
                             (void *) &passwdfile, &len) < 0) {
        LOG(log_error, logtype_uams,
            "UAM RandNum: failed to get afppasswd file option; refusing to load Randnum.");
        return -1;
    }

    if (!passwdfile || len == 0) {
        LOG(log_error, logtype_uams,
            "UAM RandNum: afppasswd file is not configured; refusing to load Randnum.");
        return -1;
    }

    if (*passwdfile == '~') {
        LOG(log_error, logtype_uams,
            "UAM RandNum: home password files cannot be protected by the "
            "required afppasswd key file; refusing to load Randnum.");
        return -1;
    }

    keyfd = afppasswd_open_keyfile(passwdfile, (int) len);

    if (keyfd < 0) {
        return -1;
    }

    close(keyfd);
    return 0;
}

/*!
 * @brief handle /path/afppasswd with a required key file.
 * we're a lot more trusting of this file.
 * @note we use our own password entry writing bits
 * as we want to avoid tromping over global variables.
 * in addition, we require a key file and fail if it is not available.
 *
 * here are the formats:
 *
 * password file
 * -------------
 * @code
 * username:password:last login date:failedcount
 * @endcode
 *
 * password is just the hex equivalent of the DES encrypted password.
 *
 * key file
 * --------
 * @code
 * key (in hex)
 * @endcode
 */
static int afppasswd(const struct passwd *pwd,
                     const char *path, const int pathlen,
                     unsigned char *passwd, int len,
                     const int set)
{
    uint8_t key[DES_KEY_SZ * 2];
    char buf[MAXPATHLEN + 1], *p;
    FILE *fp;
    unsigned int i, j;
    int keyfd = -1, err = 0;
    ssize_t keylen;
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
        for (i = j = 0; i < sizeof(key); i += 2, j++) {
            p[j] = (unhex(p[i]) << 4) | unhex(p[i + 1]);
        }

        if (j <= DES_KEY_SZ) {
            memset(p + j, 0, sizeof(key) - j);
        }
    }

    /* read in the hex representation of an 8-byte key */
    keylen = read(keyfd, key, sizeof(key));

    if (keylen < 0) {
        LOG(log_info, logtype_uams, "read(keyfd) failed (%s)", strerror(errno));
        err = AFPERR_ACCESS;
        goto afppasswd_done;
    }

    if (keylen != sizeof(key)) {
        LOG(log_info, logtype_uams, "invalid key length in afppasswd key file");
        err = AFPERR_ACCESS;
        goto afppasswd_done;
    }

    for (i = 0; i < sizeof(key); i++) {
        if (!isxdigit(key[i])) {
            LOG(log_info, logtype_uams, "invalid character in afppasswd key file");
            err = AFPERR_ACCESS;
            goto afppasswd_done;
        }
    }

    /* convert to binary key */
    for (i = j = 0; i < sizeof(key); i += 2, j++) {
        key[j] = (unhex(key[i]) << 4) | unhex(key[i + 1]);
    }

    if (j <= DES_KEY_SZ) {
        memset(key + j, 0, sizeof(key) - j);
    }

    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);

    if (randnum_cipher_check("gcry_cipher_open", ctxerror) < 0) {
        err = AFPERR_ACCESS;
        goto afppasswd_done;
    }

    ctxerror = gcry_cipher_setkey(ctx, key, DES_KEY_SZ);

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
        struct flock lock;
        int fd = fileno(fp);

        /* convert to hex password */
        for (i = j = 0; i < DES_KEY_SZ; i++, j += 2) {
            key[j] = hextable[(passwd[i] & 0xF0) >> 4];
            key[j + 1] = hextable[passwd[i] & 0x0F];
        }

        memcpy(p, key, sizeof(key));
        /* get exclusive access to the user's password entry. we don't
         * worry so much on reads. in the worse possible case there, the
         * user will just need to re-enter their password. */
        lock.l_type = F_WRLCK;
        lock.l_start = pos;
        lock.l_len = 1;
        lock.l_whence = SEEK_SET;
        fseek(fp, pos, SEEK_SET);
        fcntl(fd, F_SETLKW, &lock);
        fwrite(buf, p - buf + sizeof(key), 1, fp);
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
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
 * @brief this sets the uid.
 * @note the afppasswd file must be read and updated as root.
 */
static int randpass(const struct passwd *pwd, const char *file,
                    unsigned char *passwd, const int len, const int set)
{
    int i;
    uid_t uid = geteuid();
    i = strlen(file);

    if (*file == '~') {
        char path[MAXPATHLEN + 1];

        if ((strlen(pwd->pw_dir) + i - 1) > MAXPATHLEN) {
            return AFPERR_PARAM;
        }

        strcpy(path,  pwd->pw_dir);
        strcat(path, "/");
        strcat(path, file + 2);

        /* change ourselves to the user */
        if (!uid && (seteuid(pwd->pw_uid) < 0)) {
            LOG(log_error, logtype_uams, "seteuid(%i) failed (%s)", pwd->pw_uid,
                strerror(errno));
            return AFPERR_MISC;
        }

        i = home_passwd(pwd, path, i, passwd, len, set);

        /* change ourselves back to root */
        if (!uid && (seteuid(0) < 0)) {
            LOG(log_error, logtype_uams, "seteuid(%i) failed (%s)", 0, strerror(errno));
            return AFPERR_MISC;
        }

        return i;
    }

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
 * randnum sends an 8-byte number and uses the user's password to
 * check against the encrypted reply.
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
 * @brief check encrypted reply.
 * @note we actually setup the encryption stuff here
 * as the first part of randnum and rand2num are identical.
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
    if (uam_ct_memcmp(randbuf, ibuf, sizeof(randbuf))) {
        /* != */
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_NOTAUTH;
    }

    explicit_bzero(randbuf, sizeof(randbuf));
    *uam_pwd = randpwd;
    return AFP_OK;
}


/*! differences from randnum:
 * 1. each byte of the key is shifted left one bit
 * 2. client sends the server a 64-bit number. the server encrypts it
 *    and sends it back as part of the reply.
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
    if (uam_ct_memcmp(randbuf, ibuf, sizeof(randbuf))) {
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
 * @brief change password
 * @note an FPLogin must already have completed successfully for this
 *       to work.
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

    if (uam_ct_memcmp(seskey, ibuf, sizeof(seskey))) {
        err = AFPERR_NOTAUTH;
    } else if (uam_ct_memcmp(seskey, ibuf + PASSWDLEN, sizeof(seskey)) == 0) {
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

/*! randnum login */
static int randnum_login(void *obj, struct passwd **uam_pwd,
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t ulen;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *) &username, &ulen) < 0) {
        return AFPERR_MISC;
    }

    if (uam_extract_username_v1(&ibuf, &ibuflen, username, ulen) < 0) {
        return AFPERR_PARAM;
    }

    return rand_login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/*! randnum login ext */
static int randnum_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
                             char *ibuf, size_t ibuflen,
                             char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t ulen;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *) &username, &ulen) < 0) {
        return AFPERR_MISC;
    }

    if (uam_extract_username_v2(uname, username, ulen) < 0) {
        return AFPERR_PARAM;
    }

    return rand_login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

static int uam_setup(void *obj, const char *path)
{
    if (randnum_check_passwdfile_key(obj) < 0) {
        return -1;
    }

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
