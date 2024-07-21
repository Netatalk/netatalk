/*
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef UAM_PGP

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif /* HAVE_CRYPT_H */

#if defined(EMBEDDED_SSL) || defined(WOLFSSL_DHX)
#if defined(WOLFSSL_DHX)
#include <wolfssl/options.h>
#endif
#include <wolfssl/openssl/bn.h>
#include <wolfssl/openssl/dh.h>
#include <wolfssl/openssl/err.h>
#include <wolfssl/openssl/ssl.h>

#include <nettle/cast128.h>
#include <nettle/cbc.h>

#elif defined(OPENSSL_DHX)
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/cast.h>
#endif /* EMBEDDED_SSL */

#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/standards.h>
#include <atalk/uam.h>

#define KEYSIZE 16
#define PASSWDLEN 64
#define CRYPTBUFLEN  (KEYSIZE*2)
#define CRYPT2BUFLEN (KEYSIZE + PASSWDLEN)

/* hash a number to a 16-bit quantity */
#define pgphash(a) ((((unsigned long) (a) >> 8) ^ \
		     (unsigned long) (a)) & 0xffff)

/* the secret key */
static struct passwd *pgppwd;
#if defined(OPENSSL_DHX)
static CAST_KEY castkey;
#else
struct CBC_CTX(struct cast128_ctx, CAST128_BLOCK_SIZE) castkey;
#endif
static uint8_t randbuf[16];

/* pgp passwd */
static int pgp_login(void *obj, struct passwd **uam_pwd,
		     char *ibuf, size_t ibuflen,
		     char *rbuf, size_t *rbuflen)
{
    size_t len, i;
    char *name;

    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &name, &i) < 0)
      return AFPERR_PARAM;

    len = (unsigned char) *ibuf++;
    if ( len > i ) {
	return( AFPERR_PARAM );
    }

    memcpy(name, ibuf, len );
    ibuf += len;
    name[ len ] = '\0';
    if ((unsigned long) ibuf & 1) /* padding */
      ++ibuf;

    if (( pgppwd = uam_getname(obj, name, i)) == NULL ) {
      return AFPERR_PARAM;
    }

    LOG(log_info, logtype_uams, "pgp login: %s", name);
    if (uam_checkuser(pgppwd) < 0)
      return AFPERR_NOTAUTH;

    /* get the challenge */
    len = (unsigned char) *ibuf++;
    /* challenge */

    /* get the signature. it's always 16 bytes. */
    if (uam_afpserver_option(obj, UAM_OPTION_SIGNATURE,
			     (void *) &name, NULL) < 0) {
      *rbuflen = 0;
      goto pgp_fail;
    }
    memcpy(rbuf + KEYSIZE, name, KEYSIZE);

pgp_fail:
    return AFPERR_PARAM;
}

static int pgp_logincont(void *obj, struct passwd **uam_pwd,
			 char *ibuf, size_t ibuflen,
			 char *rbuf, size_t *rbuflen)
{
	unsigned char iv[] = "RJscorat";
    BIGNUM *bn1, *bn2, *bn3;
    uint16_t sessid;
    char *p;

    *rbuflen = 0;

    /* check for session id */
    memcpy(&sessid, ibuf, sizeof(sessid));
    if (sessid != pgphash(obj))
      return AFPERR_PARAM;
    ibuf += sizeof(sessid);

    /* use rbuf as scratch space */
#if defined(OPENSSL_DHX)
    CAST_cbc_encrypt((unsigned char *)ibuf, (unsigned char *)rbuf, CRYPT2BUFLEN, &castkey,
		     iv, CAST_DECRYPT);
#else
    CBC_SET_IV(&castkey, iv);
    CBC_DECRYPT(&castkey, cast128_decrypt, CRYPT2BUFLEN, (unsigned char *)rbuf, (unsigned char *)ibuf);
#endif

    /* check to make sure that the random number is the same. we
     * get sent back an incremented random number. */
    if (!(bn1 = BN_bin2bn((unsigned char *)rbuf, KEYSIZE, NULL)))
      return AFPERR_PARAM;

    if (!(bn2 = BN_bin2bn(randbuf, sizeof(randbuf), NULL))) {
      BN_free(bn1);
      return AFPERR_PARAM;
    }

    /* zero out the random number */
    memset(rbuf, 0, sizeof(randbuf));
    memset(randbuf, 0, sizeof(randbuf));
    rbuf += KEYSIZE;

    if (!(bn3 = BN_new())) {
      BN_free(bn2);
      BN_free(bn1);
      return AFPERR_PARAM;
    }

    BN_sub(bn3, bn1, bn2);
    BN_free(bn2);
    BN_free(bn1);

    /* okay. is it one more? */
    if (!BN_is_one(bn3)) {
      BN_free(bn3);
      return AFPERR_PARAM;
    }
    BN_free(bn3);

    rbuf[PASSWDLEN] = '\0';
    p = crypt( rbuf, pgppwd->pw_passwd );
    memset(rbuf, 0, PASSWDLEN);
    if ( strcmp( p, pgppwd->pw_passwd ) == 0 ) {
      *uam_pwd = pgppwd;
      return AFP_OK;
    }

    return AFPERR_NOTAUTH;
}


static int uam_setup(void *obj, const char *path)
{
  if (uam_register(UAM_SERVER_LOGIN, path, "PGPuam 1.0",
		   pgp_login, pgp_logincont, NULL) < 0)
    return -1;

  return 0;
}

static void uam_cleanup(void)
{
  uam_unregister(UAM_SERVER_LOGIN, "PGPuam 1.0");
}

UAM_MODULE_EXPORT struct uam_export uams_pgp = {
  UAM_MODULE_SERVER,
  UAM_MODULE_VERSION,
  uam_setup, uam_cleanup
};

#endif /* UAM_PGP */
