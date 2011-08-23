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

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif /* MIN */

/* FIXME CNID */
const char *Cnid_srv = "localhost";
const char *Cnid_port = "4700";

#define OPTIONS "dn:f:s:uc:g:P:ptDS:TL:F:U:hIvVm:"
#define LENGTH 512

/* return an option. this uses an internal array, so it's necessary
 * to duplicate it if you want to hold it for long. this is probably
 * non-optimal. */
static char *getoption(char *buf, const char *option)
{
    static char string[LENGTH + 1];
    char *end;
    int len;

    if (option && (buf = strstr(buf, option)))
        buf = strpbrk(buf, " \t");

    while (buf && isspace(*buf))
        buf++;

    if (!buf)
        return NULL;

    /* search for any quoted stuff */
    if (*buf == '"' && (end = strchr(buf + 1, '"'))) {
        buf++;
        len = MIN(end - buf, LENGTH);
    } else if ((end = strpbrk(buf, " \t\n"))) /* option or eoln */
        len = MIN(end - buf, LENGTH);
    else
        len = MIN(strlen(buf), LENGTH);

    strncpy(string, buf, len);
    string[len] = '\0';
    return string;
}

/* get rid of any allocated afp_option buffers. */
void afp_options_free(struct afp_options *opt,
                      const struct afp_options *save)
{
    if (opt->defaultvol.name && (opt->defaultvol.name != save->defaultvol.name))
        free(opt->defaultvol.name);
    if (opt->defaultvol.full_name && (opt->defaultvol.full_name != save->defaultvol.full_name))
        free(opt->defaultvol.full_name);

    if (opt->systemvol.name && (opt->systemvol.name != save->systemvol.name))
        free(opt->systemvol.name);
    if (opt->systemvol.full_name && (opt->systemvol.full_name != save->systemvol.full_name))
        free(opt->systemvol.full_name);

    if (opt->uservol.name && (opt->uservol.name != save->uservol.name))
        free(opt->uservol.name);
    if (opt->uservol.full_name && (opt->uservol.full_name != save->uservol.full_name))
        free(opt->uservol.full_name);

    if (opt->loginmesg && (opt->loginmesg != save->loginmesg))
        free(opt->loginmesg);
    if (opt->guest && (opt->guest != save->guest))
        free(opt->guest);
    if (opt->server && (opt->server != save->server))
        free(opt->server);
    if (opt->ipaddr && (opt->ipaddr != save->ipaddr))
        free(opt->ipaddr);
    if (opt->port && (opt->port != save->port))
        free(opt->port);
    if (opt->fqdn && (opt->fqdn != save->fqdn))
        free(opt->fqdn);
    if (opt->uampath && (opt->uampath != save->uampath))
        free(opt->uampath);
    if (opt->uamlist && (opt->uamlist != save->uamlist))
        free(opt->uamlist);
    if (opt->passwdfile && (opt->passwdfile != save->passwdfile))
        free(opt->passwdfile);
    if (opt->signatureopt && (opt->signatureopt != save->signatureopt))
	free(opt->signatureopt);
    if (opt->k5service && (opt->k5service != save->k5service))
	free(opt->k5service);
    if (opt->k5realm && (opt->k5realm != save->k5realm))
	free(opt->k5realm);
    if (opt->k5keytab && (opt->k5keytab != save->k5keytab))
	free(opt->k5keytab);
    if (opt->unixcodepage && (opt->unixcodepage != save->unixcodepage))
	free(opt->unixcodepage);
    if (opt->maccodepage && (opt->maccodepage != save->maccodepage))
	free(opt->maccodepage);

    if (opt->ntdomain && (opt->ntdomain != save->ntdomain))
	free(opt->ntdomain);
    if (opt->ntseparator && (opt->ntseparator != save->ntseparator))
	free(opt->ntseparator);
    if (opt->logconfig && (opt->logconfig != save->logconfig))
	free(opt->logconfig);
	if (opt->mimicmodel && (opt->mimicmodel != save->mimicmodel))
	free(opt->mimicmodel);
}

/* initialize options */
void afp_options_init(struct afp_options *options)
{
    memset(options, 0, sizeof(struct afp_options));
    options->connections = 20;
    options->pidfile = _PATH_AFPDLOCK;
    options->defaultvol.name = _PATH_AFPDDEFVOL;
    options->systemvol.name = _PATH_AFPDSYSVOL;
    options->configfile = _PATH_AFPDCONF;
    options->sigconffile = _PATH_AFPDSIGCONF;
    options->uuidconf = _PATH_AFPDUUIDCONF;
    options->uampath = _PATH_AFPDUAMPATH;
    options->uamlist = "uams_dhx.so,uams_dhx2.so";
    options->guest = "nobody";
    options->loginmesg = "";
    options->transports = AFPTRANS_TCP; /*  TCP only */
    options->passwdfile = _PATH_AFPDPWFILE;
    options->tickleval = 30;
    options->timeout = 4;       /* 4 tickles = 2 minutes */
    options->sleep = 10 * 60 * 2; /* 10 h in 30 seconds tick */
    options->disconnected = 10 * 60 * 2; /* 10 h in 30 seconds tick */
    options->server_notif = 1;
    options->authprintdir = NULL;
    options->signatureopt = "auto";
    options->umask = 0;
#ifdef ADMIN_GRP
    options->admingid = 0;
#endif /* ADMIN_GRP */
    options->k5service = NULL;
    options->k5realm = NULL;
    options->k5keytab = NULL;
    options->unixcharset = CH_UNIX;
    options->unixcodepage = "LOCALE";
    options->maccharset = CH_MAC;
    options->maccodepage = "MAC_ROMAN";
    options->volnamelen = 80; /* spec: 255, 10.1: 73, 10.4/10.5: 80 */
    options->ntdomain = NULL;
    options->ntseparator = NULL;
#ifdef USE_SRVLOC
    /* don't advertize slp by default */
    options->flags |= OPTION_NOSLP;
#endif
    options->dircachesize = DEFAULT_MAX_DIRCACHE_SIZE;
    options->flags |= OPTION_ACL2MACCESS;
    options->flags |= OPTION_UUID;
    options->tcp_sndbuf = 0;    /* 0 means don't change OS default */
    options->tcp_rcvbuf = 0;    /* 0 means don't change OS default */
    options->dsireadbuf = 12;
	options->mimicmodel = NULL;
    options->fce_fmodwait = 60; /* put fmod events 60 seconds on hold */
}

/* parse an afpd.conf line. i'm doing it this way because it's
 * easy. it is, however, massively hokey. sample afpd.conf:
 * server:AFPServer@zone -loginmesg "blah blah blah" -nodsi 
 * "private machine"@zone2 -noguest -port 11012
 * server2 -nocleartxt -nodsi
 *
 * NOTE: this ignores unknown options 
 */
int afp_options_parseline(char *buf, struct afp_options *options)
{
    char *c, *opt;

    /* handle server */
    if (*buf != '-' && (c = getoption(buf, NULL)) && (opt = strdup(c)))
        options->server = opt;

    /* parse toggles */
    if (strstr(buf, " -nodebug"))
        options->flags &= ~OPTION_DEBUG;
#ifdef USE_SRVLOC
    if (strstr(buf, " -slp"))
        options->flags &= ~OPTION_NOSLP;
#endif
#ifdef USE_ZEROCONF
    if (strstr(buf, " -nozeroconf"))
        options->flags |= OPTION_NOZEROCONF;
#endif
    if (strstr(buf, " -nouservolfirst"))
        options->flags &= ~OPTION_USERVOLFIRST;
    if (strstr(buf, " -uservolfirst"))
        options->flags |= OPTION_USERVOLFIRST;
    if (strstr(buf, " -nouservol"))
        options->flags |= OPTION_NOUSERVOL;
    if (strstr(buf, " -uservol"))
        options->flags &= ~OPTION_NOUSERVOL;
    if (strstr(buf, " -proxy"))
        options->flags |= OPTION_PROXY;
    if (strstr(buf, " -noicon"))
        options->flags &= ~OPTION_CUSTOMICON;
    if (strstr(buf, " -icon"))
        options->flags |= OPTION_CUSTOMICON;
    if (strstr(buf, " -advertise_ssh"))
        options->flags |= OPTION_ANNOUNCESSH;
    if (strstr(buf, " -noacl2maccess"))
        options->flags &= ~OPTION_ACL2MACCESS;
    if (strstr(buf, " -keepsessions")) {
        default_options.flags |= OPTION_KEEPSESSIONS;
        options->flags |= OPTION_KEEPSESSIONS;
    }

    /* passwd bits */
    if (strstr(buf, " -nosavepassword"))
        options->passwdbits |= PASSWD_NOSAVE;
    if (strstr(buf, " -savepassword"))
        options->passwdbits &= ~PASSWD_NOSAVE;
    if (strstr(buf, " -nosetpassword"))
        options->passwdbits &= ~PASSWD_SET;
    if (strstr(buf, " -setpassword"))
        options->passwdbits |= PASSWD_SET;

    /* transports */
    if (strstr(buf, " -transall"))
        options->transports = AFPTRANS_ALL;
    if (strstr(buf, " -notransall"))
        options->transports = AFPTRANS_NONE;
    if (strstr(buf, " -tcp"))
        options->transports |= AFPTRANS_TCP;
    if (strstr(buf, " -notcp"))
        options->transports &= ~AFPTRANS_TCP;
    if (strstr(buf, " -ddp"))
        options->transports |= AFPTRANS_DDP;
    if (strstr(buf, " -noddp"))
        options->transports &= ~AFPTRANS_DDP;
    if (strstr(buf, "-client_polling"))
        options->server_notif = 0;

    /* figure out options w/ values. currently, this will ignore the setting
     * if memory is lacking. */

    if ((c = getoption(buf, "-hostname"))) {
        int len = strlen (c);
        if (len <= MAXHOSTNAMELEN) {
            memcpy(options->hostname, c, len);
            options->hostname[len] = 0;
        }
        else
            LOG(log_info, logtype_afpd, "WARNING: hostname %s is too long (%d)",c,len);
    }

    if ((c = getoption(buf, "-defaultvol")) && (opt = strdup(c)))
        options->defaultvol.name = opt;
    if ((c = getoption(buf, "-systemvol")) && (opt = strdup(c)))
        options->systemvol.name = opt;
    if ((c = getoption(buf, "-loginmesg")) && (opt = strdup(c))) {
        int i = 0, j = 0;
        while (c[i]) {
            if (c[i] != '\\') {
                opt[j++] = c[i];
            } else {
                i++;
                if (c[i] == 'n')
                    opt[j++] = '\n';
            }
            i++;
        }
        opt[j] = 0;
        options->loginmesg = opt;
        
    }
    if ((c = getoption(buf, "-guestname")) && (opt = strdup(c)))
        options->guest = opt;
    if ((c = getoption(buf, "-passwdfile")) && (opt = strdup(c)))
        options->passwdfile = opt;
    if ((c = getoption(buf, "-passwdminlen")))
        options->passwdminlen = MIN(1, atoi(c));
    if ((c = getoption(buf, "-loginmaxfail")))
        options->loginmaxfail = atoi(c);
    if ((c = getoption(buf, "-tickleval"))) {
        options->tickleval = atoi(c);
        if (options->tickleval <= 0) {
            options->tickleval = 30;
        }
    }
    if ((c = getoption(buf, "-timeout"))) {
        options->timeout = atoi(c);
        if (options->timeout <= 0) {
            options->timeout = 4;
        }
    }

    if ((c = getoption(buf, "-sleep"))) {
        options->disconnected = options->sleep = atoi(c) * 120;
        if (options->sleep <= 4) {
            options->disconnected = options->sleep = 4;
        }
    }

    if ((c = getoption(buf, "-dsireadbuf"))) {
        options->dsireadbuf = atoi(c);
        if (options->dsireadbuf < 6)
            options->dsireadbuf = 6;
    }

    if ((c = getoption(buf, "-server_quantum")))
        options->server_quantum = strtoul(c, NULL, 0);

    if ((c = getoption(buf, "-volnamelen"))) {
        options->volnamelen = atoi(c);
        if (options->volnamelen < 8) {
            options->volnamelen = 8; /* max mangled volname "???#FFFF" */
        }
        if (options->volnamelen > 255) {
	    options->volnamelen = 255; /* AFP3 spec */
        }
    }

    /* -[no]setuplog <logtype> <loglevel> [<filename>]*/
    c = buf;
    /* Now THIS is hokey! Multiple occurrences are not supported by our current code, */
    /* so I have to loop myself. */
    while (NULL != (c = strstr(c, "-setuplog"))) {
        char *optstr;
        if ((optstr = getoption(c, "-setuplog"))) {
            /* hokey2: options->logconfig must be converted to store an array of logstrings */
            if (options->logconfig)
                free(options->logconfig);
            options->logconfig = strdup(optstr);
            setuplog(optstr);
            c += sizeof("-setuplog");
        }
    }

    if ((c = getoption(buf, "-unsetuplog")))
      unsetuplog(c);

#ifdef ADMIN_GRP
    if ((c = getoption(buf, "-admingroup"))) {
        struct group *gr = getgrnam(c);
        if (gr != NULL) {
            options->admingid = gr->gr_gid;
        }
    }
#endif /* ADMIN_GRP */

    if ((c = getoption(buf, "-k5service")) && (opt = strdup(c)))
	options->k5service = opt;
    if ((c = getoption(buf, "-k5realm")) && (opt = strdup(c)))
	options->k5realm = opt;
    if ((c = getoption(buf, "-k5keytab"))) {
	if ( NULL == (options->k5keytab = (char *) malloc(sizeof(char)*(strlen(c)+14)) )) {
		LOG(log_error, logtype_afpd, "malloc failed");
		exit(-1);
	}
	snprintf(options->k5keytab, strlen(c)+14, "KRB5_KTNAME=%s", c);
	putenv(options->k5keytab);
	/* setenv( "KRB5_KTNAME", c, 1 ); */
    }
    if ((c = getoption(buf, "-authprintdir")) && (opt = strdup(c)))
        options->authprintdir = opt;
    if ((c = getoption(buf, "-uampath")) && (opt = strdup(c)))
        options->uampath = opt;
    if ((c = getoption(buf, "-uamlist")) && (opt = strdup(c)))
        options->uamlist = opt;

    if ((c = getoption(buf, "-ipaddr"))) {
#if 0
        struct in_addr inaddr;
        if (inet_aton(c, &inaddr) && (opt = strdup(c))) {
            if (!gethostbyaddr((const char *) &inaddr, sizeof(inaddr), AF_INET))
                LOG(log_info, logtype_afpd, "WARNING: can't find %s", opt);
            options->ipaddr = opt;
        }
        else {
            LOG(log_error, logtype_afpd, "Error parsing -ipaddr, is %s in numbers-and-dots notation?", c);
        }
#endif
        options->ipaddr = strdup(c);
    }

    /* FIXME CNID Cnid_srv is a server attribute */
    if ((c = getoption(buf, "-cnidserver"))) {
        char *p = strrchr(c, ':');
        if (p)
            *p = 0;
        Cnid_srv = strdup(c);
        if (p)
            Cnid_port = strdup(p + 1);
        LOG(log_debug, logtype_afpd, "CNID Server: %s:%s", Cnid_srv, Cnid_port);
    }

    if ((c = getoption(buf, "-port")))
        options->port = strdup(c);
    if ((c = getoption(buf, "-ddpaddr")))
        atalk_aton(c, &options->ddpaddr);
    if ((c = getoption(buf, "-signature")) && (opt = strdup(c)))
        options->signatureopt = opt;

    /* do a little checking for the domain name. */
    if ((c = getoption(buf, "-fqdn"))) {
        char *p = strchr(c, ':');
        if (p)
            *p = '\0';
        if (gethostbyname(c)) {
            if (p)
                *p = ':';
            if ((opt = strdup(c)))
                options->fqdn = opt;
        }
	else {
            LOG(log_error, logtype_afpd, "error parsing -fqdn, gethostbyname failed for: %s", c);
	}
    }

    if ((c = getoption(buf, "-unixcodepage"))) {
    	if ((charset_t)-1  == ( options->unixcharset = add_charset(c)) ) {
            options->unixcharset = CH_UNIX;
            LOG(log_warning, logtype_afpd, "setting Unix codepage to '%s' failed", c);
    	}
	else {
            if ((opt = strdup(c)))
                options->unixcodepage = opt;
	}
    }
	
    if ((c = getoption(buf, "-maccodepage"))) {
    	if ((charset_t)-1 == ( options->maccharset = add_charset(c)) ) {
            options->maccharset = CH_MAC;
            LOG(log_warning, logtype_afpd, "setting Mac codepage to '%s' failed", c);
    	}
	else {
            if ((opt = strdup(c)))
                options->maccodepage = opt;
	}
    }
    
    if ((c = strstr(buf, "-closevol"))) {
        options->closevol= 1;
    }

    if ((c = getoption(buf, "-ntdomain")) && (opt = strdup(c)))
       options->ntdomain = opt;

    if ((c = getoption(buf, "-ntseparator")) && (opt = strdup(c)))
       options->ntseparator = opt;

    if ((c = getoption(buf, "-dircachesize")))
        options->dircachesize = atoi(c);
     
    if ((c = getoption(buf, "-tcpsndbuf")))
        options->tcp_sndbuf = atoi(c);

    if ((c = getoption(buf, "-tcprcvbuf")))
        options->tcp_rcvbuf = atoi(c);

	if ((c = getoption(buf, "-fcelistener"))) {
		LOG(log_note, logtype_afpd, "Adding fce listener \"%s\"", c);
		fce_add_udp_socket(c);
	}
	if ((c = getoption(buf, "-fcecoalesce"))) {
		LOG(log_note, logtype_afpd, "Fce coalesce: %s", c);
		fce_set_coalesce(c);
	}
	if ((c = getoption(buf, "-fceevents"))) {
		LOG(log_note, logtype_afpd, "Fce events: %s", c);
		fce_set_events(c);
	}

    if ((c = getoption(buf, "-fceholdfmod")))
        options->fce_fmodwait = atoi(c);

    if ((c = getoption(buf, "-mimicmodel")) && (opt = strdup(c)))
       options->mimicmodel = opt;

    return 1;
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

	printf( "DDP(AppleTalk) Support:\t" );
#ifdef NO_DDP
	puts( "No" );
#else
	puts( "Yes" );
#endif

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

	printf( "           SLP support:\t" );
#ifdef USE_SRVLOC
	puts( "Yes" );
#else
	puts( "No" );
#endif

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

	printf( "        Dropbox kludge:\t" );
#ifdef DROPKLUDGE
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "  Force volume uid/gid:\t" );
#ifdef FORCE_UIDGID
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "           ACL support:\t" );
#ifdef HAVE_ACLS
	puts( "Yes" );
#else
	puts( "No" );
#endif

	printf( "            EA support:\t" );
	puts( EA_MODULES );

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
	fprintf( stderr, "Usage:\t%s [-duptDTI] [-f defaultvolumes] [-s systemvolumes] [-n nbpname]\n", name );
	fprintf( stderr, "\t     [-c maxconnections] [-g guest] [-P pidfile] [-S port] [-L message]\n" );
	fprintf( stderr, "\t     [-F configfile] [-U uams] [-m umask]\n" );
	fprintf( stderr, "\t%s -h|-v|-V\n", name );
}

int afp_options_parse(int ac, char **av, struct afp_options *options)
{
    extern char *optarg;
    extern int optind;

    char *p;
    char *tmp;	/* Used for error checking the result of strtol */
    int c, err = 0;

    if (gethostname(options->hostname, sizeof(options->hostname )) < 0 ) {
        perror( "gethostname" );
        return 0;
    }
    if (NULL != ( p = strchr(options->hostname, '.' )) ) {
        *p = '\0';
    }

#ifdef ultrix
    if (NULL == ( p = strrchr( av[ 0 ], '/' )) ) {
        p = av[ 0 ];
    } else {
        p++;
    }
    openlog( p, LOG_PID ); /* ultrix only */
#endif /* ultrix */

    while (EOF != ( c = getopt( ac, av, OPTIONS )) ) {
        switch ( c ) {
        case 'd' :
            options->flags |= OPTION_DEBUG;
            break;
        case 'n' :
            options->server = optarg;
            break;
        case 'f' :
            options->defaultvol.name = optarg;
            break;
        case 's' :
            options->systemvol.name = optarg;
            break;
        case 'u' :
            options->flags |= OPTION_USERVOLFIRST;
            break;
        case 'c' :
            options->connections = atoi( optarg );
            break;
        case 'g' :
            options->guest = optarg;
            break;

        case 'P' :
            options->pidfile = optarg;
            break;

        case 'p':
            options->passwdbits |= PASSWD_NOSAVE;
            break;
        case 't':
            options->passwdbits |= PASSWD_SET;
            break;

        case 'D':
            options->transports &= ~AFPTRANS_DDP;
            break;
        case 'S':
            options->port = optarg;
            break;
        case 'T':
            options->transports &= ~AFPTRANS_TCP;
            break;
        case 'L':
            options->loginmesg = optarg;
            break;
        case 'F':
            options->configfile = optarg;
            break;
        case 'U':
            options->uamlist = optarg;
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
            show_usage( p );
            exit( 0 );
            break;
        case 'I':
            options->flags |= OPTION_CUSTOMICON;
            break;
        case 'm':
            options->umask = strtoul(optarg, &tmp, 8);
            if ((options->umask > 0777)) {
                fprintf(stderr, "%s: out of range umask setting provided\n", p);
                err++;
            }
            if (tmp[0] != '\0') {
                fprintf(stderr, "%s: invalid characters in umask setting provided\n", p);
                err++;
            }
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
