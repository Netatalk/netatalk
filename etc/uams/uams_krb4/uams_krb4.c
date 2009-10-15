/*
 * $Id: uams_krb4.c,v 1.10 2009-10-15 11:39:48 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#if defined( KRB ) || defined( UAM_AFSKRB )
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <limits.h>

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

#include <ctype.h>
#include <pwd.h>
#include <atalk/logger.h>
#include <netinet/in.h>
#include <des.h>
#include <krb.h>
#if 0
#include <prot.h>
#endif /* 0 */

#include <netatalk/endian.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/util.h>
#include <atalk/uam.h>

static C_Block			seskey;
static Key_schedule		seskeysched;

static char		realm[ REALM_SZ ];

#ifdef UAM_AFSKRB
static int		validseskey = 0;
static int		logged = 0;
static char		*tktfile;
static char		instance[ INST_SZ ], name[ ANAME_SZ ];
#endif /* UAM_AFSKRB */

#ifdef AFS
#include <afs/stds.h>
#include <rx/rxkad.h>
#include <afs/afs.h>
#include <afs/venus.h>
#include <afs/afsint.h>

char *ka_LocalCell();

struct ClearToken {
    int32_t AuthHandle;
    char HandShakeKey[8];
    int32_t ViceId;
    int32_t BeginTimestamp;
    int32_t EndTimestamp;
};
#endif /* AFS */


#ifdef KRB

static void lcase( p )
    char	*p;
{
    for (; *p; p++ ) {
	if ( isupper( *p )) {
	    *p = tolower( *p );
	}
    }
    return;
}

static void ucase( p )
    char	*p;
{
    for (; *p; p++ ) {
	if ( islower( *p )) {
	    *p = toupper( *p );
	}
    }
    return;
}

#define KRB4CMD_HELO	1
#define KRB4RPL_REALM	2
#define KRB4WRT_SESS	3
#define KRB4RPL_DONE	4
#define KRB4RPL_PRINC	5
#define KRB4WRT_TOKEN	6
#define KRB4WRT_SKIP	7
#define KRB4RPL_DONEMUT	8
#define KRB4CMD_SESS	9
#define KRB4CMD_TOKEN	10
#define KRB4CMD_SKIP	11

static int krb4_login(void *obj, struct passwd **uam_pwd,
		      char *ibuf, size_t ibuflen,
		      char *rbuf, size_t *rbuflen )
{
    char		*p;
    char		*username;
    struct passwd	*pwd;
    u_int16_t		len;
    KTEXT_ST		tkt;
    static AUTH_DAT	ad;
    int			rc, proto;
    size_t		ulen;
    char		inst[ 40 ], princ[ 40 ];

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, &username, &ulen) < 0)
      return AFPERR_MISC;

    if (uam_afpserver_option(obj, UAM_OPTION_PROTOCOL, &proto, NULL) < 0)
      return AFPERR_MISC;

    switch( *ibuf ) {
	case KRB4CMD_SESS:
	    LOG(log_info, logtype_default, "krb4_login: KRB4CMD_SESS" );
	    ++ibuf;
	    p = ibuf;
	    memcpy( &len, p, sizeof( u_int16_t ));
	    tkt.length = ntohs( len );
	    p += sizeof( u_int16_t );

	    if ( tkt.length <= 0 || tkt.length > MAX_KTXT_LEN ) {
		*rbuflen = 0;
		LOG(log_info, logtype_default, "krb4_login: tkt.length = %d", tkt.length );
		return( AFPERR_BADUAM );
	    }

	    memcpy( tkt.dat, p, tkt.length );
	    p += tkt.length;

	    strcpy( inst, "*" );

	    switch( proto ) {
		case AFPPROTO_ASP:
		    strcpy( princ, "afpserver" );
		    break;
		case AFPPROTO_DSI:
		    strcpy( princ, "rcmd" );
		    break;
	    }

	    if( (rc = krb_rd_req( &tkt, princ, inst, 0, &ad, "" )) 
		!= RD_AP_OK ) {
		LOG(log_error, logtype_default, 
			"krb4_login: krb_rd_req(): %s", krb_err_txt[ rc ] );
		*rbuflen = 0;
		return( AFPERR_BADUAM );
	    }

	    LOG(log_info, logtype_default, "krb4_login: %s.%s@%s", ad.pname, ad.pinst, 
		ad.prealm );
	    strcpy( realm, ad.prealm );
	    memcpy( seskey, ad.session, sizeof( C_Block ) );
	    key_sched( (C_Block *)seskey, seskeysched );

	    strncpy( username, ad.pname, ulen );

	    p = rbuf;

#ifdef AFS
	    *p = KRB4RPL_DONE;	/* XXX */
	    *rbuflen = 1;

	    if (( pwd = uam_getname( obj, ad.pname, strlen(ad.pname) )) == NULL ) {
		return AFPERR_PARAM;
	    }
/*
	    if (uam_checkuser( pwd ) < 0) {
		return AFPERR_NOTAUTH;
	    }
*/
	    *uam_pwd = pwd;
	    return( AFP_OK ); /* Should this be AFPERR_AUTHCONT? */
#else /* AFS */
	    /* get principals */
	    *p++ = KRB4RPL_PRINC;
	    len = strlen( realm );
	    *p++ = len + 1;
	    *p++ = '@';
	    strcpy( p, realm );
	    p += len + 1;
	    break;
#endif /* AFS */
	case KRB4CMD_HELO:
	    p = rbuf;
	    if (krb_get_lrealm( realm, 1 ) != KSUCCESS ) {
		LOG(log_error, logtype_default, "krb4_login: can't get local realm!" );
		return( AFPERR_NOTAUTH );
	    }
	    *p++ = KRB4RPL_REALM;
	    *p++ = 1;
	    len = strlen( realm );
	    *p++ = len;
	    strcpy( p, realm );
	    p += len + 1;
	    break;

	default:
	    *rbuflen = 0;
	    LOG(log_info, logtype_default, "krb4_login: bad command %d", *ibuf );
	    return( AFPERR_NOTAUTH );
    }

#ifdef AFS
    if ( setpag() < 0 ) {
	*rbuflen = 0;
	LOG(log_error, logtype_default, "krb_login: setpag: %s", strerror(errno) );
	return( AFPERR_BADUAM );
    }
#endif /*AFS*/

    *rbuflen = p - rbuf;
    return( AFPERR_AUTHCONT );
}

static int krb4_action( void *v1, void *v2, const int i )
{
	return i;
}

/*
   I have a hunch that problems might arise on platforms 
   with non-16bit short's and non-32bit int's
*/
static int krb4_logincont(void *obj, struct passwd **uam_pwd,
			  char *ibuf, size_t ibuflen,
			  char *rbuf, size_t *rbuflen)
{
    static struct passwd	*pwd;
    KTEXT_ST		tkt;
    static AUTH_DAT	ad;
    int			rc;
    u_int16_t	        len;
    char		*p, *username, *servername;
    CREDENTIALS		cr;
#ifdef AFS
    struct ViceIoctl	vi;
    struct ClearToken	ct;
#endif /* AFS */
    char		buf[ 1024 ];
    int			aint, ulen, snlen;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, &username, &ulen) < 0)
      return AFPERR_MISC;

    if (uam_afpserver_option(obj, UAM_OPTION_HOSTNAME, &servername, &snlen) < 0)
      return AFPERR_MISC;

    ibuf++;
    switch ( rc = *ibuf++ ) {
	case KRB4CMD_TOKEN :
#ifdef AFS
	    p = buf;
	    memcpy( &len, ibuf, sizeof( u_int16_t ) );
	    ibuf += sizeof( len );
	    len = ntohs( len );
	    aint = len;
	    memcpy( p, &aint, sizeof( int ) );
	    p += sizeof( int );
	    memcpy( p, ibuf, len );
	    pcbc_encrypt( (C_Block *)p, (C_Block *)p, len, seskeysched,
		seskey, DECRYPT );
	    p += len;
	    ibuf += len;

	    memcpy( &len, ibuf, sizeof( u_int16_t ) );
	    ibuf += sizeof( u_int16_t ) );
	    len = ntohs( len );

	    if ( len != sizeof( struct ClearToken ) ) {
		LOG(log_error, logtype_default, "krb4_logincont: token too short" );
		*rbuflen = 0;
		return( AFPERR_BADUAM );
	    }
	    memcpy( &ct, ibuf, len );

	    pcbc_encrypt( (C_Block *)&ct, (C_Block *)&ct, len, 
		seskeysched, seskey, DECRYPT );

	    aint = sizeof( struct ClearToken );
	    memcpy( p, &aint, sizeof( int ) );
	    p += sizeof( int );
	    memcpy( p, &ct, sizeof( struct ClearToken ) );
	    p += sizeof( struct ClearToken );

	    aint = 0;
	    memcpy( p, &aint, sizeof( int ) );
	    p += sizeof( int );

	    lcase( realm );
	    strcpy( p, realm );
	    p += strlen( realm ) + 1;

	    vi.in = buf;
	    vi.in_size = p - buf;
	    vi.out = buf;
	    vi.out_size = sizeof( buf );

	    if ( pioctl( 0, VIOCSETTOK, &vi, 0 ) < 0 ) {
		LOG(log_error, logtype_default, "krb4_logincont: pioctl: %s", strerror(errno) );
		*rbuflen = 0;
		return( AFPERR_BADUAM );
	    }
	    /* FALL THROUGH */

	case KRB4CMD_SKIP:
	    p = rbuf;
	    *p = KRB4RPL_DONE;	/* XXX */
	    *rbuflen = 1;

	    if (( pwd = uam_getname( obj, username, strlen(username) ) ) == NULL ) {
		return( AFPERR_NOTAUTH );
	    }
/*
	    if (uam_checkuser(pwd) < 0) {
		return AFPERR_NOTAUTH;
	    }
*/
	    *uam_pwd = pwd;
	    return( AFP_OK );
#endif /* AFS */
	default:
	    /* read in the rest */
	    if (uam_afp_read(obj, rbuf, rbuflen, krb4_action) < 0)
		return AFPERR_PARAM;

	    p = rbuf;
	    switch ( rc = *p++ ) {
		case KRB4WRT_SESS :
		    memcpy( &len, p, sizeof( len ));
		    tkt.length = ntohs( len );
		    p += sizeof( short );

		    if ( tkt.length <= 0 || tkt.length > MAX_KTXT_LEN ) {
			return( AFPERR_BADUAM );
		    }
		    memcpy( tkt.dat, p, tkt.length );
		    p += tkt.length;

		    if (( rc = krb_rd_req( &tkt, "afpserver", servername, 
			0, &ad, "" )) != RD_AP_OK ) {
			LOG(log_error, logtype_default, "krb4_logincont: krb_rd_req(): %s", krb_err_txt[ rc ] );
			return( AFPERR_BADUAM );
		    }

		    LOG(log_info, logtype_default, "krb4_login: %s.%s@%s", ad.pname, 
			ad.pinst, ad.prealm );
		    memcpy(realm, ad.prealm, sizeof(realm));
		    memcpy(seskey, ad.session, sizeof( C_Block ));
		    key_sched((C_Block *) seskey, seskeysched );

		    strncpy(username, ad.pname, ulen);

		    p = rbuf;
#ifndef AFS
		    *p = KRB4RPL_DONE;	/* XXX */
		    *rbuflen = 1;

		    if (( pwd = uam_getname( obj, ad.pname, strlen(ad.pname) )) 
			== NULL ) {
			return( AFPERR_PARAM );
		    }
		    *uam_pwd = pwd;
		    return AFP_OK;
#else /* ! AFS */
		    /* get principals */
		    *p++ = KRB4RPL_PRINC;
		    len = strlen( realm );
		    *p++ = len + 1;
		    *p++ = '@';
		    strcpy( p, realm );
		    p += len + 1;
		    *rbuflen = p - rbuf;
		    return( AFPERR_AUTHCONT );

		case KRB4WRT_TOKEN :
		    memcpy( &len, p, sizeof( len ));
		    len = ntohs( len );
		    p += sizeof( len );
		    memcpy( &cr, p, len );

		    pcbc_encrypt((C_Block *)&cr, (C_Block *)&cr, len, 
			seskeysched, seskey, DES_DECRYPT );

		    p = buf;
		    cr.ticket_st.length = ntohl( cr.ticket_st.length );
		    memcpy( p, &cr.ticket_st.length, sizeof( int ));
		    p += sizeof( int );
		    memcpy( p, cr.ticket_st.dat, cr.ticket_st.length );
		    p += cr.ticket_st.length;

		    ct.AuthHandle = ntohl( cr.kvno );
		    memcpy( ct.HandShakeKey, cr.session, sizeof( cr.session ));
		    ct.ViceId = 0;
		    ct.BeginTimestamp = ntohl( cr.issue_date );
		    ct.EndTimestamp = krb_life_to_time( ntohl( cr.issue_date ),
		    ntohl( cr.lifetime ));

		    aint = sizeof( struct ClearToken );
		    memcpy( p, &aint, sizeof( int ));
		    p += sizeof( int );
		    memcpy( p, &ct, sizeof( struct ClearToken ));
		    p += sizeof( struct ClearToken );

		    aint = 0;
		    memcpy( p, &aint, sizeof( int ));
		    p += sizeof( int );

		    lcase( realm );
		    strcpy( p, realm );
		    p += strlen( realm ) + 1;

		    vi.in = buf;
		    vi.in_size = p - buf;
		    vi.out = buf;
		    vi.out_size = sizeof( buf );
		    if ( pioctl( 0, VIOCSETTOK, &vi, 0 ) < 0 ) {
			LOG(log_error, logtype_default, "krb4_logincont: pioctl: %s", strerror(errno) );
			return( AFPERR_BADUAM );
		    }
		    /* FALL THROUGH */

		case KRB4WRT_SKIP :
		    p = rbuf;
		    *p = KRB4RPL_DONE;	/* XXX */
		    *rbuflen = 1;

		    if (( pwd = uam_getname( obj, ad.pname, strlen(ad.pname) )) 
			== NULL ) {
			return( AFPERR_PARAM );
		    }
		    *uam_pwd = pwd;
		    return AFP_OK;
#endif /*AFS*/

		default:
		    LOG(log_info, logtype_default, "krb4_logincont: bad command %d", rc );
		    *rbuflen = 0;
		    return( AFPERR_NOTAUTH );
		    break;
	    }
	    break;
    }
}

#endif /* KRB */


#ifdef AFS
#include <rx/rxkad.h>
#include <afs/afsint.h>

char *ka_LocalCell();

static void
addrealm(realm,cells)
    char *realm;
	char ***cells;
{
    char **ptr;
	int temp;

	ptr= *cells;

    for(;*ptr != 0 ;ptr++)
        if(!strcmp(realm,*ptr))
            return;

	temp=ptr- *cells;
	*cells=(char**)realloc(*cells,((2+temp)*sizeof(char*)));
	ptr= *cells+temp;

    *ptr=(char*)malloc(strlen(realm)+1);
    strcpy(*ptr++,realm);
	*ptr=0;
    return;
}

static int kcheckuser(pwd,passwd)
	struct passwd *pwd;
	char *passwd;
{
	int32_t code;
	char *instance="";
	char realm[MAXKTCREALMLEN];
	char lorealm[MAXKTCREALMLEN];
	char *cell;
	Date lifetime=MAXKTCTICKETLIFETIME;
	int rval;
	char **cells=(char **)malloc(sizeof(char*));
	char *temp;
	int rc,cellNum;
	struct ktc_principal serviceName;

	*cells=0;

	code = ka_Init(0);

	{
		char *temp,*temp1;
		temp=(char*)malloc(strlen(pwd->pw_dir)+1);
		strcpy(temp,pwd->pw_dir);
		temp1=temp;
		temp=strtok(temp,"/");
		temp=strtok('\0',"/");
		ka_CellToRealm(temp,realm,0);
		addrealm(realm,&cells);
		free(temp1);
	}

	setpag();
	authenticate(cells,pwd->pw_name,passwd);
	cellNum=0;
	rc=ktc_ListTokens(cellNum,&cellNum,&serviceName);
	if(rc)
		rval=1;
	else{
		rval=0;
	}

	return(rval);
}

static void authenticate(cells,name,passwd)
	char **cells;
	char *name;
	char *passwd;
{
	char **ptr=cells;
	char *errorstring;

	while(*ptr){
	    ka_UserAuthenticate(name,/*instance*/"",/*cell*/*ptr++,
		    passwd,/*setpag*/0,&errorstring);
	}
}
#endif /* AFS */

#if defined( UAM_AFSKRB ) && defined( AFS )
static int afskrb_login(void *obj, struct passwd *uam_pwd,
			char *ibuf, size_t ibuflen, 
			char *rbuf, size_t *rbuflen )
{
    KTEXT_ST	authent, rpkt;
    CREDENTIALS	cr;
    char	*p, *q, *username, servername;
    int		len, rc, whoserealm, ulen, snlen;
    short	slen;

    *rbuflen = 0;
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, &username, &ulen) < 0)
      return AFPERR_MISC;

    if (uam_afpserver_option(obj, UAM_OPTION_HOSTNAME, &servername, &snlen ) < 0)
      return AFPERR_MISC;
    
    len = (unsigned char) *ibuf++;
    ibuf[ len ] = '\0';
    if (( p = strchr( ibuf, '@' )) != NULL ) {
	*p++ = '\0';
	strcpy( realm, p );
	ucase( realm );
	whoserealm = 0;
    } else {
	if ( krb_get_lrealm( realm, 1 ) != KSUCCESS ) {
	    return AFPERR_BADUAM;
	}
	whoserealm = 1;
    }
    if (( p = strchr( ibuf, '.' )) != NULL ) {
	*p++ = '\0';
	strcpy( instance, p );
    } else {
	*instance = '\0';
    }
    strcpy( name, ibuf );
    /*
     * We don't have the session key, yet. Get one.
     */
    p = rbuf;
    if ( validseskey == 0 ) {
	if ( setpag() < 0 ) {
	    LOG(log_error, logtype_default, "krb_login: setpag: %s", strerror(errno) );
	    return AFPERR_BADUAM;
	}
	krb_set_tkt_string(( tktfile = mktemp( _PATH_AFPTKT )));
	if (( rc =  krb_get_svc_in_tkt( "afpserver", servername, realm,
		TICKET_GRANTING_TICKET, realm, 255, KEYFILE )) != INTK_OK ) {
	    LOG(log_error, logtype_default, "krb_login: can't get ticket-granting-ticket" );
	    return (( whoserealm ) ? AFPERR_BADUAM : AFPERR_PARAM );
	}
	if ( krb_mk_req( &authent, name, instance, realm, 0 ) != KSUCCESS ) {
	    return ( AFPERR_PARAM );
	}
	if ( krb_get_cred( name, instance, realm, &cr ) != KSUCCESS ) {
	    return ( AFPERR_BADUAM );
	}

	if ( unlink( tktfile ) < 0 ) {
	    LOG(log_error, logtype_default, "krb_login: unlink %s: %s", tktfile, strerror(errno) );
	    return ( AFPERR_BADUAM );
	}

	memcpy( seskey, cr.session, sizeof( C_Block ));
	key_sched((C_Block *) seskey, seskeysched );
	validseskey = 1;
	strncpy(username, name, ulen);

	memcpy( p, authent.dat, authent.length );
	p += authent.length;
    }

    if ( kuam_get_in_tkt( name, instance, realm, TICKET_GRANTING_TICKET,
	    realm, 255, &rpkt ) != INTK_OK ) {
	return ( AFPERR_PARAM );
    }


    q = (char *)rpkt.dat;
    *p++ = *q++;
    *p++ = *q++;
    while ( *q++ )
	;
    while ( *q++ )
	;
    while ( *q++ )
	;
    q += 10;

    len = strlen( realm );
    strcpy( p, realm );
    p += len + 1;
    memcpy( &slen, q, sizeof( short ));
    memcpy( p, &slen, sizeof( short ));
    p += sizeof( short );
    q += sizeof( short );
    memcpy( p, q, slen );
    p += slen;

    *rbuflen = p - rbuf;
    return( AFPERR_AUTHCONT );
}

static int afskrb_logincont(void *obj, struct passwd *uam_pwd,
			    char *ibuf, size_t ibuflen, 
			    char *rbuf, size_t *rbuflen )
{
    CREDENTIALS		cr;
    struct ViceIoctl	vi;
    struct ClearToken	ct;
    struct passwd	*pwd;
    char		buf[ 1024 ], *p;
    int			aint;
    short		clen;

    *rbuflen = 0;
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, &username, NULL) < 0)
      return AFPERR_MISC;
    
    ibuf += 2;
    memcpy( &clen, ibuf, sizeof( short ));
    clen = ntohs( clen );
    ibuf += sizeof( short );

    pcbc_encrypt((C_Block *)ibuf, (C_Block *)ibuf,
	    clen, seskeysched, seskey, DES_DECRYPT );
    if ( kuam_set_in_tkt( name, instance, realm, TICKET_GRANTING_TICKET,
	    realm, ibuf ) != INTK_OK ) {
	return ( AFPERR_PARAM );
    }

    if ( get_ad_tkt( "afs", "", realm, 255 ) != KSUCCESS ) {
	return ( AFPERR_PARAM );
    }
    if ( krb_get_cred( "afs", "", realm, &cr ) != KSUCCESS ) {
	return ( AFPERR_PARAM );
    }

    p = buf;
    memcpy( p, &cr.ticket_st.length, sizeof( int ));
    p += sizeof( int );
    memcpy( p, cr.ticket_st.dat, cr.ticket_st.length );
    p += cr.ticket_st.length;

    ct.AuthHandle = cr.kvno;
    memcpy( ct.HandShakeKey, cr.session, sizeof( cr.session ));
    ct.ViceId = 0;
    ct.BeginTimestamp = cr.issue_date;
    /* ct.EndTimestamp = cr.issue_date + ( cr.lifetime * 5 * 60 ); */
    ct.EndTimestamp = krb_life_to_time( cr.issue_date, cr.lifetime );

    aint = sizeof( struct ClearToken );
    memcpy( p, &aint, sizeof( int ));
    p += sizeof( int );
    memcpy( p, &ct, sizeof( struct ClearToken ));
    p += sizeof( struct ClearToken );

    aint = 0;
    memcpy( p, &aint, sizeof( int ));
    p += sizeof( int );

    lcase( realm );
    strcpy( p, realm );
    p += strlen( realm ) + 1;

    vi.in = buf;
    vi.in_size = p - buf;
    vi.out = buf;
    vi.out_size = sizeof( buf );
    if ( pioctl( 0, VIOCSETTOK, &vi, 0 ) < 0 ) {
	LOG(log_error, logtype_default, "krb_logincont: pioctl: %s", strerror(errno) );
	return ( AFPERR_BADUAM );
    }

    if ( unlink( tktfile ) < 0 ) {
	LOG(log_error, logtype_default, "krb_logincont: %s: %s", tktfile, strerror(errno) );
	return ( AFPERR_BADUAM );
    }

    if (( pwd = uam_getname( obj, username, strlen(username) )) == NULL ) {
	return ( AFPERR_PARAM );
    }

    if ( logged == 0 ) {
	logged = 1;
	LOG(log_info, logtype_default, "authenticated %s.%s@%s", name, instance, realm );
	*uam_pwd = pwd;
	return AFP_OK;
    }
    LOG(log_info, logtype_default, "re-authenticated %s.%s@%s", name, instance, realm );
    return( AFP_OK );
}
#endif /* UAM_AFSKRB AFS */

static int uam_setup(const char *path)
{
#ifdef KRB
   uam_register(UAM_SERVER_LOGIN, path, "Kerberos IV", krb4_login, 
		krb4_logincont, NULL);
   /* uam_afpserver_action(); */
#endif /* KRB */
#ifdef UAM_AFSKRB
   uam_register(UAM_SERVER_LOGIN, path, "AFS Kerberos", afskrb_login, 
		afskrb_logincont, NULL);
   /* uam_afpserver_action(); */
#endif /* UAM_AFSKRB */
   return 0;
}

static void uam_cleanup(void)
{
#ifdef KRB
   /* uam_afpserver_action(); */
   uam_unregister(UAM_SERVER_LOGIN, "Kerberos IV");
#endif
#ifdef UAM_AFSKRB
   /* uam_afpserver_action(); */
   uam_unregister(UAM_SERVER_LOGIN, "AFS Kerberos");
#endif
}

UAM_MODULE_EXPORT struct uam_export uams_krb4 = {
  UAM_MODULE_SERVER,
  UAM_MODULE_VERSION,
  uam_setup, uam_cleanup
};

#endif /* KRB or UAM_AFSKRB */

