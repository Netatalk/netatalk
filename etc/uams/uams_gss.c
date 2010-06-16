/*
 * $Id: uams_gss.c,v 1.12 2010-03-30 10:25:49 franklahm Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * Copyright (c) 2003 The Reed Institute
 * Copyright (c) 2004 Bjoern Fernhomberg
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

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

#include <errno.h>
#include <atalk/logger.h>
#include <atalk/afp.h>
#include <atalk/uam.h>
#include <atalk/util.h>

/* Kerberos includes */

#if HAVE_GSSAPI_H
#include <gssapi.h>
#endif

#if HAVE_GSSAPI_GSSAPI_H
#include <gssapi/gssapi.h>
#endif

#if HAVE_GSSAPI_GSSAPI_GENERIC_H
#include <gssapi/gssapi_generic.h>
#endif

#if HAVE_GSSAPI_GSSAPI_KRB5_H
#include <gssapi/gssapi_krb5.h>
#endif

#if HAVE_COM_ERR_H
#include <com_err.h>
#endif

/* We work around something I don't entirely understand... */
/* BF: This is a Heimdal/MIT compatibility fix */
#ifndef HAVE_GSS_C_NT_HOSTBASED_SERVICE
#define GSS_C_NT_HOSTBASED_SERVICE gss_nt_service_name
#endif

#ifdef MIN
#undef MIN
#endif

#define MIN(a, b) ((a > b) ? b : a)

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


static void log_ctx_flags( OM_uint32 flags )
{
#ifdef DEBUG1
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
#endif
}

static void log_principal(gss_name_t server_name)
{
#if 0
    /* FIXME: must call gss_canonicalize_name before gss_export_name */
    OM_uint32 major_status = 0, minor_status = 0;
    gss_buffer_desc exported_name;

    /* Only for debugging purposes, check the gssapi internal representation */
    major_status = gss_export_name(&minor_status, server_name, &exported_name);
    LOG(log_debug, logtype_uams, "log_principal: exported server name is %s", (char*) exported_name.value);
    gss_release_buffer( &minor_status, &exported_name );
#endif
}

/* get the principal from afpd and import it into server_name */
static int get_afpd_principal(void *obj, gss_name_t *server_name)
{
    OM_uint32 major_status = 0, minor_status = 0;
    char *fqdn, *service, *principal, *p;
    size_t fqdnlen=0, servicelen=0;
    size_t principal_length;
    gss_buffer_desc s_princ_buffer;

    /* get all the required information from afpd */
    if (uam_afpserver_option(obj, UAM_OPTION_FQDN, (void*) &fqdn, &fqdnlen) < 0)
        return 1;
    LOG(log_debug, logtype_uams, "get_afpd_principal: fqdn: %s", fqdn);

    if (uam_afpserver_option(obj, UAM_OPTION_KRB5SERVICE, (void *)&service, &servicelen) < 0)
        return 1;
    LOG(log_debug, logtype_uams, "get_afpd_principal: service: %s", service);

    /* we need all the info, log error and return if one's missing */
    if (!service || !servicelen || !fqdn || !fqdnlen) {
        LOG(log_error, logtype_uams,
            "get_afpd_principal: could not retrieve required information from afpd.");
        return 1;
    }

    /* allocate memory to hold the temporary principal string */
    principal_length = servicelen + 1 + fqdnlen + 1;
    if ( NULL == (principal = (char*) malloc( principal_length)) ) {
        LOG(log_error, logtype_uams,
            "get_afpd_principal: out of memory allocating %u bytes",
            principal_length);
        return 1;
    }

    /*
     * Build the principal string.
     * Format: 'service@fqdn'
     */
    strlcpy( principal, service, principal_length);
    strlcat( principal, "@", principal_length);

    /*
     * The fqdn we get from afpd may contain a port.
     * We need to strip the port from fqdn for principal.
     */
    if ((p = strchr(fqdn, ':')))
        *p = '\0';

    strlcat( principal, fqdn, principal_length);
    if (p)
        *p = ':';
    /*
     * Import our principal into the gssapi internal representation
     * stored in server_name.
     */
    s_princ_buffer.value = principal;
    s_princ_buffer.length = strlen( principal ) + 1;

    LOG(log_debug, logtype_uams, "get_afpd_principal: importing principal `%s'", principal);
    major_status = gss_import_name( &minor_status,
                                    &s_princ_buffer,
                                    GSS_C_NT_HOSTBASED_SERVICE,
                                    server_name );

    /*
     * Get rid of malloc'ed memmory.
     * Don't release the s_princ_buffer, we free principal instead.
     */
    free(principal);

    if (major_status != GSS_S_COMPLETE) {
        /* Importing our service principal failed, bail out. */
        log_status( "import_principal", major_status, minor_status );
        return 1;
    }
    return 0;
}


/* get the username */
static int get_client_username(char *username, int ulen, gss_name_t *client_name)
{
    OM_uint32 major_status = 0, minor_status = 0;
    gss_buffer_desc client_name_buffer;
    char *p;
    int namelen, ret=0;

    /*
     * To extract the unix username, use gss_display_name on client_name.
     * We do rely on gss_display_name returning a zero terminated string.
     * The username returned contains the realm and possibly an instance.
     * We only want the username for afpd, so we have to strip those from
     * the username before copying it to afpd's buffer.
     */

    major_status = gss_display_name( &minor_status, *client_name,
                                     &client_name_buffer, (gss_OID *)NULL );
    if (major_status != GSS_S_COMPLETE) {
        log_status( "display_name", major_status, minor_status );
        return 1;
    }

    LOG(log_debug, logtype_uams, "get_client_username: user is `%s'", client_name_buffer.value);

    /* chop off realm */
    p = strchr( client_name_buffer.value, '@' );
    if (p)
        *p = 0;
    /* FIXME: chop off instance? */
    p = strchr( client_name_buffer.value, '/' );
    if (p)
        *p = 0;

    /* check if this username fits into afpd's username buffer */
    namelen = strlen(client_name_buffer.value);
    if ( namelen >= ulen ) {
        /* The username is too long for afpd's buffer, bail out */
        LOG(log_error, logtype_uams,
            "get_client_username: username `%s' too long", client_name_buffer.value);
        ret = 1;
    }
    else {
        /* copy stripped username to afpd's buffer */
        strlcpy(username, client_name_buffer.value, ulen);
    }

    /* we're done with client_name_buffer, release it */
    gss_release_buffer(&minor_status, &client_name_buffer );

    return ret;
}

/* wrap afpd's sessionkey */
static int wrap_sessionkey(gss_ctx_id_t context, struct session_info *sinfo)
{
    OM_uint32 status = 0;
    int ret=0;
    gss_buffer_desc sesskey_buff, wrap_buff;

    /*
     * gss_wrap afpd's session_key.
     * This is needed fo OS X 10.3 clients. They request this information
     * with type 8 (kGetKerberosSessionKey) on FPGetSession.
     * See AFP 3.1 specs, page 77.
     */

    sesskey_buff.value = sinfo->sessionkey;
    sesskey_buff.length = sinfo->sessionkey_len;

    /* gss_wrap the session key with the default machanism.
       Require both confidentiality and integrity services */
    gss_wrap (&status, context, 1, GSS_C_QOP_DEFAULT, &sesskey_buff, NULL, &wrap_buff);

    if ( status != GSS_S_COMPLETE) {
        LOG(log_error, logtype_uams, "wrap_sessionkey: failed to gss_wrap sessionkey");
        log_status( "GSS wrap", 0, status );
        return 1;
    }

    /* store the wrapped session key in afpd's session_info struct */
    if ( NULL == (sinfo->cryptedkey = malloc ( wrap_buff.length )) ) {
        LOG(log_error, logtype_uams,
            "wrap_sessionkey: out of memory tyring to allocate %u bytes",
            wrap_buff.length);
        ret = 1;
    } else {
        /* cryptedkey is binary data */
        memcpy (sinfo->cryptedkey, wrap_buff.value, wrap_buff.length);
        sinfo->cryptedkey_len = wrap_buff.length;
    }

    /* we're done with buffer, release */
    gss_release_buffer( &status, &wrap_buff );

    return ret;
}

/*-------------*/
static int acquire_credentials (gss_name_t *server_name, gss_cred_id_t *server_creds)
{
    OM_uint32 major_status = 0, minor_status = 0;
    char *envp;

    if ((envp = getenv("KRB5_KTNAME")))
        LOG(log_debug, logtype_uams,
            "acquire credentials: acquiring credentials (uid = %d, keytab = %s)",
            (int)geteuid(), envp);
    else
        LOG(log_debug, logtype_uams,
            "acquire credentials: acquiring credentials (uid = %d) - $KRB5_KTNAME not found in env",
            (int)geteuid());
        
    /*
     * Acquire credentials usable for accepting context negotiations.
     * Credentials are for server_name, have an indefinite lifetime,
     * have no specific mechanisms, are to be used for accepting context
     * negotiations and are to be placed in server_creds.
     * We don't care about the mechanisms or about the time for which they are valid.
     */
    major_status = gss_acquire_cred( &minor_status, *server_name,
                                     GSS_C_INDEFINITE, GSS_C_NO_OID_SET, GSS_C_ACCEPT,
                                     server_creds, NULL, NULL );

    if (major_status != GSS_S_COMPLETE) {
        log_status( "acquire_cred", major_status, minor_status );
        return 1;
    }

    return 0;
}

/*-------------*/
static int accept_sec_context (gss_ctx_id_t *context, gss_cred_id_t server_creds,
                               gss_buffer_desc *ticket_buffer, gss_name_t *client_name,
                               gss_buffer_desc *authenticator_buff)
{
    OM_uint32 major_status = 0, minor_status = 0, ret_flags;

    /* Initialize autheticator buffer. */
    authenticator_buff->length = 0;
    authenticator_buff->value = NULL;

    LOG(log_debug, logtype_uams, "accept_context: accepting context (ticketlen: %u)",
        ticket_buffer->length);

    /*
     * Try to accept the secondary context using the tocken in ticket_buffer.
     * We don't care about the mechanisms used, nor for the time.
     * We don't act as a proxy either.
     */
    major_status = gss_accept_sec_context( &minor_status, context,
                                           server_creds, ticket_buffer, GSS_C_NO_CHANNEL_BINDINGS,
                                           client_name, NULL, authenticator_buff,
                                           &ret_flags, NULL, NULL );

    if (major_status != GSS_S_COMPLETE) {
        log_status( "accept_sec_context", major_status, minor_status );
        return 1;
    }
    log_ctx_flags( ret_flags );
    return 0;
}


/* return 0 on success */
static int do_gss_auth(void *obj, char *ibuf, int ticket_len,
                       char *rbuf, int *rbuflen, char *username, int ulen,
                       struct session_info *sinfo )
{
    OM_uint32 status = 0;
    gss_name_t server_name, client_name;
    gss_cred_id_t server_creds;
    gss_ctx_id_t context_handle = GSS_C_NO_CONTEXT;
    gss_buffer_desc ticket_buffer, authenticator_buff;
    int ret = 0;

    /* import our principal name from afpd */
    if (get_afpd_principal(obj, &server_name) != 0) {
        return 1;
    }
    log_principal(server_name);

    /* Now we have to acquire our credentials */
    if ((ret = acquire_credentials (&server_name, &server_creds)))
        goto cleanup_vars;

    /*
     * Try to accept the secondary context, using the ticket/token the
     * client sent us. Ticket is stored at current ibuf position.
     * Don't try to release ticket_buffer later, it points into ibuf!
     */
    ticket_buffer.length = ticket_len;
    ticket_buffer.value = ibuf;

    ret = accept_sec_context (&context_handle, server_creds, &ticket_buffer,
                              &client_name, &authenticator_buff);

    if (!ret) {
        /* We succesfully acquired the secondary context, now get the
           username for afpd and gss_wrap the sessionkey */
        if ( 0 == (ret = get_client_username(username, ulen, &client_name)) ) {
            ret = wrap_sessionkey(context_handle, sinfo);
        }

        if (!ret) {
            /* FIXME: Is copying the authenticator really necessary?
               Where is this documented? */
            u_int16_t auth_len = htons( authenticator_buff.length );

            /* copy the authenticator length into the reply buffer */
            memcpy( rbuf, &auth_len, sizeof(auth_len) );
            *rbuflen += sizeof(auth_len);
            rbuf += sizeof(auth_len);

            /* copy the authenticator value into the reply buffer */
            memcpy( rbuf, authenticator_buff.value, authenticator_buff.length );
            *rbuflen += authenticator_buff.length;
        }

        /* Clean up after ourselves */
        gss_release_name( &status, &client_name );
        if ( authenticator_buff.value)
            gss_release_buffer( &status, &authenticator_buff );

        gss_delete_sec_context( &status, &context_handle, NULL );
    }
    gss_release_cred( &status, &server_creds );

cleanup_vars:
    gss_release_name( &status, &server_name );

    return ret;
}

/* -------------------------- */
static int gss_login(void *obj, struct passwd **uam_pwd,
                     char *ibuf, size_t ibuflen,
                     char *rbuf, size_t *rbuflen)
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
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
{
    struct passwd *pwd = NULL;
    u_int16_t login_id;
    char *username;
    u_int16_t ticket_len;
    char *p;
    int rblen;
    size_t userlen;
    struct session_info *sinfo;

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

    if (ibuflen < 1 +sizeof(login_id)) {
        LOG(log_info, logtype_uams, "uams_gss.c :LoginCont: received incomplete packet");
        return AFPERR_PARAM;
    }
    ibuf++, ibuflen--; /* ?? */

    /* 2 byte ID from LoginExt -- always '00 01' in this implementation */
    memcpy( &login_id, ibuf, sizeof(login_id) );
    ibuf += sizeof(login_id), ibuflen -= sizeof(login_id);
    login_id = ntohs( login_id );

    /* get the username buffer from apfd */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username, &userlen) < 0)
        return AFPERR_MISC;

    /* get the session_info structure from afpd. We need the session key */
    if (uam_afpserver_option(obj, UAM_OPTION_SESSIONINFO, (void *)&sinfo, NULL) < 0)
        return AFPERR_MISC;

    if (sinfo->sessionkey == NULL || sinfo->sessionkey_len == 0) {
        /* Should never happen. Most likely way too old afpd version */
        LOG(log_info, logtype_uams, "internal error: afpd's sessionkey not set");
        return AFPERR_MISC;
    }

    /* We skip past the 'username' parameter because all that matters is the ticket */
    p = ibuf;
    while( *ibuf && ibuflen ) { ibuf++, ibuflen--; }
    if (ibuflen < 4) {
        LOG(log_info, logtype_uams, "uams_gss.c :LoginCont: user is %s, no ticket", p);
        return AFPERR_PARAM;
    }

    ibuf++, ibuflen--; /* null termination */

    if ((ibuf - p + 1) % 2) ibuf++, ibuflen--; /* deal with potential padding */

    LOG(log_debug, logtype_uams, "uams_gss.c :LoginCont: client thinks user is %s", p);

    /* get the length of the ticket the client sends us */
    memcpy(&ticket_len, ibuf, sizeof(ticket_len));
    ibuf += sizeof(ticket_len); ibuflen -= sizeof(ticket_len);
    ticket_len = ntohs( ticket_len );

    /* a little bounds checking */
    if (ticket_len > ibuflen) {
        LOG(log_info, logtype_uams,
            "uams_gss.c :LoginCont: invalid ticket length (%u > %u)", ticket_len, ibuflen);
        return AFPERR_PARAM;
    }

    /* now try to authenticate */
    if (!do_gss_auth(obj, ibuf, ticket_len, rbuf, &rblen, username, userlen, sinfo)) {
        /* We use the username we got back from the gssapi client_name.
           Should we compare this to the username the client sent in the clear?
           We know the character encoding of the cleartext username (UTF8), what
           encoding is the gssapi name in? */
        if((pwd = uam_getname( obj, username, userlen )) == NULL) {
            LOG(log_info, logtype_uams, "uam_getname() failed for %s", username);
            return AFPERR_NOTAUTH;
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
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
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

int uam_setup(const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "Client Krb v2",
                     gss_login, gss_logincont, gss_logout, gss_login_ext) < 0)
        if (uam_register(UAM_SERVER_LOGIN, path, "Client Krb v2",
                         gss_login, gss_logincont, gss_logout) < 0)
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
