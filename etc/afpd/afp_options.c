/* 
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 * modified from main.c. this handles afp options.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <syslog.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <atalk/paths.h>
#include "globals.h"
#include "status.h"
#include "auth.h"

#include <atalk/compat.h>

#ifdef ADMIN_GRP
#include <grp.h>
#include <sys/types.h>
#endif

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

#define OPTIONS "dn:f:s:uc:g:P:ptDS:TL:F:U:I"
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
  if (opt->defaultvol && (opt->defaultvol != save->defaultvol))
    free(opt->defaultvol);
  if (opt->systemvol && (opt->systemvol != save->systemvol))
    free(opt->systemvol);
  if (opt->loginmesg && (opt->loginmesg != save->loginmesg))
    free(opt->loginmesg);
  if (opt->guest && (opt->guest != save->guest)) 
    free(opt->guest);
  if (opt->server && (opt->server != save->server))
    free(opt->server);
  if (opt->ipaddr && (opt->ipaddr != save->ipaddr))
    free(opt->ipaddr);
  if (opt->fqdn && (opt->fqdn != save->fqdn))
    free(opt->fqdn);
  if (opt->uampath && (opt->uampath != save->uampath))
    free(opt->uampath);
  if (opt->uamlist && (opt->uamlist != save->uamlist))
    free(opt->uamlist);
  if (opt->nlspath && (opt->nlspath != save->nlspath))
    free(opt->nlspath);
  if (opt->passwdfile && (opt->passwdfile != save->passwdfile))
    free(opt->passwdfile);
}

/* initialize options */
void afp_options_init(struct afp_options *options)
{
  memset(options, 0, sizeof(struct afp_options));
  options->connections = 20;
  options->pidfile = _PATH_AFPDLOCK;
  options->defaultvol = _PATH_AFPDDEFVOL;
  options->systemvol = _PATH_AFPDSYSVOL;
  options->configfile = _PATH_AFPDCONF;
  options->nlspath = _PATH_AFPDNLSPATH;
  options->uampath = _PATH_AFPDUAMPATH;
  options->uamlist = "uams_guest.so,uams_clrtxt.so,uams_dhx.so";
  options->guest = "nobody";
  options->loginmesg = "";
  options->transports = AFPTRANS_ALL;
  options->passwdfile = _PATH_AFPDPWFILE;
  options->tickleval = 30;
#ifdef ADMIN_GRP
  options->admingid = 0;
#endif ADMIN_GRP
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

  /* figure out options w/ values. currently, this will ignore the setting
   * if memory is lacking. */
  if ((c = getoption(buf, "-defaultvol")) && (opt = strdup(c)))
    options->defaultvol = opt;
  if ((c = getoption(buf, "-systemvol")) && (opt = strdup(c)))
    options->systemvol = opt;
  if ((c = getoption(buf, "-loginmesg")) && (opt = strdup(c)))
    options->loginmesg = opt;
  if ((c = getoption(buf, "-guestname")) && (opt = strdup(c)))
    options->guest = opt;
  if ((c = getoption(buf, "-passwdfile")) && (opt = strdup(c)))
    options->passwdfile = opt;
  if ((c = getoption(buf, "-passwdminlen")))
    options->passwdminlen = MIN(1, atoi(c));
  if ((c = getoption(buf, "-loginmaxfail")))
    options->loginmaxfail = atoi(c);
  if ((c = getoption(buf, "-tickleval")))
    options->tickleval = atoi(c);

  if (c = getoption(buf, "-server_quantum"))
    options->server_quantum = strtoul(c, NULL, 0);


#ifdef ADMIN_GRP
  if ((c = getoption(buf, "-admingroup"))) {
    struct group *gr = getgrnam(c);
    if (gr != NULL) { 
      options->admingid = gr->gr_gid;
    }
  } 
#endif
  if ((c = getoption(buf, "-uampath")) && (opt = strdup(c)))
    options->uampath = opt;
  if ((c = getoption(buf, "-uamlist")) && (opt = strdup(c)))
    options->uamlist = opt;
  if ((c = getoption(buf, "-nlspath")) && (opt = strdup(c)))
    options->nlspath = opt;

  if (c = getoption(buf, "-ipaddr")) {
    struct in_addr inaddr;
    if (inet_aton(c, &inaddr) && (opt = strdup(c))) { 
      if (!gethostbyaddr((const char *) &inaddr, sizeof(inaddr), AF_INET)) 
	syslog(LOG_INFO, "WARNING: can't find %s\n", opt);
      options->ipaddr = opt;
    }
  }

  if ((c = getoption(buf, "-port")))
    options->port = atoi(c);
  if (c = getoption(buf, "-ddpaddr"))
    atalk_aton(c, &options->ddpaddr);

  /* do a little checking for the domain name. */
  if (c = getoption(buf, "-fqdn")) {
    char *p = strchr(c, ':');
    if (p)
      *p = '\0';
    if (gethostbyname(c)) {
      if (p)
	*p = ':';
      if (opt = strdup(c))
	options->fqdn = opt;
    }
  }

  return 1;
}

int afp_options_parse(int ac, char **av, struct afp_options *options)
{
  extern char *optarg;
  extern int optind;
  
  char *p;
  int c, err = 0;

  if (gethostname(options->hostname, sizeof(options->hostname )) < 0 ) {
    perror( "gethostname" );
    return 0;
  }
    if (( p = strchr(options->hostname, '.' )) != 0 ) {
	*p = '\0';
    }

    if (( p = strrchr( av[ 0 ], '/' )) == NULL ) {
	p = av[ 0 ];
    } else {
	p++;
    }

    while (( c = getopt( ac, av, OPTIONS )) != EOF ) {
	switch ( c ) {
	case 'd' :
	    options->flags |= OPTION_DEBUG;
	    break;
	case 'n' :
	    options->server = optarg;
	    break;
	case 'f' :
	    options->defaultvol = optarg;
	    break;
	case 's' :
	    options->systemvol = optarg;
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
	    options->port = atoi(optarg);
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
	case 'I':
  	    options->flags |= OPTION_CUSTOMICON;
	default :
	    err++;
	}
    }
    if ( err || optind != ac ) {
	fprintf( stderr,
		"Usage:\t%s [ -dpDTIt ] [ -n nbpname ] [ -f defvols ] \
[ -P pidfile ] [ -s sysvols ] \n", p );
	fprintf( stderr,
		"\t[ -u ] [ -c maxconn ] [ -g guest ] \
[ -S port ] [ -L loginmesg ] [ -F configfile ] [ -U uamlist ]\n" );
	return 0;
    }

#ifdef ultrix
    openlog( p, LOG_PID );
#else ultrix
    openlog( p, LOG_NDELAY|LOG_PID, LOG_LOCAL0 );
#endif ultrix

    return 1;
}
