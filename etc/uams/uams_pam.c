/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu) 
 * All Rights Reserved.  See COPYRIGHT.
 */
#ifdef USE_PAM
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include <security/pam_appl.h>

#include <atalk/afp.h>
#include <atalk/uam.h>

#define PASSWDLEN 8

/* Static variables used to communicate between the conversation function
 * and the server_login function
 */
static pam_handle_t *pamh = NULL; 
static char *hostname;
static char *PAM_username;
static char *PAM_password;

/* PAM conversation function
 * Here we assume (for now, at least) that echo on means login name, and
 * echo off means password.
 */
static int PAM_conv (int num_msg,
                     const struct pam_message **msg,
                     struct pam_response **resp,
                     void *appdata_ptr) 
{
  struct pam_response *reply;
  int count;
  
#define COPY_STRING(s) (s) ? strdup(s) : NULL
  
  if (num_msg < 1)
    return PAM_CONV_ERR;

  reply = (struct pam_response *) 
    calloc(num_msg, sizeof(struct pam_response));

  if (!reply)
    return PAM_CONV_ERR;

  for (count = 0; count < num_msg; count++) {
    char *string = NULL;

    switch (msg[count]->msg_style) {
    case PAM_PROMPT_ECHO_ON:
      if (!(string = COPY_STRING(PAM_username)))
	goto pam_fail_conv;
      break;
    case PAM_PROMPT_ECHO_OFF:
      if (!(string = COPY_STRING(PAM_password)))
	goto pam_fail_conv;
      break;
    case PAM_TEXT_INFO:
#ifdef PAM_BINARY_PROMPT
    case PAM_BINARY_PROMPT:
#endif
      /* ignore it... */
      break;
    case PAM_ERROR_MSG:
    default:
      goto pam_fail_conv;
    }

    if (string) {  
      reply[count].resp_retcode = 0;
      reply[count].resp = string;
      string = NULL;
    }
  }

  *resp = reply;
  return PAM_SUCCESS;

pam_fail_conv:
  for (count = 0; count < num_msg; count++) {
    if (!reply[count].resp)
      continue;
    switch (msg[count]->msg_style) {
    case PAM_PROMPT_ECHO_OFF:
    case PAM_PROMPT_ECHO_ON:
      free(reply[count].resp);
      break;      
    }
  }
  free(reply);
  return PAM_CONV_ERR;
}

static struct pam_conv PAM_conversation = {
  &PAM_conv,
  NULL
};


/* cleartxt login */
static int pam_login(void *obj, struct passwd **uam_pwd,
		     char *ibuf, int ibuflen,
		     char *rbuf, int *rbuflen)
{
    struct passwd *pwd;
    char *username; 
    int err, len, ulen, PAM_error;

    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
			     (void *) &username, &ulen) < 0)
      return AFPERR_MISC;

    if (uam_afpserver_option(obj, UAM_OPTION_HOSTNAME,
			     (void *) &hostname, NULL) < 0)
      return AFPERR_MISC;

    len = (unsigned char) *ibuf++;
    if ( len > ulen ) {
	return( AFPERR_PARAM );
    }

    memcpy(username, ibuf, len );
    ibuf += len;
    username[ len ] = '\0';
    if ((unsigned long) ibuf & 1)  /* pad character */
      ++ibuf;
    ibuf[ PASSWDLEN ] = '\0';

    if (( pwd = uam_getname(username, ulen)) == NULL ) {
	return AFPERR_PARAM;
    }

    syslog(LOG_INFO, "cleartext login: %s", username);
    PAM_username = username;
    PAM_password = ibuf; /* Set these things up for the conv function */

    err = AFPERR_NOTAUTH;
    PAM_error = pam_start("netatalk", username, &PAM_conversation,
			  &pamh);
    if (PAM_error != PAM_SUCCESS)
      goto login_err;

    pam_set_item(pamh, PAM_TTY, "afpd");
    pam_set_item(pamh, PAM_RHOST, hostname);
    /* use PAM_DISALLOW_NULL_AUTHTOK if passwdminlen > 0 */
    PAM_error = pam_authenticate(pamh,0);
    if (PAM_error != PAM_SUCCESS) {
      if (PAM_error == PAM_MAXTRIES) 
	err = AFPERR_PWDEXPR;
      goto login_err;
    }      

    PAM_error = pam_acct_mgmt(pamh, 0);
    if (PAM_error != PAM_SUCCESS) {
      if (PAM_error == PAM_ACCT_EXPIRED)
	err = AFPERR_PWDEXPR;
#ifdef PAM_AUTHTOKEN_REQD
      else if (PAM_error == PAM_AUTHTOKEN_REQD) 
	err = AFPERR_PWDCHNG;
#endif
      goto login_err;
    }

#ifndef PAM_CRED_ESTABLISH
#define PAM_CRED_ESTABLISH PAM_ESTABLISH_CRED
#endif
    PAM_error = pam_setcred(pamh, PAM_CRED_ESTABLISH);
    if (PAM_error != PAM_SUCCESS)
      goto login_err;

    PAM_error = pam_open_session(pamh, 0);
    if (PAM_error != PAM_SUCCESS)
      goto login_err;

    *uam_pwd = pwd;
    return AFP_OK;

login_err:
    pam_end(pamh, PAM_error);
    pamh = NULL;
    return err;
}

/* logout */
static void pam_logout() {
    pam_close_session(pamh, 0);
    pam_end(pamh, 0);
    pamh = NULL;
}

/* change passwd */
static int pam_changepw(void *obj, char *username,
			struct passwd *pwd, char *ibuf, int ibuflen,
			char *rbuf, int *rbuflen)
{
    char pw[PASSWDLEN + 1];
    pam_handle_t *lpamh;
    uid_t uid;
    int PAM_error;

    /* old password */
    memcpy(pw, ibuf, PASSWDLEN);
    memset(ibuf, 0, PASSWDLEN);
    pw[PASSWDLEN] = '\0';

    /* let's do a quick check for the same password */
    if (memcmp(pw, ibuf + PASSWDLEN, PASSWDLEN) == 0)
      return AFPERR_PWDSAME;

    /* Set these things up for the conv function */
    PAM_username = username;
    PAM_password = pw; 

    PAM_error = pam_start("netatalk", username, &PAM_conversation,
			  &lpamh);
    if (PAM_error != PAM_SUCCESS) 
      return AFPERR_PARAM;
    pam_set_item(lpamh, PAM_TTY, "afpd");
    pam_set_item(lpamh, PAM_RHOST, hostname);

    /* we might need to do this as root */
    uid = geteuid();
    seteuid(0);
    PAM_error = pam_authenticate(lpamh,0);
    if (PAM_error != PAM_SUCCESS) {
      seteuid(uid);
      pam_end(lpamh, PAM_error);
      return AFPERR_NOTAUTH;
    }

    /* new password */
    ibuf += PASSWDLEN;
    PAM_password = ibuf;
    ibuf[PASSWDLEN] = '\0';
    
    /* this really does need to be done as root */
    PAM_error = pam_chauthtok(lpamh, 0);
    seteuid(uid); /* un-root ourselves. */
    memset(pw, 0, PASSWDLEN);
    memset(ibuf, 0, PASSWDLEN);
    if (PAM_error != PAM_SUCCESS) {
      pam_end(lpamh, PAM_error);
      return AFPERR_ACCESS;
    }

    pam_end(lpamh, 0);
    return AFP_OK;
}


/* Printer ClearTxtUAM login */
int pam_printer(start, stop, username, out)
        char    *start, *stop, *username;
	struct papfile	*out;
{
    int PAM_error;
    char	*data, *p, *q;
    char	password[PASSWDLEN + 1] = "\0";
    static const char *loginok = "0\r";

    data = (char *)malloc(stop - start + 1);
    strncpy(data, start, stop - start + 1);

    /* We are looking for the following format in data:
     * (username) (password)
     *
     * Let's hope username doesn't contain ") ("!
     */

    /* Parse input for username in () */
    if ((p = strchr(data, '(' )) == NULL) {
	syslog(LOG_INFO,"Bad Login ClearTxtUAM: username not found in string");
	free(data);
	return(-1);
    }
    p++;
    if ((q = strstr(data, ") (" )) == NULL) {
	syslog(LOG_INFO,"Bad Login ClearTxtUAM: username not found in string");
	free(data);
	return(-1);
    }
    strncpy(username, p, q - p);

    /* Parse input for password in next () */
    p = q + 3;
    if ((q = strrchr(data, ')' )) == NULL) {
	syslog(LOG_INFO,"Bad Login ClearTxtUAM: password not found in string");
	free(data);
	return(-1);
    }
    strncpy(password, p, q - p);

    /* Done copying username and password, clean up */
    free(data);

    PAM_username = username;
    PAM_password = password;

    PAM_error = pam_start("netatalk", username, &PAM_conversation,
                          &pamh);
    if (PAM_error != PAM_SUCCESS) {
	syslog(LOG_INFO, "Bad Login ClearTxtUAM: %s: %s", 
			username, pam_strerror(pamh, PAM_error));
        pam_end(pamh, PAM_error);
        pamh = NULL;
        return(-1);
    }

    pam_set_item(pamh, PAM_TTY, "papd");
    pam_set_item(pamh, PAM_RHOST, hostname);
    PAM_error = pam_authenticate(pamh,0);
    if (PAM_error != PAM_SUCCESS) {
	syslog(LOG_INFO, "Bad Login ClearTxtUAM: %s: %s", 
			username, pam_strerror(pamh, PAM_error));
        pam_end(pamh, PAM_error);
        pamh = NULL;
        return(-1);
    }      

    PAM_error = pam_acct_mgmt(pamh, 0);
    if (PAM_error != PAM_SUCCESS) {
	syslog(LOG_INFO, "Bad Login ClearTxtUAM: %s: %s", 
			username, pam_strerror(pamh, PAM_error));
        pam_end(pamh, PAM_error);
        pamh = NULL;
        return(-1);
    }

    PAM_error = pam_open_session(pamh, 0);
    if (PAM_error != PAM_SUCCESS) {
	syslog(LOG_INFO, "Bad Login ClearTxtUAM: %s: %s", 
			username, pam_strerror(pamh, PAM_error));
        pam_end(pamh, PAM_error);
        pamh = NULL;
        return(-1);
    }

    /* Login successful, but no need to hang onto it,
       so logout immediately */
    append(out, loginok, strlen(loginok));
    syslog(LOG_INFO, "Login ClearTxtUAM: %s", username);
    pam_close_session(pamh, 0);
    pam_end(pamh, 0);
    pamh = NULL;

    return(0);
}


static int uam_setup(const char *path)
{
  if (uam_register(UAM_SERVER_LOGIN, path, "Cleartxt Passwrd", 
		   pam_login, NULL, pam_logout) < 0)
	return -1;

  if (uam_register(UAM_SERVER_CHANGEPW, path, "Cleartxt Passwrd",
		   pam_changepw) < 0) {
	uam_unregister(UAM_SERVER_LOGIN, "Cleartxt Passwrd");
	return -1;
  }

  if (uam_register(UAM_SERVER_PRINTAUTH, path, "ClearTxtUAM",
		   pam_printer) < 0) {
	return -1;
  }

  return 0;
}

static void uam_cleanup(void)
{
  uam_unregister(UAM_SERVER_LOGIN, "Cleartxt Passwrd");
  uam_unregister(UAM_SERVER_CHANGEPW, "Cleartxt Passwrd");
  uam_unregister(UAM_SERVER_PRINTAUTH, "ClearTxtUAM");
}

UAM_MODULE_EXPORT struct uam_export uams_clrtxt = {
  UAM_MODULE_SERVER,
  UAM_MODULE_VERSION,
  uam_setup, uam_cleanup
};

#endif /* USE_PAM */
