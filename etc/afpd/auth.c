/*
 * $Id: auth.c,v 1.60 2009-02-26 14:05:05 franklahm Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <netatalk/endian.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/util.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <atalk/logger.h>
#include <atalk/server_ipc.h>
#include <atalk/uuid.h>

#ifdef TRU64
#include <netdb.h>
#include <arpa/inet.h>
#include <sia.h>
#include <siad.h>

extern void afp_get_cmdline( int *ac, char ***av );
#endif /* TRU64 */

#include "globals.h"
#include "auth.h"
#include "uam_auth.h"
#include "switch.h"
#include "status.h"
#include "fork.h"
#ifdef HAVE_NFSv4_ACLS
#include "acls.h"
#endif
#ifdef HAVE_EXT_ATTRS
#include "extattrs.h"
#endif

int	afp_version = 11;
static int afp_version_index;

uid_t	uuid;

#if defined( sun ) && !defined( __svr4__ ) || defined( ultrix )

int	*groups;
#define GROUPS_SIZE sizeof(int)

#else /* sun __svr4__ ultrix */

gid_t	*groups;
#define GROUPS_SIZE sizeof(gid_t)
#endif /* sun ultrix */

int	ngroups;

/*
 * These numbers are scattered throughout the code.
 */
static struct afp_versions	afp_versions[] = {
            { "AFPVersion 1.1",	11 },
            { "AFPVersion 2.0",	20 },
            { "AFPVersion 2.1",	21 },
            { "AFP2.2",	22 },
#ifdef AFP3x
            { "AFPX03", 30 },
            { "AFP3.1", 31 },
            { "AFP3.2", 32 }
#endif            
        };

static struct uam_mod uam_modules = {NULL, NULL, &uam_modules, &uam_modules};
static struct uam_obj uam_login = {"", "", 0, {{NULL, NULL, NULL, NULL }}, &uam_login,
                                      &uam_login};
static struct uam_obj uam_changepw = {"", "", 0, {{NULL, NULL, NULL, NULL}}, &uam_changepw,
                                         &uam_changepw};

static struct uam_obj *afp_uam = NULL;


void status_versions( data )
char	*data;
{
    char                *start = data;
    u_int16_t           status;
    int			len, num, i;

    memcpy(&status, start + AFPSTATUS_VERSOFF, sizeof(status));
    num = sizeof( afp_versions ) / sizeof( afp_versions[ 0 ] );
    data += ntohs( status );
    *data++ = num;
    for ( i = 0; i < num; i++ ) {
        len = strlen( afp_versions[ i ].av_name );
        *data++ = len;
        memcpy( data, afp_versions[ i ].av_name , len );
        data += len;
    }
    status = htons( data - start );
    memcpy(start + AFPSTATUS_UAMSOFF, &status, sizeof(status));
}

void status_uams(char *data, const char *authlist)
{
    char                *start = data;
    u_int16_t           status;
    struct uam_obj      *uams;
    int			len, num = 0;

    memcpy(&status, start + AFPSTATUS_UAMSOFF, sizeof(status));
    uams = &uam_login;
    while ((uams = uams->uam_prev) != &uam_login) {
        if (strstr(authlist, uams->uam_path))
            num++;
    }

    data += ntohs( status );
    *data++ = num;
    while ((uams = uams->uam_prev) != &uam_login) {
        if (strstr(authlist, uams->uam_path)) {
            LOG(log_info, logtype_afpd, "uam: \"%s\" available", uams->uam_name);
            len = strlen( uams->uam_name);
            *data++ = len;
            memcpy( data, uams->uam_name, len );
            data += len;
        }
    }

    /* icon offset */
    status = htons(data - start);
    memcpy(start + AFPSTATUS_ICONOFF, &status, sizeof(status));
}

/* handle errors by closing the connection. this is only needed
 * by the afp_* functions. */
static int send_reply(const AFPObj *obj, const int err)
{
    if ((err == AFP_OK) || (err == AFPERR_AUTHCONT))
        return err;

    obj->reply(obj->handle, err);
    obj->exit(0);

    return AFP_OK;
}

static int afp_errpwdexpired(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj _U_;
char	*ibuf _U_, *rbuf _U_;
int	ibuflen _U_, *rbuflen;
{
    *rbuflen = 0;
    return AFPERR_PWDEXPR;
}

static int afp_null_nolog(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj _U_;
char	*ibuf _U_, *rbuf _U_;
int	ibuflen _U_, *rbuflen;
{
    *rbuflen = 0;
    return( AFPERR_NOOP );
}

static int set_auth_switch(int expired)
{
    int i;

    if (expired) {
	/* 
	 * BF: expired password handling
	 * to allow the user to change his/her password we have to allow login
	 * but every following call except for FPChangePassword will be thrown
	 * away with an AFPERR_PWDEXPR error. (thanks to Leland Wallace from Apple
	 * for clarifying this)
         */

	for (i=0; i<=0xff; i++) {
	    uam_afpserver_action(i, UAM_AFPSERVER_PREAUTH, afp_errpwdexpired, NULL); 
	}
        uam_afpserver_action(AFP_LOGOUT, UAM_AFPSERVER_PREAUTH, afp_logout, NULL); 
	uam_afpserver_action(AFP_CHANGEPW, UAM_AFPSERVER_PREAUTH, afp_changepw, NULL);
    }
    else {
        afp_switch = postauth_switch;
        switch (afp_version) {
        case 32:
	    uam_afpserver_action(AFP_SPOTLIGHT_PRIVATE, UAM_AFPSERVER_POSTAUTH, afp_null_nolog, NULL);
	    uam_afpserver_action(AFP_SYNCDIR, UAM_AFPSERVER_POSTAUTH, afp_syncdir, NULL);
	    uam_afpserver_action(AFP_SYNCFORK, UAM_AFPSERVER_POSTAUTH, afp_syncfork, NULL);
#ifdef HAVE_NFSv4_ACLS
	    uam_afpserver_action(AFP_GETACL, UAM_AFPSERVER_POSTAUTH, afp_getacl, NULL);
	    uam_afpserver_action(AFP_SETACL, UAM_AFPSERVER_POSTAUTH, afp_setacl, NULL);
	    uam_afpserver_action(AFP_ACCESS, UAM_AFPSERVER_POSTAUTH, afp_access, NULL);
#endif
#ifdef HAVE_EXT_ATTRS
	    uam_afpserver_action(AFP_GETEXTATTR, UAM_AFPSERVER_POSTAUTH, afp_getextattr, NULL);
	    uam_afpserver_action(AFP_SETEXTATTR, UAM_AFPSERVER_POSTAUTH, afp_setextattr, NULL);
	    uam_afpserver_action(AFP_REMOVEATTR, UAM_AFPSERVER_POSTAUTH, afp_remextattr, NULL);
	    uam_afpserver_action(AFP_LISTEXTATTR, UAM_AFPSERVER_POSTAUTH, afp_listextattr, NULL);
#endif
        case 31:
	    uam_afpserver_action(AFP_ENUMERATE_EXT2, UAM_AFPSERVER_POSTAUTH, afp_enumerate_ext2, NULL);
        case 30:
	    uam_afpserver_action(AFP_ENUMERATE_EXT, UAM_AFPSERVER_POSTAUTH, afp_enumerate_ext, NULL); 
	    uam_afpserver_action(AFP_BYTELOCK_EXT,  UAM_AFPSERVER_POSTAUTH, afp_bytelock_ext, NULL); 
            /* catsearch_ext uses the same packet as catsearch FIXME double check this, it wasn't true for enue
               enumerate_ext */
	    uam_afpserver_action(AFP_CATSEARCH_EXT, UAM_AFPSERVER_POSTAUTH, afp_catsearch_ext, NULL); 
	    uam_afpserver_action(AFP_GETSESSTOKEN,  UAM_AFPSERVER_POSTAUTH, afp_getsession, NULL); 
	    uam_afpserver_action(AFP_READ_EXT,      UAM_AFPSERVER_POSTAUTH, afp_read_ext, NULL); 
	    uam_afpserver_action(AFP_WRITE_EXT,     UAM_AFPSERVER_POSTAUTH, afp_write_ext, NULL); 
	    uam_afpserver_action(AFP_DISCTOLDSESS,  UAM_AFPSERVER_POSTAUTH, afp_disconnect, NULL); 

        case 22:
            /*
             * If first connection to a server is done in classic AFP2.2 version is used
             * but OSX uses AFP3.x FPzzz command !
            */
	    uam_afpserver_action(AFP_ZZZ,  UAM_AFPSERVER_POSTAUTH, afp_zzz, NULL); 
	    break;
        }
    }

    return AFP_OK;
}

static int login(AFPObj *obj, struct passwd *pwd, void (*logout)(void), int expired)
{
#ifdef ADMIN_GRP
    int admin = 0;
#endif /* ADMIN_GRP */

    /* UAM had syslog control; afpd needs to reassert itself */
    set_processname("afpd");
    syslog_setup(log_debug, logtype_default, logoption_ndelay|logoption_pid, logfacility_daemon);

    if ( pwd->pw_uid == 0 ) {	/* don't allow root login */
        LOG(log_error, logtype_afpd, "login: root login denied!" );
        return AFPERR_NOTAUTH;
    }

    LOG(log_info, logtype_afpd, "login %s (uid %d, gid %d) %s", pwd->pw_name,
        pwd->pw_uid, pwd->pw_gid , afp_versions[afp_version_index].av_name);

#ifndef NO_DDP
    if (obj->proto == AFPPROTO_ASP) {
        ASP asp = obj->handle;
        int addr_net = ntohs( asp->asp_sat.sat_addr.s_net );
        int addr_node  = asp->asp_sat.sat_addr.s_node;

        if (obj->options.authprintdir) {
            if(addr_net && addr_node) { /* Do we have a valid Appletalk address? */
                char nodename[256];
                FILE *fp;
		int mypid = getpid();
                struct stat stat_buf;

                sprintf(nodename, "%s/net%d.%dnode%d", obj->options.authprintdir,
                        addr_net / 256, addr_net % 256, addr_node);
                LOG(log_info, logtype_afpd, "registering %s (uid %d) on %u.%u as %s",
                    pwd->pw_name, pwd->pw_uid, addr_net, addr_node, nodename);

                if (stat(nodename, &stat_buf) == 0) { /* file exists */
                    if (S_ISREG(stat_buf.st_mode)) { /* normal file */
                        unlink(nodename);
                        fp = fopen(nodename, "w");
                        fprintf(fp, "%s:%d\n", pwd->pw_name, mypid);
                        fclose(fp);
                        chown( nodename, pwd->pw_uid, -1 );
                    } else { /* somebody is messing with us */
                        LOG(log_error, logtype_afpd, "print authfile %s is not a normal file, it will not be modified", nodename );
                    }
                } else { /* file 'nodename' does not exist */
                    fp = fopen(nodename, "w");
                    fprintf(fp, "%s:%d\n", pwd->pw_name, mypid);
                    fclose(fp);
                    chown( nodename, pwd->pw_uid, -1 );
                }
            } /* if (addr_net && addr_node ) */
        } /* if (options->authprintdir) */
    } /* if (obj->proto == AFPPROTO_ASP) */
#endif

    if (initgroups( pwd->pw_name, pwd->pw_gid ) < 0) {
#ifdef RUN_AS_USER
        LOG(log_info, logtype_afpd, "running with uid %d", geteuid());
#else /* RUN_AS_USER */
        LOG(log_error, logtype_afpd, "login: %s", strerror(errno));
        return AFPERR_BADUAM;
#endif /* RUN_AS_USER */

    }

    /* Basically if the user is in the admin group, we stay root */

    if (( ngroups = getgroups( 0, NULL )) < 0 ) {
        LOG(log_error, logtype_afpd, "login: %s getgroups: %s", pwd->pw_name, strerror(errno) );
        return AFPERR_BADUAM;
    }
    
    if ( NULL == (groups = calloc(ngroups, GROUPS_SIZE)) ) {
        LOG(log_error, logtype_afpd, "login: %s calloc: %d", ngroups);
        return AFPERR_BADUAM;
    }

    if (( ngroups = getgroups( ngroups, groups )) < 0 ) {
        LOG(log_error, logtype_afpd, "login: %s getgroups: %s", pwd->pw_name, strerror(errno) );
        return AFPERR_BADUAM;
    }

#ifdef ADMIN_GRP
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "obj->options.admingid == %d", obj->options.admingid);
#endif /* DEBUG */
    if (obj->options.admingid != 0) {
        int i;
        for (i = 0; i < ngroups; i++) {
            if (groups[i] == obj->options.admingid) admin = 1;
        }
    }
    if (admin) {
        ad_setfuid(0);
        LOG(log_info, logtype_afpd, "admin login -- %s", pwd->pw_name );
    }
    if (!admin)
#endif /* DEBUG */
#ifdef TRU64
    {
        struct DSI *dsi = obj->handle;
        struct hostent *hp;
        char *clientname;
        int argc;
        char **argv;
        char hostname[256];

        afp_get_cmdline( &argc, &argv );

        hp = gethostbyaddr( (char *) &dsi->client.sin_addr,
                            sizeof( struct in_addr ),
                            dsi->client.sin_family );

        if( hp )
            clientname = hp->h_name;
        else
            clientname = inet_ntoa( dsi->client.sin_addr );

        sprintf( hostname, "%s@%s", pwd->pw_name, clientname );

        if( sia_become_user( NULL, argc, argv, hostname, pwd->pw_name,
                             NULL, FALSE, NULL, NULL,
                             SIA_BEU_REALLOGIN ) != SIASUCCESS )
            return AFPERR_BADUAM;

        LOG(log_info, logtype_afpd, "session from %s (%s)", hostname,
            inet_ntoa( dsi->client.sin_addr ) );

        if (setegid( pwd->pw_gid ) < 0 || seteuid( pwd->pw_uid ) < 0) {
            LOG(log_error, logtype_afpd, "login: %s %s", pwd->pw_name, strerror(errno) );
            return AFPERR_BADUAM;
        }
    }
#else /* TRU64 */
    if (setegid( pwd->pw_gid ) < 0 || seteuid( pwd->pw_uid ) < 0) {
        LOG(log_error, logtype_afpd, "login: %s %s", pwd->pw_name, strerror(errno) );
        return AFPERR_BADUAM;
    }
#endif /* TRU64 */

    /* There's probably a better way to do this, but for now, we just
    play root */

#ifdef ADMIN_GRP
    if (admin) uuid = 0;
    else
#endif /* ADMIN_GRP */
        uuid = pwd->pw_uid;

    set_auth_switch(expired);

    obj->logout = logout;

#ifdef FORCE_UIDGID
    obj->force_uid = 1;
    save_uidgid ( &obj->uidgid );
#endif 
    	
    return( AFP_OK );
}

/* ---------------------- */
int afp_zzz (obj, ibuf, ibuflen, rbuf, rbuflen ) /* Function 122 */
AFPObj       *obj;
char         *ibuf  _U_, *rbuf;
unsigned int ibuflen  _U_, *rbuflen;
{
    u_int32_t	retdata;

    *rbuflen = 0;

    retdata = obj->options.sleep /120;
    if (!retdata) {
    	retdata = 1;
    }
    *rbuflen = sizeof(retdata);
    retdata = htonl(retdata);
    memcpy(rbuf, &retdata, sizeof(retdata));
    if (obj->sleep)
        obj->sleep();
    rbuf += sizeof(retdata);
    return AFP_OK;
}

/* ---------------------- */
static int create_session_token(AFPObj *obj)
{
    pid_t pid;

    /* use 8 bytes for token as OSX, don't know if it helps */
    if ( sizeof(pid_t) > SESSIONTOKEN_LEN) {
       LOG(log_error, logtype_afpd, "sizeof(pid_t) > %u", SESSIONTOKEN_LEN );
       return AFPERR_MISC;
    }

    if ( NULL == (obj->sinfo.sessiontoken = malloc(SESSIONTOKEN_LEN)) )
       return AFPERR_MISC;

    memset(obj->sinfo.sessiontoken, 0, SESSIONTOKEN_LEN);
    obj->sinfo.sessiontoken_len = SESSIONTOKEN_LEN;
    pid = getpid();
    memcpy(obj->sinfo.sessiontoken, &pid, sizeof(pid_t));

    return 0;
}

static int create_session_key(AFPObj *obj)
{
    /* create session key */
    if (obj->sinfo.sessionkey == NULL) {
        if (NULL == (obj->sinfo.sessionkey = malloc(SESSIONKEY_LEN)) )
            return AFPERR_MISC;   
        uam_random_string(obj, obj->sinfo.sessionkey, SESSIONKEY_LEN);
        obj->sinfo.sessionkey_len = SESSIONKEY_LEN;
    }
    return AFP_OK;
}

   
/* ---------------------- */
int afp_getsession(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj       *obj;
char	     *ibuf, *rbuf;
unsigned int ibuflen, *rbuflen;
{
    u_int16_t           type;
    u_int32_t           idlen = 0;
    u_int32_t		boottime;
    u_int32_t           tklen, tp;
    char                *token; 
    char                *p;

    *rbuflen = 0;
    tklen = 0;

    ibuf += 2;
    ibuflen -= 2;

    memcpy(&type, ibuf, sizeof(type));
    type = ntohs(type);
    ibuf += sizeof(type);
    ibuflen -= sizeof(type);

    if ( obj->sinfo.sessiontoken == NULL ) {
        if ( create_session_token( obj ) )
            return AFPERR_MISC;
    }

    /*
     * 
    */
    switch (type) {
    case 0: /* old version ?*/
            tklen = obj->sinfo.sessiontoken_len;
            token = obj->sinfo.sessiontoken;
        break;
    case 1: /* disconnect */
    case 2: /* reconnect update id */
        if (ibuflen >= sizeof(idlen)) {
            memcpy(&idlen, ibuf, sizeof(idlen));
            idlen = ntohl(idlen);
            ibuf += sizeof(idlen);
            ibuflen -= sizeof(idlen);
            if (ibuflen < idlen) {
                return AFPERR_PARAM;
            }
            /* memcpy (id, ibuf, idlen) */
            tklen = obj->sinfo.sessiontoken_len;
            token = obj->sinfo.sessiontoken;
        }
        break;
    case 3: /* Jaguar */
    case 4:
	if (ibuflen >= 8 ) {
	    p = ibuf;
	    memcpy( &idlen, ibuf, sizeof(idlen));
	    idlen = ntohl(idlen);
	    ibuf += sizeof(idlen);
	    ibuflen -= sizeof(idlen);
	    ibuf += sizeof(boottime);
	    ibuflen -= sizeof(boottime);
	    if (ibuflen < idlen || idlen > (90-10)) {
		return AFPERR_PARAM;
	    }
	    server_ipc_write(IPC_GETSESSION, idlen+8, p );
	    tklen = obj->sinfo.sessiontoken_len;
	    token = obj->sinfo.sessiontoken;
	}
	break;
     case 8: /* Panther Kerberos Token */
            tklen = obj->sinfo.cryptedkey_len;
            token = obj->sinfo.cryptedkey;
        break;
     default:
            return AFPERR_NOOP;
        break;

    }

    if (tklen == 0)
        return AFPERR_MISC;

    tp = htonl(tklen);
    memcpy(rbuf, &tp, sizeof(tklen));
    rbuf += sizeof(tklen);
    *rbuflen += sizeof(tklen);

    memcpy(rbuf, token, tklen);
    *rbuflen += tklen;

    return AFP_OK;
}

/* ---------------------- */
int afp_disconnect(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj  _U_;
char	*ibuf, *rbuf  _U_;
int	ibuflen  _U_, *rbuflen;
{
    u_int16_t           type;

    u_int32_t           tklen;
    pid_t               token;
    int                 i;

    *rbuflen = 0;
    ibuf += 2;

#if 0
    /* check for guest user */
    if ( 0 == (strcasecmp(obj->username, obj->options.guest)) ) {
        return AFPERR_MISC;
    }
#endif

    memcpy(&type, ibuf, sizeof(type));
    type = ntohs(type);
    ibuf += sizeof(type);

    memcpy(&tklen, ibuf, sizeof(tklen));
    tklen = ntohl(tklen);
    ibuf += sizeof(tklen);

    if ( sizeof(pid_t) > SESSIONTOKEN_LEN) {
         LOG(log_error, logtype_afpd, "sizeof(pid_t) > %u", SESSIONTOKEN_LEN );
         return AFPERR_MISC;
    }
    if (tklen != SESSIONTOKEN_LEN) {
        return AFPERR_MISC;
    }   
    tklen = sizeof(pid_t);
    memcpy(&token, ibuf, tklen);

    /* our stuff is pid + zero pad */
    ibuf += tklen;
    for (i = tklen; i < SESSIONTOKEN_LEN; i++, ibuf++) {
         if (*ibuf != 0) {
             return AFPERR_MISC;
         }
    }
    
    /* killed old session, not easy */
    server_ipc_write(IPC_KILLTOKEN, tklen, &token);
    sleep(1);
    
    return AFPERR_SESSCLOS;   /* was AFP_OK */
}

/* ---------------------- */
static int get_version(obj, ibuf, ibuflen, len)
AFPObj  *obj;
char    *ibuf;
int  	ibuflen;
int  	len;
{
    int num,i;

    if (!len || len > ibuflen)
        return AFPERR_BADVERS;

    num = sizeof( afp_versions ) / sizeof( afp_versions[ 0 ]);
    for ( i = 0; i < num; i++ ) {
        if ( strncmp( ibuf, afp_versions[ i ].av_name , len ) == 0 ) {
            afp_version = afp_versions[ i ].av_number;
            afp_version_index = i;
            break;
        }
    }
    if ( i == num ) 				/* An inappropo version */
        return AFPERR_BADVERS ;

    if (afp_version >= 30 && obj->proto != AFPPROTO_DSI)
        return AFPERR_BADVERS ;

    /* FIXME Hack */
    if (afp_version >= 30 && sizeof(off_t) != 8) {
        LOG(log_error, logtype_afpd, "get_version: no LARGE_FILE support recompile!" );
        return AFPERR_BADVERS ;
    }

    return 0;
}

/* ---------------------- */
int afp_login(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct passwd *pwd = NULL;
    int		len, i;

    *rbuflen = 0;

    if ( nologin & 1)
        return send_reply(obj, AFPERR_SHUTDOWN );

    if (ibuflen <= 1)
        return send_reply(obj, AFPERR_BADVERS );

    ibuf++;
    len = (unsigned char) *ibuf++;
    ibuflen -= 2;

    i = get_version(obj, ibuf, ibuflen, len);
    if (i) 
        return send_reply(obj, i );

    ibuf += len;
    ibuflen -= len;

    if (ibuflen <= 1)
        return send_reply(obj, AFPERR_BADUAM);

    len = (unsigned char) *ibuf++;
    ibuflen--;

    if (!len || len > ibuflen)
        return send_reply(obj, AFPERR_BADUAM);

    if (NULL == (afp_uam = auth_uamfind(UAM_SERVER_LOGIN, ibuf, len)) )
        return send_reply(obj, AFPERR_BADUAM);
    ibuf += len;
    ibuflen -= len;

    if (AFP_OK != (i = create_session_key(obj)) )
        return send_reply(obj, i);

    i = afp_uam->u.uam_login.login(obj, &pwd, ibuf, ibuflen, rbuf, rbuflen);

    if (!pwd || ( i != AFP_OK && i != AFPERR_PWDEXPR))
        return send_reply(obj, i);

    return send_reply(obj, login(obj, pwd, afp_uam->u.uam_login.logout, ((i==AFPERR_PWDEXPR)?1:0)));
}

/* ---------------------- */
int afp_login_ext(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj;
char	*ibuf, *rbuf;
unsigned int	ibuflen, *rbuflen;
{
    struct passwd *pwd = NULL;
    unsigned int  len;
    int		i;
    char        type;
    u_int16_t   len16;
    char        *username;
    
    *rbuflen = 0;

    if ( nologin & 1)
        return send_reply(obj, AFPERR_SHUTDOWN );

    if (ibuflen <= 4)
        return send_reply(obj, AFPERR_BADVERS );

    ibuf++; 
    ibuf++;     /* pad  */
    ibuf +=2;   /* flag */

    len = (unsigned char) *ibuf;
    ibuf++;
    ibuflen -= 5;
    
    i = get_version(obj, ibuf, ibuflen, len);
    if (i) 
        return send_reply(obj, i );

    ibuf    += len;
    ibuflen -= len;

    if (ibuflen <= 1)
        return send_reply(obj, AFPERR_BADUAM);

    len = (unsigned char) *ibuf;
    ibuf++;
    ibuflen--;

    if (!len || len > ibuflen)
        return send_reply(obj, AFPERR_BADUAM);

    if ((afp_uam = auth_uamfind(UAM_SERVER_LOGIN, ibuf, len)) == NULL)
        return send_reply(obj, AFPERR_BADUAM);
    ibuf    += len;
    ibuflen -= len;

    if (!afp_uam->u.uam_login.login_ext) {
        LOG(log_error, logtype_afpd, "login_ext: uam %s not AFP 3 ready!", afp_uam->uam_name );
        return send_reply(obj, AFPERR_BADUAM);
    }
    /* user name */
    if (len <= 1 +sizeof(len16)) 
        return send_reply(obj, AFPERR_PARAM);
    type = *ibuf;
    username = ibuf;
    ibuf++;
    ibuflen--;
    if (type != 3) 
        return send_reply(obj, AFPERR_PARAM);

    memcpy(&len16, ibuf, sizeof(len16));
    ibuf += sizeof(len16);
    ibuflen -= sizeof(len16);
    len = ntohs(len16);
    if (len > ibuflen) 
        return send_reply(obj, AFPERR_PARAM);
    ibuf += len;
    ibuflen -= len;    

    /* directory service name */
    if (!ibuflen) 
        return send_reply(obj, AFPERR_PARAM);
    type = *ibuf;
    ibuf++;
    ibuflen--;
    
    switch(type) {
    case 1:
    case 2:
        if (!ibuflen)
            return send_reply(obj, AFPERR_PARAM);
        len = (unsigned char) *ibuf;
        ibuf++;
        ibuflen--;
        break;
    case 3:
        /* With "No User Authen" it is equal */
        if (ibuflen < sizeof(len16)) 
            return send_reply(obj, AFPERR_PARAM);
        memcpy(&len16, ibuf, sizeof(len16));
        ibuf += sizeof(len16);
        ibuflen -= sizeof(len16);
        len = ntohs(len16);
        break;
    default:
        return send_reply(obj, AFPERR_PARAM);
    }    
#if 0
    if (len != 0) {
        LOG(log_error, logtype_afpd, "login_ext: directory service path not null!" );
        return send_reply(obj, AFPERR_PARAM);
    }
#endif
    ibuf += len;
    ibuflen -= len;
    
    /* Pad */
    if (ibuflen && ((unsigned long) ibuf & 1)) { /* pad character */
        ibuf++;
        ibuflen--;
    }

    if (AFP_OK != (i = create_session_key(obj)) ) {
        return send_reply(obj, i);
    }

    /* FIXME user name are in UTF8 */    
    i = afp_uam->u.uam_login.login_ext(obj, username, &pwd, ibuf, ibuflen, rbuf, rbuflen);

    if (!pwd || ( i != AFP_OK && i != AFPERR_PWDEXPR))
        return send_reply(obj, i);

    return send_reply(obj, login(obj, pwd, afp_uam->u.uam_login.logout, ((i==AFPERR_PWDEXPR)?1:0)));
}

/* ---------------------- */
int afp_logincont(obj, ibuf, ibuflen, rbuf, rbuflen)
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct passwd *pwd = NULL;
    int err;

    if ( afp_uam == NULL || afp_uam->u.uam_login.logincont == NULL ) {
        *rbuflen = 0;
        return send_reply(obj, AFPERR_NOTAUTH );
    }

    ibuf += 2; ibuflen -= 2;
    err = afp_uam->u.uam_login.logincont(obj, &pwd, ibuf, ibuflen,
                                         rbuf, rbuflen);
    if (!pwd || ( err != AFP_OK && err != AFPERR_PWDEXPR))
        return send_reply(obj, err);

    return send_reply(obj, login(obj, pwd, afp_uam->u.uam_login.logout, ((err==AFPERR_PWDEXPR)?1:0)));
}


int afp_logout(obj, ibuf, ibuflen, rbuf, rbuflen)
AFPObj     *obj;
char       *ibuf _U_, *rbuf  _U_;
int        ibuflen  _U_, *rbuflen  _U_;
{
    LOG(log_info, logtype_afpd, "logout %s", obj->username);
    close_all_vol();
    obj->exit(0);
    return AFP_OK;
}



/* change password  --
 * NOTE: an FPLogin must already have completed successfully for this
 *       to work. this also does a little pre-processing before it hands
 *       it off to the uam. 
 */
int afp_changepw(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    char username[MACFILELEN + 1], *start = ibuf;
    struct uam_obj *uam;
    struct passwd *pwd;
    size_t len;
    int    ret;

    *rbuflen = 0;
    ibuf += 2;

    /* check if password change is allowed, OS-X ignores the flag.
     * we shouldn't trust the client on this anyway.
     * not sure about the "right" error code, NOOP for now */ 
    if (!(obj->options.passwdbits & PASSWD_SET))
         return AFPERR_NOOP;

    /* make sure we can deal w/ this uam */
    len = (unsigned char) *ibuf++;
    if ((uam = auth_uamfind(UAM_SERVER_CHANGEPW, ibuf, len)) == NULL)
        return AFPERR_BADUAM;

    ibuf += len;
    if ((len + 1) & 1) /* pad byte */
        ibuf++;

    if ( afp_version < 30) {
        len = (unsigned char) *ibuf++;
        if ( len > sizeof(username) - 1) {
            return AFPERR_PARAM;
        }
        memcpy(username, ibuf, len);
        username[ len ] = '\0';
        ibuf += len;
        if ((len + 1) & 1) /* pad byte */
            ibuf++;
    } else {
	/* AFP > 3.0 doesn't pass the username, APF 3.1 specs page 124 */
	if ( ibuf[0] != '\0' || ibuf[1] != '\0')
	    return AFPERR_PARAM;
        ibuf += 2;
	len = MIN(sizeof(username), strlen(obj->username));
        memcpy(username, obj->username, len);
	username[ len ] = '\0';
    }
        

    LOG(log_info, logtype_afpd, "changing password for <%s>", username);

    if (( pwd = uam_getname( obj, username, sizeof(username))) == NULL )
        return AFPERR_PARAM;

    /* send it off to the uam. we really don't use ibuflen right now. */
    ibuflen -= (ibuf - start);
    ret = uam->u.uam_changepw(obj, username, pwd, ibuf, ibuflen,
                              rbuf, rbuflen);
    LOG(log_info, logtype_afpd, "password change %s.",
        (ret == AFPERR_AUTHCONT) ? "continued" :
        (ret ? "failed" : "succeeded"));
    if ( ret == AFP_OK )
	set_auth_switch(0);
	
    return ret;
}


/* FPGetUserInfo */
int afp_getuserinfo(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj _U_;
char	*ibuf, *rbuf;
int	ibuflen _U_, *rbuflen;
{
    u_int8_t  thisuser;
    u_int32_t id;
    u_int16_t bitmap;

    LOG(log_debug, logtype_afpd, "begin afp_getuserinfo:");

    *rbuflen = 0;
    ibuf++;
    thisuser = *ibuf++;
    ibuf += sizeof(id); /* userid is not used in AFP 2.0 */
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);

    /* deal with error cases. we don't have to worry about
     * AFPERR_ACCESS or AFPERR_NOITEM as geteuid and getegid always
     * succeed. */
    if (!thisuser)
        return AFPERR_PARAM;
    if ((bitmap & USERIBIT_ALL) != bitmap)
        return AFPERR_BITMAP;

    /* copy the bitmap back to reply buffer */
    memcpy(rbuf, ibuf, sizeof(bitmap));
    rbuf += sizeof(bitmap);
    *rbuflen = sizeof(bitmap);

    /* copy the user/group info */
    if (bitmap & USERIBIT_USER) {
        id = htonl(geteuid());
        memcpy(rbuf, &id, sizeof(id));
        rbuf += sizeof(id);
        *rbuflen += sizeof(id);
    }

    if (bitmap & USERIBIT_GROUP) {
        id = htonl(getegid());
        memcpy(rbuf, &id, sizeof(id));
        rbuf += sizeof(id);
        *rbuflen += sizeof(id);
    }

#ifdef HAVE_NFSv4_ACLS
    if (bitmap & USERIBIT_UUID) {
        int ret;
        uuid_t uuid;
        char *uuidstring;
        
	if ( ! (obj->options.flags & OPTION_UUID))
	    return AFPERR_BITMAP;
	LOG(log_debug, logtype_afpd, "afp_getuserinfo: get UUID for \'%s\'", obj->username);
	ret = getuuidfromname( obj->username, UUID_USER, uuid);
	if (ret != 0) {
	    LOG(log_info, logtype_afpd, "afp_getuserinfo: error getting UUID !");
	    return AFPERR_NOITEM;
	}
	uuid_bin2string( uuid, &uuidstring);
	LOG(log_debug, logtype_afpd, "afp_getuserinfo: got UUID: %s", uuidstring);
	free(uuidstring);
	memcpy(rbuf, uuid, UUID_BINSIZE);
	rbuf += UUID_BINSIZE;
	*rbuflen += UUID_BINSIZE;
    }
#endif
    return AFP_OK;
    LOG(log_debug, logtype_afpd, "END afp_getuserinfo:");
}

#define UAM_LIST(type) (((type) == UAM_SERVER_LOGIN || (type) == UAM_SERVER_LOGIN_EXT) ? &uam_login : \
			(((type) == UAM_SERVER_CHANGEPW) ? \
			 &uam_changepw : NULL))

/* just do a linked list search. this could be sped up with a hashed
 * list, but i doubt anyone's going to have enough uams to matter. */
struct uam_obj *auth_uamfind(const int type, const char *name,
                                         const int len)
{
    struct uam_obj *prev, *start;

    if (!name || !(start = UAM_LIST(type)))
        return NULL;

    prev = start;
    while ((prev = prev->uam_prev) != start)
        if (strndiacasecmp(prev->uam_name, name, len) == 0)
            return prev;

    return NULL;
}

int auth_register(const int type, struct uam_obj *uam)
{
    struct uam_obj *start;

    if (!uam || !uam->uam_name || (*uam->uam_name == '\0'))
        return -1;

    if (!(start = UAM_LIST(type)))
        return 1; /* we don't know what to do with it, caller must free it */

    uam_attach(start, uam);
    return 0;
}

/* load all of the modules */
int auth_load(const char *path, const char *list)
{
    char name[MAXPATHLEN + 1], buf[MAXPATHLEN + 1], *p;
    struct uam_mod *mod;
    struct stat st;
    size_t len;

    if (!path || !*path || !list || (len = strlen(path)) > sizeof(name) - 2)
        return -1;

    strlcpy(buf, list, sizeof(buf));
    if ((p = strtok(buf, ",")) == NULL)
        return -1;

    strcpy(name, path);
    if (name[len - 1] != '/') {
        strcat(name, "/");
        len++;
    }

    while (p) {
        strlcpy(name + len, p, sizeof(name) - len);
        LOG(log_debug, logtype_afpd, "uam: loading (%s)", name);
        /*
        if ((stat(name, &st) == 0) && (mod = uam_load(name, p))) {
        */
        if (stat(name, &st) == 0) {
            if ((mod = uam_load(name, p))) {
                uam_attach(&uam_modules, mod);
                LOG(log_info, logtype_afpd, "uam: %s loaded", p);
            } else {
                LOG(log_info, logtype_afpd, "uam: %s load failure",p);
            }
        } else {
            LOG(log_info, logtype_afpd, "uam: uam not found (status=%d)", stat(name, &st));
        }
        p = strtok(NULL, ",");
    }

    return 0;
}

/* get rid of all of the uams */
void auth_unload()
{
    struct uam_mod *mod, *prev, *start = &uam_modules;

    prev = start->uam_prev;
    while ((mod = prev) != start) {
        prev = prev->uam_prev;
        uam_detach(mod);
        uam_unload(mod);
    }
}
