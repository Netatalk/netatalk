/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 * modified from main.c. this handles afp options.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <atalk/logger.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#ifdef ADMIN_GRP
#include <grp.h>
#include <sys/types.h>
#endif /* ADMIN_GRP */

#include <atalk/paths.h>
#include <atalk/util.h>
#include <atalk/compat.h>
#include <atalk/globals.h>
#include <atalk/fce_api.h>

#include "status.h"
#include "auth.h"
#include "dircache.h"

#define LENGTH 512

/* get rid of any allocated afp_option buffers. */
void afp_options_free(struct afp_options *opt)
{
	if (opt->adminauthuser)
        free(opt->adminauthuser);
	if (opt->configfile)
        free(opt->configfile);
    if (opt->fqdn)
        free(opt->fqdn);
    if (opt->guest)
        free(opt->guest);
    if (opt->ipaddr)
        free(opt->ipaddr);
    if (opt->k5realm)
        free(opt->k5realm);
    if (opt->k5keytab)
        free(opt->k5keytab);
    if (opt->k5service)
        free(opt->k5service);
    if (opt->logconfig)
        free(opt->logconfig);
    if (opt->loginmesg)
        free(opt->loginmesg);
    if (opt->maccodepage)
        free(opt->maccodepage);
	if (opt->mimicmodel)
        free(opt->mimicmodel);
    if (opt->ntdomain)
        free(opt->ntdomain);
    if (opt->ntseparator)
        free(opt->ntseparator);
    if (opt->passwdfile)
        free(opt->passwdfile);
    if (opt->port)
        free(opt->port);
    if (opt->server)
        free(opt->server);
    if (opt->signatureopt)
        free(opt->signatureopt);
    if (opt->uamlist)
        free(opt->uamlist);
    if (opt->uampath)
        free(opt->uampath);
    if (opt->unixcodepage)
        free(opt->unixcodepage);
}

#define MAXVAL 1024
int afp_config_parse(AFPObj *AFPObj)
{
    dictionary *config;
    struct afp_options *options = &AFPObj->options;
    int i;
    const char *p, *tmp;
    char val[MAXVAL];

    memset(options, 0, sizeof(struct afp_options));
    options->configfile  = strdup(_PATH_CONFDIR "afp.conf");
    options->sigconffile = strdup(_PATH_CONFDIR "afp_signature.conf");
    options->uuidconf    = strdup(_PATH_CONFDIR "afp_voluuid.conf");
    options->flags |= OPTION_ACL2MACCESS | OPTION_UUID | OPTION_SERVERNOTIF;

    while (EOF != (p = getopt(AFPObj->argc, AFPObj->argv, "dF:"))) {
        switch (p) {
        case 'd':
            options->flags |= OPTION_DEBUG;
            break;
        case 'F':
            if (options->configfile)
                free(options->configfile);
            options->configfile = strdup(optarg);
            break;
        default :
            break;
        }
    }

    if ((config = iniparser_load(AFPObj->options.configfile)) == NULL)
        return -1;
    AFPObj->iniconfig = config;

    /* [Global] */
    options->logconfig = iniparser_getstring(config, INISEC_GLOBAL, "loglevel", "default:note");
    options->logfile   = iniparser_getstring(config, INISEC_GLOBAL, "logfile",  NULL);
    set_processname("afpd");
    setuplog(logconfig, logfile);

    /* [AFP] "options" options wo values */
    p = iniparser_getstring(config, INISEC_AFP, "options", "");
    strcpy(val, " ");
    strlcat(val, p, MAXVAL);

    if (strstr(val, " nozeroconf"))
        options->flags |= OPTION_NOZEROCONF;
    if (strstr(val, " icon"))
        options->flags |= OPTION_CUSTOMICON;
    if (strstr(val, " noicon"))
        options->flags &= ~OPTION_CUSTOMICON;
    if (strstr(val, " advertise_ssh"))
        options->flags |= OPTION_ANNOUNCESSH;
    if (strstr(val, " noacl2maccess"))
        options->flags &= ~OPTION_ACL2MACCESS;
    if (strstr(val, " keepsessions"))
        options->flags |= OPTION_KEEPSESSIONS;
    if (strstr(val, " keepsessions"))
        options->flags |= OPTION_CLOSEVOL;
    if (strstr(val, " client_polling"))
        options->flags &= ~OPTION_SERVERNOTIF;
    if (strstr(val, " nosavepassword"))
        options->passwdbits |= PASSWD_NOSAVE;
    if (strstr(val, " savepassword"))
        options->passwdbits &= ~PASSWD_NOSAVE;
    if (strstr(val, " nosetpassword"))
        options->passwdbits &= ~PASSWD_SET;
    if (strstr(val, " setpassword"))
        options->passwdbits |= PASSWD_SET;

    /* figure out options w values */

    options->loginmesg      = iniparser_getstrdup(config, INISEC_AFP, "loginmesg",      "");
    options->guest          = iniparser_getstrdup(config, INISEC_AFP, "guestname",      "nobody");
    options->passwdfile     = iniparser_getstrdup(config, INISEC_AFP, "passwdfile",     _PATH_AFPDPWFILE);
    options->uampath        = iniparser_getstrdup(config, INISEC_AFP, "uampath",        _PATH_AFPDUAMPATH);
    options->uamlist        = iniparser_getstrdup(config, INISEC_AFP, "uamlist",        "uams_dhx.so,uams_dhx2.so");
    options->port           = iniparser_getstrdup(config, INISEC_AFP, "port",           "548");
    options->signatureopt   = iniparser_getstrdup(config, INISEC_AFP, "signature",      "auto");
    options->k5service      = iniparser_getstrdup(config, INISEC_AFP, "k5service",      NULL);
    options->k5realm        = iniparser_getstrdup(config, INISEC_AFP, "k5realm",        NULL);
    options->authprintdir   = iniparser_getstrdup(config, INISEC_AFP, "authprintdir",   NULL);
    options->listen         = iniparser_getstrdup(config, INISEC_AFP, "listen",         NULL);
    options->hostname       = iniparser_getstrdup(config, INISEC_AFP, "hostname",       NULL);
    options->ntdomain       = iniparser_getstrdup(config, INISEC_AFP, "ntdomain",       NULL);
    options->ntseparator    = iniparser_getstrdup(config, INISEC_AFP, "ntseparator",    NULL);
    options->mimicmodel     = iniparser_getstrdup(config, INISEC_AFP, "mimicmodel",     NULL);
    options->adminauthuser  = iniparser_getstrdup(config, INISEC_AFP, "adminauthuser",  NULL);
    options->connections    = iniparser_getint   (config, INISEC_AFP, "maxcon",         200);
    options->passwdminlen   = iniparser_getint   (config, INISEC_AFP, "passwdminlen",   0);
    options->tickleval      = iniparser_getint   (config, INISEC_AFP, "tickleval",      30);
    options->timeout        = iniparser_getint   (config, INISEC_AFP, "timeout",        4);
    options->dsireadbuf     = iniparser_getint   (config, INISEC_AFP, "dsireadbuf",     12);
    options->server_quantum = iniparser_getint   (config, INISEC_AFP, "server_quantum", DSI_SERVQUANT_DEF);
    options->volnamelen     = iniparser_getint   (config, INISEC_AFP, "volnamelen",     80);
    options->dircachesize   = iniparser_getint   (config, INISEC_AFP, "dircachesize",   DEFAULT_MAX_DIRCACHE_SIZE);
    options->tcp_sndbuf     = iniparser_getint   (config, INISEC_AFP, "tcpsndbuf",      0);
    options->tcp_rcvbuf     = iniparser_getint   (config, INISEC_AFP, "tcprcvbuf",      0);
    options->fce_fmodwait   = iniparser_getint   (config, INISEC_AFP, "fceholdfmod",    60);
    options->sleep          = iniparser_getint   (config, INISEC_AFP, "sleep",          10) * 60 * 2;
    options->disconnect     = iniparser_getint   (config, INISEC_AFP, "disconnect"      24) * 60 * 2;


    if ((p = iniparser_getstring(config, INISEC_AFP, "k5keytab", NULL))) {
        EC_NULL_LOG( options->k5keytab = malloc(strlen(p) + 14) );
        snprintf(options->k5keytab, strlen(p) + 14, "KRB5_KTNAME=%s", p);
        putenv(options->k5keytab);
    }

#ifdef ADMIN_GRP
    if ((p = iniparser_getstring(config, INISEC_AFP, "admingroup",  NULL))) {
         struct group *gr = getgrnam(p);
         if (gr != NULL)
             options->admingid = gr->gr_gid;
    }
#endif /* ADMIN_GRP */

    p = iniparser_getstring(config, INISEC_AFP, "cnidserver", "localhost:4700");
    tmp = strrchr(p, ':');
    if (tmp)
        *t = 0;
    options->Cnid_srv = strdup(p);
    if (tmp)
        options->Cnid_port = strdup(tmp + 1);
    LOG(log_debug, logtype_afpd, "CNID Server: %s:%s", options->Cnid_srv, options->Cnid_port);


    if ((p = iniparser_getstring(config, INISEC_AFP, "fqdn", NULL))) {
        /* do a little checking for the domain name. */
        tmp = strchr(c, ':');
        if (tmp)
            *tmp = '\0';
        if (gethostbyname(p)) {
            if (tmp)
                *tmp = ':';
            if ((opt = strdup(p)))
                options->fqdn = opt;
        } else {
            LOG(log_error, logtype_afpd, "error parsing -fqdn, gethostbyname failed for: %s", c);
        }
    }

    p = iniparser_getstring(config, INISEC_AFP, "unixcodepage", "LOCALE");
    if ((options->unixcharset = add_charset(p)) == (charset_t)-1) {
        options->unixcharset = CH_UNIX;
        LOG(log_warning, logtype_afpd, "Setting Unix codepage to '%s' failed", p);
    } else {
        options->unixcodepage = strdup(p);
    }
	
    p = iniparser_getstring(config, INISEC_AFP, "maccodepage", "MAC_ROMAN");
    if ((options->maccharset = add_charset(p)) == (charset_t)-1) {
        options->maccharset = CH_MAC;
        LOG(log_warning, logtype_afpd, "Setting Unix codepage to '%s' failed", p);
    } else {
        options->maccharset = strdup(p);
    }

    if ((p = iniparser_getstring(config, INISEC_AFP, "fcelistener", NULL))) {
		LOG(log_note, logtype_afpd, "Adding FCE listener: %s", p);
		fce_add_udp_socket(p);
    }
    if ((p = iniparser_getstring(config, INISEC_AFP, "fcecoalesce", NULL))) {
		LOG(log_note, logtype_afpd, "Fce coalesce: %s", p);
		fce_set_coalesce(p);
    }
    if ((p = iniparser_getstring(config, INISEC_AFP, "fceevents", NULL))) {
		LOG(log_note, logtype_afpd, "Fce events: %s", p);
		fce_set_events(p);
    }

    /* Check for sane values */
    if (options->tickleval <= 0)
        options->tickleval = 30;
    if (options->timeout <= 0)
        options->timeout = 4;
    if (options->sleep <= 4)
        options->disconnected = options->sleep = 4;
    if (options->dsireadbuf < 6)
        options->dsireadbuf = 6;
    if (options->volnamelen < 8)
        options->volnamelen = 8; /* max mangled volname "???#FFFF" */
    if (options->volnamelen > 255)
	    options->volnamelen = 255; /* AFP3 spec */

    return 0;
}

/*
 * Show version information about afpd.
 * Used by "afp -v".
 */
static void show_version( void )
{
	int num, i;

	printf( "afpd %s - Apple Filing Protocol (AFP) daemon of Netatalk\n\n", VERSION );

	puts( "This program is free software; you can redistribute it and/or modify it under" );
	puts( "the terms of the GNU General Public License as published by the Free Software" );
	puts( "Foundation; either version 2 of the License, or (at your option) any later" );
	puts( "version. Please see the file COPYING for further information and details.\n" );

	puts( "afpd has been compiled with support for these features:\n" );

	num = sizeof( afp_versions ) / sizeof( afp_versions[ 0 ] );
	printf( "          AFP versions:\t" );
	for ( i = 0; i < num; i++ ) {
		printf( "%d.%d ", afp_versions[ i ].av_number/10, afp_versions[ i ].av_number%10);
	}
	puts( "" );

	printf( "         CNID backends:\t" );
#ifdef CNID_BACKEND_CDB
	printf( "cdb ");
#endif
#ifdef CNID_BACKEND_DB3
	printf( "db3 " );
#endif
#ifdef CNID_BACKEND_DBD
#ifdef CNID_BACKEND_DBD_TXN
	printf( "dbd-txn " );
#else
	printf( "dbd " );
#endif
#endif
#ifdef CNID_BACKEND_HASH
	printf( "hash " );
#endif
#ifdef CNID_BACKEND_LAST
	printf( "last " );
#endif
#ifdef CNID_BACKEND_MTAB
	printf( "mtab " );
#endif
#ifdef CNID_BACKEND_TDB
	printf( "tdb " );
#endif
	puts( "" );
}

/*
 * Show extended version information about afpd and Netatalk.
 * Used by "afp -V".
 */
static void show_version_extended(void )
{
	show_version( );

	printf( "      Zeroconf support:\t" );
#ifdef USE_ZEROCONF
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "  TCP wrappers support:\t" );
#ifdef TCPWRAP
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "         Quota support:\t" );
#ifndef NO_QUOTA_SUPPORT
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "   Admin group support:\t" );
#ifdef ADMIN_GRP
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "    Valid shell checks:\t" );
#ifndef DISABLE_SHELLCHECK
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "      cracklib support:\t" );
#ifdef USE_CRACKLIB
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "            EA support:\t" );
	puts( EA_MODULES );

	printf( "           ACL support:\t" );
#ifdef HAVE_ACLS
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "          LDAP support:\t" );
#ifdef HAVE_LDAP
	puts( "Yes" );
#else
	puts( "No" );
#endif
}

/*
 * Display compiled-in default paths
 */
static void show_paths( void )
{
	printf( "             afpd.conf:\t%s\n", _PATH_AFPDCONF );
	printf( "   AppleVolumes.system:\t%s\n", _PATH_AFPDSYSVOL );
	printf( "  AppleVolumes.default:\t%s\n", _PATH_AFPDDEFVOL );
	printf( "    afp_signature.conf:\t%s\n", _PATH_AFPDSIGCONF );
	printf( "      afp_voluuid.conf:\t%s\n", _PATH_AFPDUUIDCONF );
#ifdef HAVE_LDAP
	printf( "         afp_ldap.conf:\t%s\n", _PATH_ACL_LDAPCONF );
#else
	printf( "         afp_ldap.conf:\tnot supported\n");
#endif
	printf( "       UAM search path:\t%s\n", _PATH_AFPDUAMPATH );
	printf( "  Server messages path:\t%s\n", SERVERTEXT);
	printf( "              lockfile:\t%s\n", _PATH_AFPDLOCK);
}

/*
 * Display usage information about afpd.
 */
static void show_usage( char *name )
{
	fprintf( stderr, "Usage:\t%s [-duptDTI] [-n nbpname]\n", name );
	fprintf( stderr, "\t     [-c maxconnections] [-g guest] [-P pidfile] [-S port] [-L message]\n" );
	fprintf( stderr, "\t     [-F configfile] [-U uams] [-m umask]\n" );
	fprintf( stderr, "\t%s -h|-v|-V\n", name );
}

int afp_options_parse_cmdline(int ac, char **av)
{
    char *p;
    char *tmp;	/* Used for error checking the result of strtol */
    int c, err = 0;
    char buf[1024];

    if (gethostname(buf, sizeof(buf)) < 0 ) {
        perror( "gethostname" );
        return 0;
    }
    if (NULL != (p = strchr(buf, '.')))
        *p = '\0';
    options->hostname = strdup(buf);

    while (EOF != ( c = getopt( ac, av, "vVh" )) ) {
        switch ( c ) {
        case 'v':	/* version */
            show_version( ); puts( "" );
            show_paths( ); puts( "" );
            exit( 0 );
            break;
        case 'V':	/* extended version */
            show_version_extended( ); puts( "" );
            show_paths( ); puts( "" );
            exit( 0 );
            break;
        case 'h':	/* usage */
            show_usage( p );
            exit( 0 );
            break;
        default :
            err++;
        }
    }
    if ( err || optind != ac ) {
        show_usage( p );
        exit( 2 );
    }

    return 1;
}
