/*
 * $Id: uams_pgp.c,v 1.10 2003-06-11 07:26:50 srittau Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu) 
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef UAM_PGP

/* for crypt() */
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <pwd.h>
#include <atalk/logger.h>

#if defined(GNUTLS_DHX)
#include <gnutls/openssl.h>
#elif defined(OPENSSL_DHX)
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/cast.h>
#else /* OPENSSL_DHX */
#include <bn.h>
#include <dh.h>
#include <cast.h>
#endif /* OPENSSL_DHX */

#include <atalk/afp.h>
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
static CAST_KEY castkey;
static u_int8_t randbuf[16];

/* pgp passwd */
static int pgp_login(void *obj, struct passwd **uam_pwd,
		     char *ibuf, int ibuflen,
		     char *rbuf, int *rbuflen)
{
    BIGNUM *bn, *gbn, *pbn;
    u_int16_t sessid;
    int len, i;
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

    if (( pgppwd = uam_getname(name, i)) == NULL ) {
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
			 char *ibuf, int ibuflen, 
			 char *rbuf, int *rbuflen)
{
    unsigned char iv[] = "RJscorat";
#ifdef HAVE_GCRYPT
    GcryMPI *bn1, *bn2, *bn3;
#else /* HAVE_GCRYPT */
    BIGNUM *bn1, *bn2, *bn3;
#endif
    u_int16_t sessid;
    char *p;

    *rbuflen = 0;

    /* check for session id */
    memcpy(&sessid, ibuf, sizeof(sessid));
    if (sessid != pgphash(obj))
      return AFPERR_PARAM;
    ibuf += sizeof(sessid);

#ifdef HAVE_GCRYPT
    {
      GcryCipherHd handle;

      handle = gcry_cipher_open(GCRY_CIPHER_CAST5,
				      GCRY_CIPHER_MODE_CBC, 0);
      if (!handle)
        return AFPERR_PARAM;

      if (gcry_cipher_setkey(handle, &castkey, ) != 0) {
        gcry_cipher_close(handle);
        return AFPERR_PARAM;
      }

      if (gcry_cipher_setiv(handle, iv, sizeof(iv)) != 0) {
        gcry_cipher_close(handle);
        return AFPERR_PARAM;
      }

      if (gcry_cipher_decrypt(handle, rbuf, CRYPT2BUFLEN, ibuf, CRYPT2BUFLEN) != 0) {
        gcry_cipher_close(handle);
        return AFPERR_PARAM;
      }

      gcry_cipher_close(handle);
    }
#else /* HAVE_GCRYPT */
    /* use rbuf as scratch space */
    CAST_cbc_encrypt(ibuf, rbuf, CRYPT2BUFLEN, &castkey,
		     iv, CAST_DECRYPT);
#endif /* HAVE_GCRYPT */

#ifdef HAVE_GCRYPT
    {
      size_t sz;

      bn1 = gcry_mpi_snew(KEYSIZE * 8);
      sz = KEYSIZE;
      gcry_mpi_scan(bn1, GCRYMPI_FMT_STD, rbuf, &sz);
      bn2 = gcry_mpi_snew(sizeof(randbuf) * 8);
      sz = sizeof(randbuf);
      gcry_mpi_scan(bn1, GCRYMPI_FMT_STD, randbuf. &sz);
    }
#else /* HAVE_GCRYPT */
    /* check to make sure that the random number is the same. we
     * get sent back an incremented random number. */
    if (!(bn1 = BN_bin2bn(rbuf, KEYSIZE, NULL)))
      return AFPERR_PARAM;

    if (!(bn2 = BN_bin2bn(randbuf, sizeof(randbuf), NULL))) {
      BN_free(bn1);
      return AFPERR_PARAM;
    }
#endif /* HAVE_GCRYPT */

    /* zero out the random number */
    memset(rbuf, 0, sizeof(randbuf));
    memset(randbuf, 0, sizeof(randbuf));
    rbuf += KEYSIZE;

#ifdef HAVE_GCRYPT
    bn3 = gcry_mpi_snew(0);
    gcry_mpi_sub(bn3, bn1, bn2);
    gcry_mpi_release(bn2);
    gcry_mpi_release(bn1);

    if (gcry_mpi_cmp_ui(bn3, 1UL) != 0) {
      gcry_mpi_release(bn3);
      return AFPERR_PARAM;
    }
    gcry_mpi_release(bn3);
#else /* HAVE_GCRYPT */
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
#endif /* HAVE_GCRYPT */

#ifdef AFS
    if ( kcheckuser(*uam_pwd, rbuf) == 0) {
      *uam_pwd = pgppwd;
      return AFP_OK;
    }
#endif /* AFS */

    rbuf[PASSWDLEN] = '\0';
    p = crypt( rbuf, pgppwd->pw_passwd );
    memset(rbuf, 0, PASSWDLEN);
    if ( strcmp( p, pgppwd->pw_passwd ) == 0 ) {
      *uam_pwd = pgppwd;
      return AFP_OK;
    }

    return AFPERR_NOTAUTH;
}


static int uam_setup(const char *path)
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
