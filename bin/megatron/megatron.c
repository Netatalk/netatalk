#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/uio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <netatalk/endian.h>
#include "asingle.h"
#include "megatron.h"
#include "hqx.h"
#include "macbin.h"
#include "nad.h"

char		*forkname[] = { "data", "resource" };
static char	forkbuf[8192];
static char	*name[] = { "unhex",
			    "unbin",
			    "unsingle",
			    "macbinary",
			    "hqx2bin",
			    "single2bin",
			    "nadheader",
			    "binheader",
			    "megatron" };

static int from_open(int un, char *file, struct FHeader *fh, int flags)
{
    switch ( un ) {
	case MEGATRON :
	case HEX2NAD :
	case HEX2BIN :
	    return( hqx_open( file, O_RDONLY, fh, flags ));
	    break;
	case BIN2NAD :
        case BINHEADER:
	    return( bin_open( file, O_RDONLY, fh, flags ));
	    break;
	case NAD2BIN :
        case NADHEADER:
	    return( nad_open( file, O_RDONLY, fh, flags ));
	    break;
	case SINGLE2NAD :
	case SINGLE2BIN :
	    return( single_open( file, O_RDONLY, fh, flags ));
	default :
	    return( -1 );
	    break;
    }
}

static ssize_t from_read(int un, int fork, char *buf, size_t len)
{
    switch ( un ) {
	case MEGATRON :
	case HEX2NAD :
	case HEX2BIN :
	    return( hqx_read( fork, buf, len ));
	    break;
	case BIN2NAD :
	    return( bin_read( fork, buf, len ));
	    break;
	case NAD2BIN :
	    return( nad_read( fork, buf, len ));
	    break;
	case SINGLE2NAD :
	case SINGLE2BIN :
	    return( single_read( fork, buf, len ));
	default :
	    return( -1 );
	    break;
    }
}

static int from_close(int un)
{
    switch ( un ) {
	case MEGATRON :
	case HEX2NAD :
	case HEX2BIN :
	    return( hqx_close( KEEP ));
	    break;
	case BIN2NAD :
	    return( bin_close( KEEP ));
	    break;
	case NAD2BIN :
	    return( nad_close( KEEP ));
	    break;
	case SINGLE2NAD :
	case SINGLE2BIN :
	    return( single_close( KEEP ));
	default :
	    return( -1 );
	    break;
    }
}

static int to_open(int to, char *file, struct FHeader *fh, int flags)
{
    switch ( to ) {
	case MEGATRON :
	case HEX2NAD :
	case BIN2NAD :
	case SINGLE2NAD :
	    return( nad_open( file, O_RDWR|O_CREAT|O_EXCL, fh, flags ));
	    break;
	case NAD2BIN :
	case HEX2BIN :
	case SINGLE2BIN :
	    return( bin_open( file, O_RDWR|O_CREAT|O_EXCL, fh, flags ));
	    break;
	default :
	    return( -1 );
	    break;
    }
}

static ssize_t to_write(int to, int fork, size_t bufc)
{
    switch ( to ) {
	case MEGATRON :
	case HEX2NAD :
	case BIN2NAD :
	case SINGLE2NAD :
	    return( nad_write( fork, forkbuf, bufc ));
	    break;
	case NAD2BIN :
	case HEX2BIN :
	case SINGLE2BIN :
	    return( bin_write( fork, forkbuf, bufc ));
	    break;
	default :
	    return( -1 );
	    break;
    }
}

static int to_close(int to, int keepflag)
{
    switch ( to ) {
	case MEGATRON :
	case HEX2NAD :
	case BIN2NAD :
	case SINGLE2NAD :
	    return( nad_close( keepflag ));
	    break;
	case NAD2BIN :
	case HEX2BIN :
	case SINGLE2BIN :
	    return( bin_close( keepflag ));
	    break;
	default :
	    return( -1 );
	    break;
    }
}

static int megatron( char *path, int module, char *newname, int flags)
{
    struct stat		st;
    struct FHeader	fh;
    ssize_t		bufc;
    int			fork;
    size_t 		forkred;

/*
 * If the source file is not stdin, make sure it exists and
 * that it is not a directory.
 */

    if ( strcmp( path, STDIN ) != 0 ) {
	if ( stat( path, &st ) < 0 ) {
	    perror( path );
	    return( -1 );
	}
	if ( S_ISDIR( st.st_mode )) {
	    fprintf( stderr, "%s is a directory.\n", path );
	    return( 0 );
	}
    }

/*
 * Open the source file and fill in the file header structure.
 */

    memset( &fh, 0, sizeof( fh ));
    if ( from_open( module, path, &fh, flags ) < 0 ) {
	return( -1 );
    }

    if ( flags & OPTION_HEADERONLY ) {
        time_t t;
	char buf[5] = "";
        int i;

        printf("name:               %s\n",fh.name);
        printf("comment:            %s\n",fh.comment);
	memcpy(&buf, &fh.finder_info.fdCreator, sizeof(u_int32_t));
	printf("creator:            '%4s'\n", buf);
	memcpy(&buf, &fh.finder_info.fdType, sizeof(u_int32_t));
	printf("type:               '%4s'\n", buf);
        for(i=0; i < NUMFORKS; ++i) 
	  printf("fork length[%d]:     %u\n", i, ntohl(fh.forklen[i]));
	t = AD_DATE_TO_UNIX(fh.create_date);
        printf("creation date:      %s", ctime(&t));
	t = AD_DATE_TO_UNIX(fh.mod_date);
        printf("modification date:  %s", ctime(&t));
	t = AD_DATE_TO_UNIX(fh.backup_date);
        printf("backup date:        %s", ctime(&t));
	return( from_close( module ));
    }
    
/*
 * Open the target file and write out the file header info.
 * set the header to the new filename if it has been supplied.
 */

    if (*newname)
        strcpy(fh.name, newname);

    if ( to_open( module, path, &fh, flags ) < 0 ) {
	(void)from_close( module );
	return( -1 );
    }

/*
 * Read in and write out the data and resource forks.
 */

    for ( fork = 0; fork < NUMFORKS ; fork++ ) {
	forkred = 0;
	while(( bufc = from_read( module, fork, forkbuf, sizeof( forkbuf )))
		> 0 ) {
	    if ( to_write( module, fork, bufc ) != bufc ) {
		fprintf( stderr, "%s: Probable write error\n", path );
		to_close( module, TRASH );
		(void)from_close( module );
		return( -1 );
	    }
	    forkred += bufc;
	}
#if DEBUG
	fprintf( stderr, "megatron: forkred is \t\t%d\n", forkred );
	fprintf( stderr, "megatron: fh.forklen[%d] is \t%d\n", fork, 
		ntohl( fh.forklen[ fork ] ));
#endif /* DEBUG */
	if (( bufc < 0 ) || ( forkred != ntohl( fh.forklen[ fork ] ))) {
	    fprintf( stderr, "%s: Problem with input, dude\n", path );
	    to_close( module, TRASH );
	    (void)from_close( module );
	    return( -1 );
	}
    }

/*
 * Close up the files, and get out of here.
 */

    if ( to_close( module, KEEP ) < 0 ) {
	perror( "megatron:" );
	(void)to_close( module, TRASH );
    }
    return( from_close( module ));
}

int main(int argc, char **argv)
{
    int		rc, c;
    int		rv = 0;
    int		converts = sizeof(name) / sizeof(char *);
    int		module = -1;
    int         flags = 0;
    char	*progname, newname[ADEDLEN_NAME + 1];

    progname = strrchr( argv[ 0 ], '/' );
    if (( progname == NULL ) || ( *progname == '\0' )) {
	progname = argv[ 0 ];
    } else progname++;

#if DEBUG
    if ( CONVERTS != converts ) {
	fprintf( stderr, "megatron: list of program links messed up\n" );
	return( -1 );
    }
#endif /* DEBUG */

    for ( c = 0 ; (( c < converts ) && ( module < 0 )) ; ++c ) {
	if ( strcmp( name[ c ], progname ) == 0 ) module = c;
    }
    if ( module == -1 ) module = ( converts - 1 );
    if ((module == NADHEADER) || (module == BINHEADER))
      flags |= OPTION_HEADERONLY;

    if ( argc == 1 ) {
	return( megatron( STDIN, module, newname, flags ));
    }

    *newname = '\0';
    for ( c = 1 ; c < argc ; ++c ) {
        if ( strcmp( argv [ c ], "--version" ) == 0 ) {
	    printf("%s (Netatalk %s megatron)\n", argv[0], VERSION);
	    return( -1 );
	}
        if ( strcmp( argv [ c ], "-v" ) == 0 ) {
	    printf("%s (Netatalk %s megatron)\n", argv[0], VERSION);
	    return( -1 );
	}
        if ( strcmp( argv [ c ], "--header" ) == 0 ) {
	    flags |= OPTION_HEADERONLY;
	    continue;
	}
	if ( strcmp( argv [ c ], "--filename" ) == 0 ) {
	  if(++c < argc) strncpy(newname,argv[c], ADEDLEN_NAME);
	  continue;
	}
	if (strcmp(argv[c], "--stdout") == 0) {
	  flags |= OPTION_STDOUT;
	  continue;
	}
	if (strcmp(argv[c], "--euc") == 0) {
	  flags |= OPTION_EUCJP;
	  continue;
	}  
	if (strcmp(argv[c], "--sjis") == 0) {
	  flags |= OPTION_SJIS;
	  continue;
	}  
	rc = megatron( argv[ c ], module, newname, flags);
	if ( rc != 0 ) {
	    rv = rc;
	}
	*newname = '\0';
    }
    return( rv );
}
