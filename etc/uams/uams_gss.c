/*
 * $Id: uams_gss.c,v 1.1 2003-08-22 17:12:45 samnoble Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu) 
 * Copyright (c) 2003 The Reed Institute
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef ATACC
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

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

#include <atalk/logger.h>

// #include <security/pam_appl.h>

#include <atalk/afp.h>
#include <atalk/uam.h>

#include <gssapi/gssapi.h>
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>

/* The following routine is derived from code found in some GSS
 * documentation from SUN.
 */
static void log_status( char *s, OM_uint32 major_status, 
		        OM_uint32 minor_status )
{
    gss_buffer_desc msg = GSS_C_EMPTY_BUFFER;
    OM_uint32 min_status, maj_status;
    OM_uint32 maj_ctx = 0, min_ctx = 0;

    while (1) {
	maj_status = gss_display_status( &min_status, major_status,
					GSS_C_GSS_CODE, GSS_C_NULL_OID,
					&maj_ctx, &msg );
        LOG(log_info, logtype_uams, "uams_gss.c :do_gss_auth: %s %.*s (error %s)", s,
	    			(int)msg.length, msg.value, strerror(errno));
	gss_release_buffer(&min_status, &msg);

	if (!maj_ctx)
	    break;
    }
    while (1) {
	maj_status = gss_display_status( &min_status, minor_status,
					GSS_C_MECH_CODE, GSS_C_NULL_OID, // gss_mech_krb5,
					&min_ctx, &msg );
        LOG(log_info, logtype_uams, "uams_gss.c :do_gss_auth: %s %.*s (error %s)", s, 
	    			(int)msg.length, msg.value, strerror(errno));
	gss_release_buffer(&min_status, &msg);

	if (!min_ctx)
	    break;
    }
}


void log_ctx_flags( OM_uint32 flags )
{
    if (flags & GSS_C_DELEG_FLAG)
        LOG(log_debug, logtype_uams, "uams_gss.c :context flag: GSS_C_DELEG_FLAG" );
    if (flags & GSS_C_MUTUAL_FLAG)
        LOG(log_debug, logtype_uams, "uams_gss.c :context flag: GSS_C_MUTUAL_FLAG" );
    if (flags & GSS_C_REPLAY_FLAG)
        LOG(log_debug, logtype_uams, "uams_gss.c :context flag: GSS_C_REPLAY_FLAG" );
    if (flags & GSS_C_SEQUENCE_FLAG)
        LOG(log_debug, logtype_uams, "uams_gss.c :context flag: GSS_C_SEQUENCE_FLAG" );
    if (flags & GSS_C_CONF_FLAG)
        LOG(log_debug, logtype_uams, "uams_gss.c :context flag: GSS_C_CONF_FLAG" );
    if (flags & GSS_C_INTEG_FLAG)
        LOG(log_debug, logtype_uams, "uams_gss.c :context flag: GSS_C_INTEG_FLAG" );
}
/* We work around something I don't entirely understand... */
#if !defined (GSS_C_NT_HOSTBASED_SERVICE)
#define GSS_C_NT_HOSTBASED_SERVICE gss_nt_service_name
#endif

#define MIN(a, b) ((a > b) ? b : a)
/* return 0 on success */
static int do_gss_auth( char *service, char *ibuf, int ticket_len,
	       	 char *rbuf, int *rbuflen, char *username, int ulen ) 
{
    OM_uint32 major_status = 0, minor_status = 0;
    gss_name_t server_name;
    gss_cred_id_t server_creds;
    gss_ctx_id_t context_handle = GSS_C_NO_CONTEXT;
    gss_buffer_desc ticket_buffer, authenticator_buff;
    gss_name_t client_name;
    OM_uint32	ret_flags;
    int ret = 0;
    gss_buffer_desc s_princ_buffer;

    s_princ_buffer.value = service;
    s_princ_buffer.length = strlen( service ) + 1;
    
    LOG(log_debug, logtype_uams, "uams_gss.c :do_gss_auth: importing name" );
    major_status = gss_import_name( &minor_status, 
		    &s_princ_buffer, 
		    GSS_C_NT_HOSTBASED_SERVICE,
		    &server_name );
    if (major_status != GSS_S_COMPLETE) {
	log_status( "import_name", major_status, minor_status );
	ret = 1;
	goto cleanup_vars;
    }

    LOG(log_debug, logtype_uams, 
	"uams_gss.c :do_gss_auth: acquiring credentials (uid = %d, keytab = %s)",
        (int)geteuid(), getenv( "KRB5_KTNAME") );
    major_status = gss_acquire_cred( &minor_status, server_name, 
			GSS_C_INDEFINITE, GSS_C_NO_OID_SET, GSS_C_ACCEPT,
		        &server_creds, NULL, NULL );	
    if (major_status != GSS_S_COMPLETE) {
	log_status( "acquire_cred", major_status, minor_status );
	ret = 1;
	goto cleanup_vars;
    }
    
    /* The GSSAPI docs say that this should be done in a big "do" loop,
     * but Apple's implementation doesn't seem to support this behavior.
     */
    ticket_buffer.length = ticket_len;
    ticket_buffer.value = ibuf;
    authenticator_buff.length = 0;
    authenticator_buff.value = NULL;
    LOG(log_debug, logtype_uams, "uams_gss.c :do_gss_auth: accepting context" );
    major_status = gss_accept_sec_context( &minor_status, &context_handle,
		    	server_creds, &ticket_buffer, GSS_C_NO_CHANNEL_BINDINGS,
			&client_name, NULL, &authenticator_buff,
			&ret_flags, NULL, NULL );

    if (major_status == GSS_S_COMPLETE) {
	gss_buffer_desc client_name_buffer;

	log_ctx_flags( ret_flags );
	/* use gss_display_name on client_name */
	major_status = gss_display_name( &minor_status, client_name,
				&client_name_buffer, (gss_OID *)NULL );
	if (major_status == GSS_S_COMPLETE) {
	    u_int16_t auth_len = htons( authenticator_buff.length );
	    /* save the username... note that doing it this way is
	     * not the best idea: if a principal is truncated, a user could be
	     * impersonated
	     */
	    memcpy( username, client_name_buffer.value, 
		MIN(client_name_buffer.length, ulen - 1));
	    username[MIN(client_name_buffer.length, ulen - 1)] = 0;
	
            LOG(log_debug, logtype_uams, "uams_gss.c :do_gss_auth: user is %s!", username );
	    /* copy the authenticator length into the reply buffer */
	    memcpy( rbuf, &auth_len, sizeof(auth_len) );
	    *rbuflen += sizeof(auth_len), rbuf += sizeof(auth_len);

	    /* copy the authenticator value into the reply buffer */
	    memcpy( rbuf, authenticator_buff.value, authenticator_buff.length );
	    *rbuflen += authenticator_buff.length;

	    gss_release_buffer( &minor_status, &client_name_buffer );
	} else {
	    log_status( "display_name", major_status, minor_status );
	    ret = 1;
	}


	/* Clean up after ourselves */
        gss_release_name( &minor_status, &client_name );
        gss_release_buffer( &minor_status, &authenticator_buff );
        gss_delete_sec_context( &minor_status, 
			&context_handle, NULL );
    } else {
	log_status( "accept_sec_context", major_status, minor_status );
        ret = 1;
    }
    gss_release_cred( &minor_status, &server_creds );

cleanup_vars:
    gss_release_name( &minor_status, &server_name );
    
    return ret;
}

/* -------------------------- */
static int gss_login(void *obj, struct passwd **uam_pwd,
		     char *ibuf, int ibuflen,
		     char *rbuf, int *rbuflen)
{

    u_int16_t  temp16;

    *rbuflen = 0;

    /* The reply contains a two-byte ID value - note 
     * that Apple's implementation seems to always return 1 as well
     */
    temp16 = htons( 1 );
    memcpy(rbuf, &temp16, sizeof(temp16));
    *rbuflen += sizeof(temp16);
    return AFPERR_AUTHCONT;
}

static int gss_logincont(void *obj, struct passwd **uam_pwd,
                     char *ibuf, int ibuflen,
                     char *rbuf, int *rbuflen)
{
    struct passwd *pwd = NULL;
    u_int16_t login_id;
    char *username;
    u_int16_t ticket_len;
    char *p;
    int rblen;
    char *service;
    int userlen, servicelen;

    /* Apple's AFP 3.1 documentation specifies that this command
     * takes the following format:
     * pad (byte)
     * id returned in LoginExt response (u_int16_t)
     * username (format unspecified) padded, when necessary, to end on an even boundary
     * ticket length (u_int16_t)
     * ticket
     */

    /* Observation of AFP clients in the wild indicate that the actual
     * format of this request is as follows:
     * pad (byte) [consumed before login_ext is called]
     * ?? (byte) - always observed to be 0
     * id returned in LoginExt response (u_int16_t)
     * username, encoding unspecified, null terminated C string, 
     *   padded when the terminating null is an even numbered byte.
     *   The packet is formated such that the username begins on an 
     *   odd numbered byte. Eg if the username is 3 characters and the
     *   terminating null makes 4, expect to pad the the result.
     *   The encoding of this string is unknown.
     * ticket length (u_int16_t)
     * ticket
     */

    rblen = *rbuflen = 0;

    ibuf++, ibuflen--; /* ?? */

    /* 2 byte ID from LoginExt -- always '00 01' in this implementation */
    memcpy( &login_id, ibuf, sizeof(login_id) );
    ibuf += sizeof(login_id), ibuflen -= sizeof(login_id);
    login_id = ntohs( login_id );

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username, &userlen) < 0)
        return AFPERR_MISC;

    if (uam_afpserver_option(obj, UAM_OPTION_KRB5SERVICE, (void *)&service, &servicelen) < 0)
	return AFPERR_MISC;

    if (service == NULL) 
	return AFPERR_MISC;

    /* We skip past the 'username' parameter because all that matters is the ticket */
    p = ibuf;
    while( *ibuf && ibuflen ) { ibuf++, ibuflen--; }
    if (ibuflen < 4) {
        LOG(log_debug, logtype_uams, "uams_gss.c :LoginCont: user is %s, no ticket", p);
	return AFPERR_PARAM;
    }

    ibuf++, ibuflen--; /* null termination */

    if ((ibuf - p + 1) % 2) ibuf++, ibuflen--; /* deal with potential padding */

    LOG(log_debug, logtype_uams, "uams_gss.c :LoginCont: client thinks user is %s", p);

    memcpy(&ticket_len, ibuf, sizeof(ticket_len));
    ibuf += sizeof(ticket_len); ibuflen -= sizeof(ticket_len);
    ticket_len = ntohs( ticket_len );

    if (ticket_len > ibuflen) {
        LOG(log_info, logtype_uams, "uams_gss.c :LoginCont: invalid ticket length");
	return AFPERR_PARAM;
    }

    if (!do_gss_auth(service, ibuf, ticket_len, rbuf, &rblen, username, userlen)) {
	char *at = strchr( username, '@' );

	// Chop off the realm name
	if (at)
	    *at = '\0';
	if((pwd = uam_getname( username, userlen )) == NULL) {
	    LOG(log_info, logtype_uams, "uam_getname() failed for %s", username);
	    return AFPERR_PARAM;
	}
	if (uam_checkuser(pwd) < 0) {
	    LOG(log_info, logtype_uams, "%s not a valid user", username);
	    return AFPERR_NOTAUTH;
	}
	*rbuflen = rblen;
	*uam_pwd = pwd;
	return AFP_OK;
    } else {
	LOG(log_info, logtype_uams, "do_gss_auth failed" );
	*rbuflen = 0;
        return AFPERR_MISC;
    }
}

/*
 * For the krb5 uam, this function only needs to return a two-byte
 * login-session id. None of the data provided by the client up to this
 * point is trustworthy as we'll have a signed ticket to parse in logincont.
 */
static int gss_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
		     char *ibuf, int ibuflen,
		     char *rbuf, int *rbuflen)
{
    u_int16_t  temp16;

    *rbuflen = 0;

    /* The reply contains a two-byte ID value - note 
     * that Apple's implementation seems to always return 1 as well
     */
    temp16 = htons( 1 );
    memcpy(rbuf, &temp16, sizeof(temp16));
    *rbuflen += sizeof(temp16);
    return AFPERR_AUTHCONT;
}

/* logout */
static void gss_logout() {
}

static int uam_setup(const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "Client Krb v2", 
		   gss_login, gss_logincont, gss_logout, gss_login_ext) < 0)
        return -1;

  return 0;
}

static void uam_cleanup(void)
{
  uam_unregister(UAM_SERVER_LOGIN_EXT, "Client Krb v2");
}

UAM_MODULE_EXPORT struct uam_export uams_gss = {
  UAM_MODULE_SERVER,
  UAM_MODULE_VERSION,
  uam_setup, uam_cleanup
};
#endif
