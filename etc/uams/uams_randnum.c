/* 
 * $Id: uams_randnum.c,v 1.14 2003-06-11 07:14:12 srittau Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu) 
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>

/* STDC check */
#if STDC_HEADERS
#include <string.h>
#else /* STDC_HEADERS */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr index
#endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif /* STDC_HEADERS */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <ctype.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <atalk/logger.h>

#include <netatalk/endian.h>

#include <atalk/afp.h>
#include <atalk/uam.h>

#include <des.h>


#ifdef USE_CRACKLIB
#include <crack.h>
#endif /* USE_CRACKLIB */

#include "crypt.h"

#ifndef __inline__
#define __inline__
#endif /* __inline__ */

#define PASSWDLEN 8

static C_Block		seskey;
static struct passwd	*randpwd;
static u_int8_t         randbuf[8];

/* hash to a 16-bit number. this will generate completely harmless 
 * warnings on 64-bit machines. */
#define randhash(a) (((((unsigned long) a) >> 8) ^ \
		      ((unsigned long)a)) & 0xffff)


/* handle ~/.passwd. courtesy of shirsch@ibm.net. */
static  __inline__ int home_passwd(const struct passwd *pwd, 
				   const char *path, const int pathlen, 
				   char *passwd, const int len,
				   const int set)
{
  struct stat st;
  int fd, i;
  
  if ( (fd = open(path, (set) ? O_WRONLY : O_RDONLY)) < 0 ) {
    LOG(log_error, logtype_uams, "Failed to open %s", path);
    return AFPERR_ACCESS;
  }

  if ( fstat( fd, &st ) < 0 ) 
    goto home_passwd_fail;
  
  /* If any of these are true, disallow login: 
   * - not a regular file
   * - gid or uid don't match user
   * - anyone else has permissions of any sort
   */
  if (!S_ISREG(st.st_mode) || (pwd->pw_uid != st.st_uid) ||
      (pwd->pw_gid != st.st_gid) ||
      (st.st_mode & ( S_IRWXG | S_IRWXO )) ) {
    LOG(log_info, logtype_uams, "Insecure permissions found for %s.", path);
    goto home_passwd_fail;
  }

  /* get the password */
  if (set) {
    if (write(fd, passwd, len) < 0) {
      LOG(log_error, logtype_uams, "Failed to write to %s", path );
      goto home_passwd_fail;
    }
  } else {
    if (read(fd, passwd, len) < 0) {
      LOG(log_error, logtype_uams, "Failed to read from %s", path );
      goto home_passwd_fail;
    }
  
  /* get rid of pesky characters */
  for (i = 0; i < len; i++)
    if ((passwd[i] != ' ') && isspace(passwd[i]))
      passwd[i] = '\0';
  }

  close(fd);
  return AFP_OK;

home_passwd_fail:
  close(fd);
  return AFPERR_ACCESS;
}



/* 
 * handle /path/afppasswd with an optional key file. we're a lot more
 * trusting of this file. NOTE: we use our own password entry writing
 * bits as we want to avoid tromping over global variables. in addition,
 * we look for a key file and use that if it's there. here are the 
 * formats: 
 * password file:
 * username:password:last login date:failedcount
 *
 * password is just the hex equivalent of either the ASCII password
 * (if the key file doesn't exist) or the des encrypted password.
 *
 * key file: 
 * key (in hex) */
#define PASSWD_ILLEGAL '*'
static int afppasswd(const struct passwd *pwd, 
		     const char *path, const int pathlen, 
		     char *passwd, int len, 
		     const int set)
{
  u_int8_t key[DES_KEY_SZ*2];
  char buf[MAXPATHLEN + 1], *p;
  FILE *fp;
  int keyfd = -1, err = 0;
  off_t pos;
  
  if ((fp = fopen(path, (set) ? "r+" : "r")) == NULL) {
    LOG(log_error, logtype_uams, "Failed to open %s", path);
    return AFPERR_ACCESS;
  }
  
  /* open the key file if it exists */
  strcpy(buf, path);
  if (pathlen < sizeof(buf) - 5) {
    strcat(buf, ".key");
    keyfd = open(buf, O_RDONLY);
  } 
  
  pos = ftell(fp);
  memset(buf, 0, sizeof(buf));
  while (fgets(buf, sizeof(buf), fp)) {
    if ((p = strchr(buf, ':'))) {
      if (strncmp(buf, pwd->pw_name, p - buf) == 0) {
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
    memset(buf, 0, sizeof(buf));
  }
  err = AFPERR_PARAM;
  goto afppasswd_done;

afppasswd_found:
  if (!set)
    atalk_unhexify(p, sizeof(key), p, sizeof(key));

  if (keyfd > -1) {
      size_t len;

      /* read in the hex representation of an 8-byte key */
      read(keyfd, key, sizeof(key));

      /* convert to binary key */
      len = strlen((char *) key);
      atalk_unhexify(key, len, key, len);

      if (set) {
	/* NOTE: this takes advantage of the fact that passwd doesn't
	 *       get used after this call if it's being set. */
	err = atalk_encrypt(key, passwd, passwd);
      } else {
	err = atalk_decrypt(key, p, p);
      }
      memset(key, 0, sizeof(key));

      if (err)
	goto afppasswd_done;
  }

  if (set) {
    struct flock lock;
    int fd = fileno(fp);

    /* convert to hex password */
    atalk_hexify(key, sizeof(key), passwd, DES_KEY_SZ);
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
  } else 
    memcpy(passwd, p, len);

  memset(buf, 0, sizeof(buf));

afppasswd_done:
  if (keyfd > -1)
    close(keyfd);
  fclose(fp);
  return err;
}


/* this sets the uid. it needs to do slightly different things
 * depending upon whether or not the password is in ~/.passwd
 * or in a global location */
static int randpass(const struct passwd *pwd, const char *file,
		    char *passwd, const int len, const int set) 
{
  int i;
  uid_t uid = geteuid();

  /* Build pathname to user's '.passwd' file */
  i = strlen(file);
  if (*file == '~') {
    char path[MAXPATHLEN + 1];

    if ( (strlen(pwd->pw_dir) + i - 1) > MAXPATHLEN)
    return AFPERR_PARAM;
  
    strcpy(path,  pwd->pw_dir );
    strcat(path, "/" );
    strcat(path, file + 2);
    if (!uid)
      seteuid(pwd->pw_uid); /* change ourselves to the user */
    i = home_passwd(pwd, path, i, passwd, len, set);
    if (!uid)
      seteuid(0); /* change ourselves back to root */
    return i;
  } 

  if (i > MAXPATHLEN)
    return AFPERR_PARAM;

  /* handle afppasswd file. we need to make sure that we're root
   * when we do this. */
  if (uid)
    seteuid(0);
  i = afppasswd(pwd, file, i, passwd, len, set);
  if (uid)
    seteuid(uid);
  return i;
}

  
/* randnum sends an 8-byte number and uses the user's password to
 * check against the encrypted reply. */
static int randnum_login(void *obj, struct passwd **uam_pwd,
			 char *ibuf, int ibuflen,
			 char *rbuf, int *rbuflen)
{
  char *username, *passwdfile;
  u_int16_t sessid;
  int len, ulen, err;
  
  *rbuflen = 0;
  
  if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, 
			   (void *) &username, &ulen) < 0)
    return AFPERR_PARAM;

  len = UAM_PASSWD_FILENAME;
  if (uam_afpserver_option(obj, UAM_OPTION_PASSWDOPT, 
			     (void *) &passwdfile, &len) < 0)
    return AFPERR_PARAM;

  len = (unsigned char) *ibuf++;
  if ( len > ulen ) {
	return AFPERR_PARAM;
  }
  memcpy(username, ibuf, len );
  ibuf += len;
  username[ len ] = '\0';
  if ((unsigned long) ibuf & 1) /* padding */
    ++ibuf;
  
  if (( randpwd = uam_getname(username, ulen)) == NULL )
    return AFPERR_PARAM; /* unknown user */
  
  LOG(log_info, logtype_uams, "randnum/rand2num login: %s", username);
  if (uam_checkuser(randpwd) < 0)
    return AFPERR_NOTAUTH;

  if ((err = randpass(randpwd, passwdfile, seskey,
		      sizeof(seskey), 0)) != AFP_OK)
    return err;

  /* get a random number */
  len = sizeof(randbuf);
  if (uam_afpserver_option(obj, UAM_OPTION_RANDNUM,
			   (void *) randbuf, &len) < 0)
    return AFPERR_PARAM;

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


/* check encrypted reply. we actually setup the encryption stuff
 * here as the first part of randnum and rand2num are identical. */
static int randnum_logincont(void *obj, struct passwd **uam_pwd,
			     char *ibuf, int ibuflen, 
			     char *rbuf, int *rbuflen)
{
  int err = AFP_OK;
  u_int16_t sessid;

  *rbuflen = 0;

  memcpy(&sessid, ibuf, sizeof(sessid));
  if (sessid != randhash(obj))
    return AFPERR_PARAM;

  ibuf += sizeof(sessid);

  err = atalk_encrypt(seskey, randbuf, randbuf);
  memset(seskey, 0, sizeof(seskey));
  if (err)
    return err;

  /* test against what the client sent */
  if (memcmp( randbuf, ibuf, sizeof(randbuf) )) { /* != */
    memset(randbuf, 0, sizeof(randbuf));
    return AFPERR_NOTAUTH;
  }

  memset(randbuf, 0, sizeof(randbuf));
  *uam_pwd = randpwd;
  return err;
}


/* differences from randnum:
 * 1) each byte of the key is shifted left one bit
 * 2) client sends the server a 64-bit number. the server encrypts it
 *    and sends it back as part of the reply.
 */
static int rand2num_logincont(void *obj, struct passwd **uam_pwd,
			      char *ibuf, int ibuflen, 
			      char *rbuf, int *rbuflen)
{
  int err = AFP_OK;
  CryptHandle crypt_handle;
  u_int16_t sessid;
  int i;

  *rbuflen = 0;

  /* compare session id */
  memcpy(&sessid, ibuf, sizeof(sessid));
  if (sessid != randhash(obj))
    return AFPERR_PARAM;

  ibuf += sizeof(sessid);

  /* shift key elements left one bit */
  for (i = 0; i < sizeof(seskey); i++)
    seskey[i] <<= 1;

  /* encrypt randbuf */
  err = atalk_encrypt_start(&crypt_handle, seskey);
  atalk_encrypt_do(crypt_handle, randbuf, randbuf);

  /* test against client's reply */
  if (memcmp(randbuf, ibuf, sizeof(randbuf))) { /* != */
    memset(randbuf, 0, sizeof(randbuf));
    return AFPERR_NOTAUTH;
  }
  ibuf += sizeof(randbuf);
  memset(randbuf, 0, sizeof(randbuf));

  /* encrypt client's challenge and send back */
  atalk_encrypt_do(crypt_handle, rbuf, ibuf);
  atalk_encrypt_end(crypt_handle);
  memset(seskey, 0, sizeof(seskey));

  *rbuflen = sizeof(randbuf);
  
  *uam_pwd = randpwd;
  return AFP_OK;
}

/* change password  --
 * NOTE: an FPLogin must already have completed successfully for this
 *       to work. 
 */
static int randnum_changepw(void *obj, const char *username, 
			    struct passwd *pwd, char *ibuf,
			    int ibuflen, char *rbuf, int *rbuflen)
{
    char *passwdfile;
    int err, len;

    if (uam_checkuser(pwd) < 0)
      return AFPERR_ACCESS;

    len = UAM_PASSWD_FILENAME;
    if (uam_afpserver_option(obj, UAM_OPTION_PASSWDOPT, 
			     (void *) &passwdfile, &len) < 0)
      return AFPERR_PARAM;

    /* old password is encrypted with new password and new password is
     * encrypted with old. */
    if ((err = randpass(pwd, passwdfile, seskey, 
			sizeof(seskey), 0)) != AFP_OK)
      return err;

    /* use old passwd to decrypt new passwd */
    ibuf += PASSWDLEN; /* new passwd */
    ibuf[PASSWDLEN] = '\0';
    err = atalk_decrypt(seskey, ibuf, ibuf);
    if (err)
      return err;

    /* now use new passwd to decrypt old passwd */
    ibuf -= PASSWDLEN; /* old passwd */
    err = atalk_decrypt(ibuf, ibuf, ibuf);
    if (err)
      return err;
    if (memcmp(seskey, ibuf, sizeof(seskey))) 
	err = AFPERR_NOTAUTH;
    else if (memcmp(seskey, ibuf + PASSWDLEN, sizeof(seskey)) == 0)
        err = AFPERR_PWDSAME;
#ifdef USE_CRACKLIB
    else if (FascistCheck(ibuf + PASSWDLEN, _PATH_CRACKLIB))
        err = AFPERR_PWDPOLCY;
#endif /* USE_CRACKLIB */

    if (!err) 
      err = randpass(pwd, passwdfile, ibuf + PASSWDLEN, sizeof(seskey), 1);

    /* zero out some fields */
    memset(seskey, 0, sizeof(seskey));
    memset(ibuf, 0, sizeof(seskey)); /* old passwd */
    memset(ibuf + PASSWDLEN, 0, sizeof(seskey)); /* new passwd */

    if (err)
      return err;

  return( AFP_OK );
}

static int uam_setup(const char *path)
{
  if (uam_register(UAM_SERVER_LOGIN, path, "Randnum exchange", 
		   randnum_login, randnum_logincont, NULL) < 0)
    return -1;
  if (uam_register(UAM_SERVER_LOGIN, path, "2-Way Randnum exchange",
		   randnum_login, rand2num_logincont, NULL) < 0) {
    uam_unregister(UAM_SERVER_LOGIN, "Randnum exchange");
    return -1;
  }
    
  if (uam_register(UAM_SERVER_CHANGEPW, path, "Randnum Exchange", 
		   randnum_changepw) < 0) {
    uam_unregister(UAM_SERVER_LOGIN, "Randnum exchange");
    uam_unregister(UAM_SERVER_LOGIN, "2-Way Randnum exchange");
    return -1;
  }
  /*uam_register(UAM_SERVER_PRINTAUTH, path, "Randnum Exchange",
    pam_printer);*/

  return 0;
}

static void uam_cleanup(void)
{
  uam_unregister(UAM_SERVER_LOGIN, "Randnum exchange");
  uam_unregister(UAM_SERVER_LOGIN, "2-Way Randnum exchange");
  uam_unregister(UAM_SERVER_CHANGEPW, "Randnum Exchange");
  /*uam_unregister(UAM_SERVER_PRINTAUTH, "Randnum Exchange");*/
}

UAM_MODULE_EXPORT struct uam_export uams_randnum = {
  UAM_MODULE_SERVER,
  UAM_MODULE_VERSION,
  uam_setup, uam_cleanup
};
