/*
 * $Id: config.c,v 1.20 2009-10-29 11:35:58 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <atalk/logger.h>
#include <sys/param.h>
#ifdef TRU64
#include <sys/mbuf.h>
#include <net/route.h>
#endif /* TRU64 */
#include <net/if.h>
#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/paths.h>
#include <atalk/util.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef __svr4__
#include <sys/sockio.h>
#include <sys/stropts.h>
#endif /* __svr4__ */

#include <atalk/unicode.h>
#include "interface.h"
#include "multicast.h"
#include "rtmp.h"
#include "zip.h"
#include "list.h"
#include "main.h"

#ifndef IFF_SLAVE /* a little backward compatibility */
#define IFF_SLAVE 0
#endif /* IFF_SLAVE */

int router(struct interface *iface, char **av);
int dontroute(struct interface *iface, char **av);
int seed(struct interface *iface, char **av);
int phase(struct interface *iface, char **av); 
int net(struct interface *iface, char **av);
int addr(struct interface *iface, char **av); 
int zone(struct interface *iface, char **av);
int noallmulti(struct interface *iface, char **av);

static const struct param {
    char	*p_name;
    int		(*p_func)(struct interface *iface, char **av);
} params[] = {
    { "router", router },
    { "dontroute", dontroute },
    { "seed",	seed },
    { "phase",	phase },
    { "net",	net },
    { "addr",	addr },
    { "zone",	zone },
    { "noallmulti", noallmulti }
};

#define ARGV_CHUNK_SIZE 128
#define MAXLINELEN 2048
static char **parseline(const char *line)
{
    const char	 *p;
    int		  argc = 0;
    char	 *buffer, *tmpbuf;
    char	**argv;

    /* Ignore empty lines and lines with leading hash marks. */
    p = line;
    while ( isspace( *p ) ) {
	p++;
    }
    if ( *p == '#' || *p == '\0' ) {
	return NULL;
    }

    buffer = (char *) malloc( strlen( p ) + 1 );
    if ( !buffer ) {
	/* FIXME: error handling */
	return NULL;
    }
    strcpy( buffer, p );
    tmpbuf = buffer;

    argv = (char **) malloc( ARGV_CHUNK_SIZE * sizeof( char * ) );
    if ( !argv ) {
	/* FIXME: error handling */
	free( buffer );
	return NULL;
    }

    /*
     * This parser should be made more powerful -- it should
     * handle various escapes, e.g. \" and \031.
     */
    do {
	if ( *tmpbuf == '"' ) {
	    argv[ argc++ ] = ++tmpbuf;
	    while ( *tmpbuf != '\0' && *tmpbuf != '"' ) {
		tmpbuf++;
	    }
	    if ( *tmpbuf == '"' ) {
		/* FIXME: error handling */
	    }
	} else {
	    argv[ argc++ ] = tmpbuf;
	    while ( *tmpbuf != '\0' && !isspace( *tmpbuf )) {
		tmpbuf++;
	    }
	}
	*tmpbuf++ = '\0';

	/* Make room for a NULL pointer and our special pointer (s.b.) */
	if ( (argc + 1) % ARGV_CHUNK_SIZE == 0 ) {
	    char **tmp;
	    tmp = (char **) realloc( argv, argc + 1 + ARGV_CHUNK_SIZE * sizeof( char * ) );
	    if ( !tmp ) {
		/* FIXME: error handling */
		free( argv );
		free( buffer );
		return NULL;
	    }
	    argv = tmp;
	}

	/* Skip white spaces. */
        while ( isspace( *tmpbuf ) ) {
            tmpbuf++;
        }
    } while ( *tmpbuf != '\0' );

    argv[ argc++ ] = NULL;
    /* We store our buffer pointer in argv, too, so we can free it later.
     * (But don't tell anyone.)
     */
    argv[ argc ] = buffer;

    return argv;
}

static void freeline( char **argv )
{
    char **tmp = argv;

    if ( argv ) {
	while ( *tmp ) {
	    tmp++;
	}
	free( *++tmp );
	free( argv );
    }
}

int writeconf(char *cf)
{
    struct stat		st;
    char		*path, *p, newpath[ MAXPATHLEN ], line[ MAXLINELEN ];
    char		**argv;
    FILE		*conf, *newconf;
    struct interface	*iface;
    struct list		*l;
    int			mode = 0644, fd;
    size_t		len;
    char		*zonename;

    if ( cf == NULL ) {
	path = _PATH_ATALKDCONF;
    } else {
	path = cf;
    }

    /* check if old conf is writable */
    if ( stat( path, &st ) == 0 ) {
	if (( st.st_mode & S_IWUSR ) == 0 ) {
	    LOG(log_info, logtype_atalkd, "%s not writable, won't rewrite", path );
	    return( -1 );
	}
	 mode = st.st_mode;
    }

    if (( p = strrchr( path, '/' )) == NULL ) {
	strcpy( newpath, _PATH_ATALKDTMP );
    } else {
	sprintf( newpath, "%.*s/%s", (int)(p - path), path, _PATH_ATALKDTMP );
    }
    if (( fd = open( newpath, O_WRONLY|O_CREAT|O_TRUNC, mode )) < 0 ) {
	LOG(log_error, logtype_atalkd, "%s: %s", newpath, strerror(errno) );
	return( -1 );
    }
    if (( newconf = fdopen( fd, "w" )) == NULL ) {
	LOG(log_error, logtype_atalkd, "fdreopen %s: %s", newpath, strerror(errno) );
	return( -1 );
    }

    if (( conf = fopen( path, "r" )) == NULL && cf ) {
	LOG(log_error, logtype_atalkd, "%s: %s", path, strerror(errno) );
	return( -1 );
    }

    iface = interfaces->i_next;

    while ( conf == NULL || fgets( line, sizeof( line ), conf ) != NULL ) {
	if ( conf != NULL && ( argv = parseline( line )) == NULL ) {
	    if ( fputs( line, newconf ) == EOF ) {
		LOG(log_error, logtype_atalkd, "fputs: %s", strerror(errno) );
		return( -1 );
	    }
	    freeline( argv );
	    continue;
	}

	/* write real lines */
	if ( iface ) {
	    fprintf( newconf, "%s", iface->i_name );
	    if ( iface->i_flags & IFACE_RSEED ) {
		fprintf( newconf, " -router" );
	    } else if ( iface->i_flags & IFACE_SEED ) {
		fprintf( newconf, " -seed" );
	    }
	    if ( iface->i_flags & IFACE_DONTROUTE) {
	        fprintf( newconf, " -dontroute");
	    }
#ifdef linux
            if ( !(iface->i_flags & IFACE_ALLMULTI)) {
	        fprintf( newconf, " -noallmulti");
            }
#endif

	    fprintf( newconf, " -phase %d",
		    ( iface->i_flags & IFACE_PHASE1 ) ? 1 : 2 );
	    fprintf( newconf, " -net %d", ntohs( iface->i_rt->rt_firstnet ));
	    if ( iface->i_rt->rt_lastnet != iface->i_rt->rt_firstnet ) {
		fprintf( newconf, "-%d", ntohs( iface->i_rt->rt_lastnet ));
	    }
	    fprintf( newconf, " -addr %u.%u",
		    ntohs( iface->i_addr.sat_addr.s_net ),
		    iface->i_addr.sat_addr.s_node );
	    for ( l = iface->i_rt->rt_zt; l; l = l->l_next ) {
                /* codepage conversion */
                if ((size_t)(-1) == (len = convert_string_allocate(CH_MAC, CH_UNIX, 
                                      ((struct ziptab *)l->l_data)->zt_name,
                                      ((struct ziptab *)l->l_data)->zt_len,
                                      &zonename)) ) {
                    if ( NULL == 
                      (zonename = strdup(((struct ziptab *)l->l_data)->zt_name))) {
		        LOG(log_error, logtype_atalkd, "malloc: %s",  strerror(errno) );
		        return( -1 );
                    }
                    len = ((struct ziptab *)l->l_data)->zt_len;
                } 
		fprintf( newconf, " -zone \"%.*s\"", (int)len, zonename);
                free(zonename);
	    }
	    fprintf( newconf, "\n" );

	    iface = iface->i_next;
	    if ( conf == NULL && iface == NULL ) {
		break;
	    }
	}
    }
    if ( conf != NULL ) {
	fclose( conf );
    }
    fclose( newconf );

    if ( rename( newpath, path ) < 0 ) {
	LOG(log_error, logtype_atalkd, "rename %s to %s: %s", newpath, path, strerror(errno) );
	return( -1 );
    }
    return( 0 );
}

/*
 * Read our config file. If it's not there, return -1. If it is there,
 * but has syntax errors, exit. Format of the file is as follows:
 *
 *	interface [ -seed ] [ -phase number ] [ -net net-range ]
 *	[ -addr net.node ] [ -zone zonename ]...
 * e.g.
 *	le0 -phase 1 -net 7938 -zone Argus
 * or
 *	le0 -phase 2 -net 8043-8044 -zone Argus -zone "Research Systems"
 *	le0 -phase 1 -net 7938 -zone Argus
 *
 * Pretty much everything is optional. Anything that is unspecified is
 * searched for on the network.  If -seed is not specified, the
 * configuration is assumed to be soft, i.e. it can be overridden by
 * another router. If -seed is specified, atalkd will exit if another
 * router disagrees.  If the phase is unspecified, it defaults to phase
 * 2 (the default can be overridden on the command line).  -addr can
 * replace -net, if the network in question isn't a range.  The default
 * zone for an interface is the first zone encountered for that
 * interface.
 */
int readconf(char *cf)
{
    struct ifreq	ifr;
    struct interface	*iface, *niface;
    char		line[ MAXLINELEN ], **argv, *p;
    unsigned int	i, j;
    int			s, cc = 0;
    FILE		*conf;

    if ( cf == NULL ) {
	p = _PATH_ATALKDCONF;
    } else {
	p = cf;
    }
    if (( conf = fopen( p, "r" )) == NULL ) {
        return( -1 );
    }

#ifndef __svr4__
    if (( s = socket( AF_APPLETALK, SOCK_DGRAM, 0 )) < 0 ) {
	perror( "socket" );
	fclose(conf);
	return -1;
    }
#endif /* __svr4__ */

    while ( fgets( line, sizeof( line ), conf ) != NULL ) {
	if (( argv = parseline( line )) == NULL ) {
	    continue;
	}

#ifndef __svr4__
	/*
	 * Check that av[ 0 ] is a valid interface.
	 * Not possible under sysV.
	 */
	strlcpy( ifr.ifr_name, argv[ 0 ], sizeof(ifr.ifr_name) );

	/* for devices that don't support appletalk */
	if ((ioctl(s, SIOCGIFADDR, &ifr) < 0) && (errno == ENODEV)) {
	  perror(argv[0]);
	  goto read_conf_err;
	}

	if ( ioctl( s, SIOCGIFFLAGS, &ifr ) < 0 ) {
	    perror( argv[ 0 ] );
	    goto read_conf_err;
	}

	if (ifr.ifr_flags & (IFF_LOOPBACK | IFF_POINTOPOINT |IFF_SLAVE)) {
	    fprintf( stderr, "%s: can't configure.\n", ifr.ifr_name );
	    goto read_conf_err;
	}

#ifdef IFF_MULTICAST
	if ((ifr.ifr_flags & IFF_MULTICAST) == 0)
	    fprintf(stderr, "%s: multicast may not work properly.\n",
		    ifr.ifr_name);
#endif /* IFF_MULTICAST */

	/* configure hw multicast for this interface. */
	if (addmulti(ifr.ifr_name, NULL) < 0) {
	  perror(ifr.ifr_name);
	  fprintf(stderr, "Can't configure multicast.\n");
	  goto read_conf_err;
	}

#endif /* __svr4__ */

	if (( niface = newiface( argv[ 0 ] )) == NULL ) {
	    perror( "newiface" );
	    goto read_conf_err;
	}

	for ( i = 1; argv[ i ]; i += cc ) {
	    if ( argv[ i ][ 0 ] == '-' ) {
		argv[ i ]++;
	    }
	    for ( j = 0; j < sizeof( params ) / sizeof( params[ 0 ] ); j++ ) {
		if ( strcmp( argv[ i ], params[ j ].p_name ) == 0 ) {
		    if ( params[ j ].p_func != NULL ) {
			cc = (*params[ j ].p_func)( niface, &argv[ i + 1 ] );
			if (cc < 0) 
			  goto read_conf_err;
			break;
		    }
		}
	    }
	    if ( j >= sizeof( params ) / sizeof( params[ 0 ] )) {
		fprintf( stderr, "%s: attribute not found.\n", argv[ i ] );
		goto read_conf_err;
	    }
	}

	for ( iface = interfaces; iface; iface = iface->i_next ) {
	    if ( strcmp( niface->i_name, iface->i_name ) == 0 &&
		    ((( niface->i_flags & iface->i_flags &
		    ( IFACE_PHASE1|IFACE_PHASE2 )) != 0 ) ||
		    niface->i_flags == 0 || iface->i_flags == 0 )) {
		break;
	    }
	}
	if ( iface ) {	/* Already have this interface and phase */
	    fprintf( stderr, "%s already configured!\n", niface->i_name );
	    goto read_conf_err;
	}

#ifdef	linux
	/* Don't set interface to allmulti if it already is, or -noallmulti was given */
	if ((ifr.ifr_flags & IFF_ALLMULTI))
		niface->i_flags |= IFACE_WASALLMULTI; 

	if ((niface->i_flags & IFACE_ALLMULTI) && !(niface->i_flags & IFACE_WASALLMULTI))
		ifsetallmulti(ifr.ifr_name, 1);
#endif

	if ( interfaces == NULL ) {
	    interfaces = niface;
	} else {
	    for ( iface = interfaces; iface->i_next; iface = iface->i_next )
		;
	    iface->i_next = niface;
	}
	niface->i_next = NULL;
    }

#ifndef __svr4__
    close( s );
#endif /* __svr4__ */

    fclose( conf );

    /*
     * Note: we've added lo0 to the interface list previously, so we must
     * have configured more than one interface...
     */
    for ( iface = interfaces, cc = 0; iface; iface = iface->i_next, cc++ )
	;
    if ( cc >= IFBASE ) {
	return( 0 );
    } else {
	return( -1 );
    }

read_conf_err:
#ifndef __svr4__
    close(s);
#endif /* __svr4__ */
    fclose(conf);
    return -1;
}

int noallmulti( struct interface *iface, char **av _U_)
{
    /* Linux specific, no effect on other platforms */
    iface->i_flags &= !IFACE_ALLMULTI;

    return (1);
}
	
/*ARGSUSED*/
int router(struct interface *iface, char **av _U_)
{
    /* make sure "-router" and "-dontroute" aren't both on the same line. */
    if (iface->i_flags & IFACE_DONTROUTE) {
	fprintf( stderr, "Can't specify both -router and -dontroute.\n");
	return -1;
    }

    /*
     * Check to be sure "-router" is before "-zone".
     */
    if ( iface->i_czt ) {
	fprintf( stderr, "Must specify -router before -zone.\n");
	return -1;
    }

    /* -router also implies -seed */
    iface->i_flags |= IFACE_RSEED | IFACE_SEED | IFACE_ISROUTER;
    return( 1 );
}

/*ARGSUSED*/
int dontroute(struct interface *iface, char **av _U_)
{
    /* make sure "-router" and "-dontroute" aren't both on the same line. */
    if (iface->i_flags & IFACE_RSEED) {
	fprintf( stderr, "Can't specify both -router and -dontroute.\n");
	return -1;
    }

    iface->i_flags |= IFACE_DONTROUTE;
    return( 1 );
}

/*ARGSUSED*/
int seed( struct interface *iface, char **av _U_)
{
    /*
     * Check to be sure "-seed" is before "-zone". we keep the old
     * semantics of just ignoring this in a routerless world.
     */
    if ( iface->i_czt ) {
	fprintf( stderr, "Must specify -seed before -zone(%s).\n",
		 iface->i_czt->zt_name);
	return -1;
    }

    iface->i_flags |= IFACE_SEED;
    return( 1 );
}

int phase(struct interface *iface, char **av)
{
    int			n;
    char		*pnum;

    if (( pnum = av[ 0 ] ) == NULL ) {
	fprintf( stderr, "No phase.\n" );
	return -1;
    }

    switch ( n = atoi( pnum )) {
    case 1 :
	iface->i_flags |= IFACE_PHASE1;
	break;

    case 2 :
	iface->i_flags |= IFACE_PHASE2;
	break;

    default :
	fprintf( stderr, "No phase %d.\n", n );
	return -1;
    }
    return( 2 );
}

int net(struct interface *iface, char **av)
{
    char		*nrange;
    char		*stop;
    int			net;

    if (( nrange = av[ 0 ] ) == NULL ) {
	fprintf( stderr, "No network.\n" );
	return -1;
    }

    if (( stop = strchr( nrange, '-' )) != NULL ) {
	stop++;
    }
    net = atoi( nrange );
    if ( net < 0 || net >= 0xffff ) {
	fprintf( stderr, "Bad network: %d\n", net );
	return -1;
    }

    if ( iface->i_rt == NULL && ( iface->i_rt = newrt(iface)) == NULL ) {
	perror( "newrt" );
	return -1;
    }

    if ( iface->i_flags & IFACE_PHASE1 ) {
	if ( stop != NULL ) {
	    fprintf( stderr, "Phase 1 doesn't use an address range.\n" );
	    return -1;
	}
	if ( iface->i_caddr.sat_addr.s_net != ATADDR_ANYNET &&
		ntohs( iface->i_caddr.sat_addr.s_net ) != net ) {
	    fprintf( stderr, "Net-range (%u) doesn't match net %u.\n",
		    net, ntohs( iface->i_caddr.sat_addr.s_net ));
	    return -1;
	}
	iface->i_rt->rt_firstnet = iface->i_rt->rt_lastnet = htons( net );
    } else if ( iface->i_flags & IFACE_PHASE2 ) {
	iface->i_rt->rt_firstnet = htons( net );
	if ( stop != NULL ) {
	    net = atoi( stop );
	    if ( net < 0 || net >= 0xffff ) {
		fprintf( stderr, "Bad network: %d\n", net );
		return -1;
	    }
	}
	iface->i_rt->rt_lastnet = htons( net );
	if ( iface->i_caddr.sat_addr.s_net != ATADDR_ANYNET &&
		( ntohs( iface->i_rt->rt_firstnet ) >
		ntohs( iface->i_caddr.sat_addr.s_net ) ||
		ntohs( iface->i_rt->rt_lastnet ) <
		ntohs( iface->i_caddr.sat_addr.s_net ))) {
	    fprintf( stderr, "Net-range (%u-%u) doesn't contain net (%u).\n",
		    ntohs( iface->i_rt->rt_firstnet ),
		    ntohs( iface->i_rt->rt_lastnet ),
		    ntohs( iface->i_caddr.sat_addr.s_net ));
	    return -1;
	}
	if ( iface->i_rt->rt_firstnet != iface->i_rt->rt_lastnet ) {
	    iface->i_rt->rt_flags |= RTMPTAB_EXTENDED;
	}
    } else {
	fprintf( stderr, "Must specify phase before networks.\n" );
	return -1;
    }
    return( 2 );
}

int addr(struct interface *iface, char **av)
{
    if ( av[ 0 ] == NULL ) {
	fprintf( stderr, "No address.\n" );
	return -1;
    }
    if ( atalk_aton( av[ 0 ], &iface->i_caddr.sat_addr ) == 0 ) {
	fprintf( stderr, "Bad address, %s\n", av[ 0 ] );
	return -1;
    }

    if ( iface->i_rt ) {
	if ( ntohs( iface->i_rt->rt_firstnet ) >
		ntohs( iface->i_caddr.sat_addr.s_net ) ||
		ntohs( iface->i_rt->rt_lastnet ) <
		ntohs( iface->i_caddr.sat_addr.s_net )) {
	    fprintf( stderr, "Net (%u) not in net-range (%u-%u).\n",
		    ntohs( iface->i_caddr.sat_addr.s_net ),
		    ntohs( iface->i_rt->rt_firstnet ),
		    ntohs( iface->i_rt->rt_lastnet ));
	    return -1;
	}
    } else {
	if (( iface->i_rt = newrt(iface)) == NULL ) {
	    perror( "newrt" );
	    return -1;
	}
	iface->i_rt->rt_firstnet = iface->i_rt->rt_lastnet =
		iface->i_caddr.sat_addr.s_net;
    }

    return( 2 );
}

int zone(struct interface *iface, char **av)
{
    struct ziptab	*zt;
    char		*zname;

    if ( av[ 0 ]  == NULL ) {
	fprintf( stderr, "No zone.\n" );
	return -1;
    }

    /* codepage conversion */
    if ((size_t)(-1) == convert_string_allocate(CH_UNIX, CH_MAC, av[0], -1, &zname)) {
	zname = strdup(av[0]);
    }

    /*
     * Only process "-zone" if this interface has "-seed".  We keep our
     * list of configured zones in the interface structure.  Then we can
     * check that the network has given us good zones.
     */
    if ( iface->i_flags & IFACE_SEED ) {
        if ( iface->i_rt == NULL ) {
  	    fprintf( stderr, "Must specify net-range before zones.\n" );
	    return -1;
        }

	if (( zt = newzt( strlen( zname ), zname )) == NULL ) {
	    perror( "newzt" );
	    return -1;
	}
	if ( iface->i_czt == NULL ) {
	    iface->i_czt = zt;
	} else {
	    zt->zt_next = iface->i_czt->zt_next;
	    iface->i_czt->zt_next = zt;
	}
    }
    free(zname);

    return( 2 );
}

/*
 * Get the configuration from the kernel. Only called if there's no
 * configuration.
 */
int getifconf(void)
{
    struct interface	*iface, *niface;
    struct ifreq        ifr;
    char                **start, **list;
    int			s;

    if (( s = socket( AF_APPLETALK, SOCK_DGRAM, 0 )) < 0 ) {
	perror( "socket" );
	return -1;
    }

    start = list = getifacelist();
    while (list && *list) {
        strlcpy(ifr.ifr_name, *list, sizeof(ifr.ifr_name));
	list++;

	if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0)
	  continue;

	if (ifr.ifr_flags & (IFF_LOOPBACK | IFF_POINTOPOINT | IFF_SLAVE))
	  continue;

	if ((ifr.ifr_flags & IFF_UP) == 0)
	  continue;

	/* for devices that don't support appletalk */
	if (ioctl(s, SIOCGIFADDR, &ifr) < 0 && (errno == ENODEV))
	  continue;

	for ( iface = interfaces; iface; iface = iface->i_next ) {
	    if ( strcmp( iface->i_name, ifr.ifr_name ) == 0 ) {
		break;
	    }
	}
	if ( iface ) {	/* Already have this interface name */
	    continue;
	}


#ifdef IFF_MULTICAST
	if ((ifr.ifr_flags & IFF_MULTICAST) == 0)
	  fprintf(stderr, "%s: multicast may not work correctly.\n",
		  ifr.ifr_name);
#endif /* IFF_MULTICAST */

	if (addmulti(ifr.ifr_name, NULL) < 0) {
	  fprintf(stderr, "%s: disabled.\n", ifr.ifr_name);
	  continue;
	}
	
	if (( niface = newiface( ifr.ifr_name )) == NULL ) {
	    perror( "newiface" );
	    close(s);
	    freeifacelist(start);
	    return -1;
	}
	/*
	 * Could try to get the address from the kernel...
	 */

	if ( interfaces == NULL ) {
	    interfaces = niface;
	} else {
	    for ( iface = interfaces; iface->i_next; iface = iface->i_next )
		;
	    iface->i_next = niface;
	}
	niface->i_next = NULL;
    }
    freeifacelist(start);
    (void)close( s );
    return( 0 );
}

/*
 * Allocate a new interface structure.  Centralized here so we can change
 * the interface structure and have it updated nicely.
 */

struct interface *newiface( const char *name)
{
    struct interface	*niface;

    if (( niface = (struct interface *)calloc(1, sizeof( struct interface )))
	    == NULL ) {
	return( NULL );
    }
    strlcpy( niface->i_name, name, sizeof(niface->i_name));
#ifdef BSD4_4
    niface->i_addr.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    niface->i_addr.sat_family = AF_APPLETALK;
#ifdef BSD4_4
    niface->i_caddr.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    niface->i_caddr.sat_family = AF_APPLETALK;
#ifdef linux
    niface->i_flags = IFACE_ALLMULTI;
#endif
    return( niface );
}

#ifdef __svr4__
int plumb(void)
{
    struct interface	*iface;
    char		device[ MAXPATHLEN + 1], *p, *t;
    int			fd, ppa;
    int			digits = 0;

    for ( iface = interfaces; iface != NULL; iface = iface->i_next ) {
	if ( strcmp( iface->i_name, LOOPIFACE ) == 0 ) {
	    continue;
	}

	strcpy( device, "/dev/" );
	strcat( device, iface->i_name );
	for (t = device; *t != '\0' ; ++t) {
	    if (isdigit(*t) == 0) {
		p = t + 1;
	    }
	    else {
		digits++;
	    }
	}

	if (digits == 0) {
	    LOG(log_error, logtype_atalkd, "plumb: invalid device: %s", device );
	    return -1;
	}
	ppa = atoi( p );
	*p = '\0';

	if (( fd = open( device, O_RDWR, 0 )) < 0 ) {
	    LOG(log_error, logtype_atalkd, "%s: %s", device, strerror(errno) );
	    return -1;
	}
	if ( ioctl( fd, I_PUSH, "ddp" ) < 0 ) {
	    LOG(log_error, logtype_atalkd, "I_PUSH: %s", strerror(errno) );
	    close(fd);
	    return -1;
	}
	if ( ioctl( fd, IF_UNITSEL, ppa ) < 0 ) {
	    LOG(log_error, logtype_atalkd, "IF_UNITSEL: %s", strerror(errno) );
	    close(fd);
	    return -1;
	}

	/* configure multicast. */
	if (addmulti(iface->i_name, NULL) < 0) {
	  perror(iface->i_name);
	  fprintf(stderr,"Can't configure multicast.\n");
	  close(fd);
	  return -1;
	}

	LOG(log_info, logtype_atalkd, "plumbed %s%d", device, ppa );
    }

    return( 0 );
}
#endif /* __svr4__ */
