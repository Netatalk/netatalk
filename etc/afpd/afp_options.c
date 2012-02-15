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
#include <atalk/errchk.h>

#include "status.h"
#include "auth.h"
#include "dircache.h"

#define LENGTH 512

/* get rid of any allocated afp_option buffers. */
void afp_options_free(struct afp_options *opt)
{
	if (opt->hostname)
        free(opt->hostname);
	if (opt->adminauthuser)
        free(opt->adminauthuser);
	if (opt->configfile)
        free(opt->configfile);
    if (opt->fqdn)
        free(opt->fqdn);
    if (opt->guest)
        free(opt->guest);
    if (opt->listen)
        free(opt->listen);
    if (opt->k5realm)
        free(opt->k5realm);
    if (opt->k5keytab)
        free(opt->k5keytab);
    if (opt->k5service)
        free(opt->k5service);
    if (opt->logconfig)
        free(opt->logconfig);
    if (opt->logfile)
        free(opt->logfile);
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
    EC_INIT;
    dictionary *config;
    struct afp_options *options = &AFPObj->options;
    int i, c;
    const char *p, *tmp;
    char *q, *r;
    char val[MAXVAL];

    options->configfile  = AFPObj->cmdlineconfigfile ? strdup(AFPObj->cmdlineconfigfile) : strdup(_PATH_CONFDIR "afp.conf");
    options->sigconffile = strdup(_PATH_CONFDIR "afp_signature.conf");
    options->uuidconf    = strdup(_PATH_CONFDIR "afp_voluuid.conf");
    options->flags       = OPTION_ACL2MACCESS | OPTION_UUID | OPTION_SERVERNOTIF | AFPObj->cmdlineflags;

    if ((config = iniparser_load(AFPObj->options.configfile)) == NULL)
        return -1;
    AFPObj->iniconfig = config;

    /* [Global] */
    options->logconfig = iniparser_getstrdup(config, INISEC_GLOBAL, "loglevel", "default:note");
    options->logfile   = iniparser_getstrdup(config, INISEC_GLOBAL, "logfile",  NULL);
    set_processname("afpd");
    setuplog(options->logconfig, options->logfile);

    /* [AFP] "options" options wo values */
    if (p = iniparser_getstrdup(config, INISEC_AFP, "options", NULL)) {
        if (p = strtok(q, ", ")) {
            while (p) {
                if (strcasecmp(p, "nozeroconf"))
                    options->flags |= OPTION_NOZEROCONF;
                if (strcasecmp(p, "icon"))
                    options->flags |= OPTION_CUSTOMICON;
                if (strcasecmp(p, "noicon"))
                    options->flags &= ~OPTION_CUSTOMICON;
                if (strcasecmp(p, "advertise_ssh"))
                    options->flags |= OPTION_ANNOUNCESSH;
                if (strcasecmp(p, "noacl2maccess"))
                    options->flags &= ~OPTION_ACL2MACCESS;
                if (strcasecmp(p, "keepsessions"))
                    options->flags |= OPTION_KEEPSESSIONS;
                if (strcasecmp(p, "closevol"))
                    options->flags |= OPTION_CLOSEVOL;
                if (strcasecmp(p, "client_polling"))
                    options->flags &= ~OPTION_SERVERNOTIF;
                if (strcasecmp(p, "nosavepassword"))
                    options->passwdbits |= PASSWD_NOSAVE;
                if (strcasecmp(p, "savepassword"))
                    options->passwdbits &= ~PASSWD_NOSAVE;
                if (strcasecmp(p, "nosetpassword"))
                    options->passwdbits &= ~PASSWD_SET;
                if (strcasecmp(p, "setpassword"))
                    options->passwdbits |= PASSWD_SET;
                p = strtok(NULL, ",");
            }
        }
    }
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
    options->disconnected   = iniparser_getint   (config, INISEC_AFP, "disconnect",     24) * 60 * 2;

    if ((p = iniparser_getstring(config, INISEC_AFP, "hostname", NULL))) {
        EC_NULL_LOG( options->hostname = strdup(p) );
    } else {
        if (gethostname(val, sizeof(val)) < 0 ) {
            perror( "gethostname" );
            EC_FAIL;
        }
        if ((q = strchr(val, '.')))
            *q = '\0';
        options->hostname = strdup(val);
    }

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

    q = iniparser_getstrdup(config, INISEC_AFP, "cnidserver", "localhost:4700");
    r = strrchr(q, ':');
    if (r)
        *r = 0;
    options->Cnid_srv = strdup(q);
    if (r)
        options->Cnid_port = strdup(r + 1);
    else
        options->Cnid_port = strdup("4700");
    LOG(log_debug, logtype_afpd, "CNID Server: %s:%s", options->Cnid_srv, options->Cnid_port);
    if (q)
        free(q);

    if ((q = iniparser_getstrdup(config, INISEC_AFP, "fqdn", NULL))) {
        /* do a little checking for the domain name. */
        r = strchr(q, ':');
        if (r)
            *r = '\0';
        if (gethostbyname(q)) {
            if (r)
                *r = ':';
            EC_NULL_LOG( options->fqdn = strdup(q) );
        } else {
            LOG(log_error, logtype_afpd, "error parsing -fqdn, gethostbyname failed for: %s", c);
        }
        free(q);
    }

    if (!(p = iniparser_getstring(config, INISEC_AFP, "unixcodepage", NULL))) {
        options->unixcharset = CH_UNIX;
        options->unixcodepage = strdup("LOCALE");
    } else {
        if ((options->unixcharset = add_charset(p)) == (charset_t)-1) {
            options->unixcharset = CH_UNIX;
            options->unixcodepage = strdup("LOCALE");
            LOG(log_warning, logtype_afpd, "Setting Unix codepage to '%s' failed", p);
        } else {
            options->unixcodepage = strdup(p);
        }
    }
	
    if (!(p = iniparser_getstring(config, INISEC_AFP, "maccodepage", NULL))) {
        options->maccharset = CH_MAC;
        options->maccodepage = strdup("MAC_ROMAN");
    } else {
        if ((options->maccharset = add_charset(p)) == (charset_t)-1) {
            options->maccharset = CH_MAC;
            options->maccodepage = strdup("MAC_ROMAN");
            LOG(log_warning, logtype_afpd, "Setting Unix codepage to '%s' failed", p);
        } else {
            options->maccodepage = strdup(p);
        }
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

EC_CLEANUP:
    EC_EXIT;
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
	printf( "              afp.conf:\t%s\n", _PATH_CONFDIR "afp.conf");
	printf( "    afp_signature.conf:\t%s\n", _PATH_CONFDIR "afp_signature.conf");
	printf( "      afp_voluuid.conf:\t%s\n", _PATH_CONFDIR "afp_voluuid.conf");
	printf( "       UAM search path:\t%s\n", _PATH_AFPDUAMPATH );
	printf( "  Server messages path:\t%s\n", SERVERTEXT);
	printf( "              lockfile:\t%s\n", _PATH_AFPDLOCK);
}

/*
 * Display usage information about afpd.
 */
static void show_usage(void)
{
	fprintf( stderr, "Usage:\tafpd [-d] [-F configfile]\n");
	fprintf( stderr, "\tafpd -h|-v|-V\n");
}

void afp_options_parse_cmdline(AFPObj *obj, int ac, char **av)
{
    int c, err = 0;

    while (EOF != ( c = getopt( ac, av, "dFvVh" )) ) {
        switch ( c ) {
        case 'd':
            obj->cmdlineflags |= OPTION_DEBUG;
            break;
        case 'F':
            obj->cmdlineconfigfile = strdup(optarg);
            break;
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
            show_usage();
            exit( 0 );
            break;
        default :
            err++;
        }
    }
    if ( err || optind != ac ) {
        show_usage();
        exit( 2 );
    }

    return;
}
