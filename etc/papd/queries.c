/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <string.h>
#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netatalk/at.h>
#include <atalk/atp.h>

#ifdef KRB
#ifdef SOLARIS
#include <kerberos/krb.h>
#else
#include <krb.h>
#endif
#endif KRB

#include "file.h"
#include "comment.h"
#include "printer.h"
#include "ppd.h"

cq_default( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    struct comment	*comment = compeek();

    for (;;) {
	switch ( markline( &start, &stop, in )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	if ( comgetflags() == 0 ) {	/* started */
	    if ( comment->c_end ) {
		comsetflags( 1 );
	    } else {
		compop();
		consumetomark( start, stop, in );
		return( CH_DONE );
	    }
	} else {
	    /* return default */
	    if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) {
		for ( p = start; p < stop; p++ ) {
		    if ( *p == ':' ) {
			break;
		    }
		}
		p++;
		while ( *p == ' ' ) {
		    p++;
		}

		*stop = '\n';
		APPEND( out, p, stop - p + 1 );
		compop();
		consumetomark( start, stop, in );
		return( CH_DONE );
	    }
	}

	consumetomark( start, stop, in );
    }
}

#ifdef KRB
char	*LoginOK = "LoginOK\n";
char	*LoginFailed = "LoginFailed\n";

#define h2b(x)	(isdigit((x))?(x)-'0':(isupper((x))?(x)-'A':(x)-'a')+10)

cq_k4login( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    unsigned char	*t;
    struct comment	*comment = compeek();
    KTEXT_ST		tkt;
    AUTH_DAT		ad;
    int			rc, i;

    switch ( markline( &start, &stop, in )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    p = start + strlen( comment->c_begin );
    while ( *p == ' ' ) {
	p++;
    }

    bzero( &tkt, sizeof( tkt ));
    for ( i = 0, t = tkt.dat; p < stop; p += 2, t++, i++ ) {
	*t = ( h2b( (unsigned char)*p ) << 4 ) +
		h2b( (unsigned char)*( p + 1 ));
    }
    tkt.length = i;

    if (( rc = krb_rd_req( &tkt, "LaserWriter", printer->p_name,
	    0, &ad, "" )) != RD_AP_OK ) {
	syslog( LOG_ERR, "cq_k4login: %s", krb_err_txt[ rc ] );
	APPEND( out, LoginFailed, strlen( LoginFailed ));
	compop();
	consumetomark( start, stop, in );
	return( CH_DONE );
    }
    syslog( LOG_INFO, "cq_k4login: %s.%s@%s", ad.pname, ad.pinst,
	    ad.prealm );
    lp_person( ad.pname );
    lp_host( ad.prealm );

    APPEND( out, LoginOK, strlen( LoginOK ));
    compop();
    consumetomark( start, stop, in );
    return( CH_DONE );
}

char	*uameth = "UMICHKerberosIV\n*\n";

cq_uameth( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop;
    struct comment	*c, *comment = compeek();

    for (;;) {
	switch ( markline( &start, &stop, in )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	if ( comgetflags() == 0 ) {	/* start */
	    if (( printer->p_flags & P_AUTH ) == 0 ) {	/* no kerberos */
		if ( comswitch( queries, cq_default ) < 0 ) {
		    syslog( LOG_ERR, "cq_uameth: can't find default!" );
		    exit( 1 );
		}
		return( CH_DONE );
	    }
	    comsetflags( 1 );
	} else {
	    if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) { /* end */
		APPEND( out, uameth, strlen( uameth ));
		compop();
		return( CH_DONE );
	    }
	}

	consumetomark( start, stop, in );
    }
}
#endif KRB

gq_true( out )
    struct papfile	*out;
{
    if ( printer->p_flags & P_SPOOLED ) {
	APPEND( out, "true\n", 5 );
	return( 0 );
    } else {
	return( -1 );
    }
}

gq_pagecost( out )
    struct papfile	*out;
{
    char		cost[ 60 ];

    /* check for spooler? XXX */
    if ( printer->p_pagecost_msg != NULL ) {
	APPEND( out, printer->p_pagecost_msg,
		strlen( printer->p_pagecost_msg ));
    } else if ( printer->p_flags & P_ACCOUNT ) {
#ifdef ABS_PRINT
	lp_pagecost();
#endif ABS_PRINT
	sprintf( cost, "%d", printer->p_pagecost );
	APPEND( out, cost, strlen( cost ));
    } else {
	return( -1 );
    }
    APPEND( out, "\n", 1 );
    return( 0 );
}

#ifdef ABS_PRINT
gq_balance( out )
    struct papfile	*out;
{
    char		balance[ 60 ];

    if ( lp_pagecost() != 0 ) {
	return( -1 );
    }
    sprintf( balance, "$%1.2f\n", printer->p_balance );
    APPEND( out, balance, strlen( balance ));
    return( 0 );
}
#endif ABS_PRINT

struct genquery {
    char	*gq_name;
    int		(*gq_handler)();
} genqueries[] = {
    { "UMICHCostPerPage", gq_pagecost },
#ifdef notdef
    { "UMICHUserBalance", gq_balance },
#endif 
    { "UMICHListQueue", gq_true },
    { "UMICHDeleteJob", gq_true },
    { NULL },
};

cq_query( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p, *q;
    struct comment	*comment = compeek();
    struct genquery	*gq;


    for (;;) {
	switch ( markline( &start, &stop, in )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	if ( comgetflags() == 0 ) {	/* started */
	    comsetflags( 1 );

	    for ( p = start; p < stop; p++ ) {
		if ( *p == ':' ) {
		    break;
		}
	    }

	    for ( p++; p < stop; p++ ) {
		if ( *p != ' ' && *p != '\t' ) {
		    break;
		}
	    }

	    for ( q = p; q < stop; q++ ) {
		if ( *q == ' ' || *q == '\t' || *q == '\r' || *q == '\n' ) {
		    break;
		}
	    }

	    for ( gq = genqueries; gq->gq_name; gq++ ) {
		if (( strlen( gq->gq_name ) == q - p ) &&
			( strncmp( gq->gq_name, p, q - p ) == 0 )) {
		    break;
		}
	    }
	    if ( gq->gq_name == NULL || gq->gq_handler == NULL ||
		    (gq->gq_handler)( out ) < 0 ) {
		if ( comswitch( queries, cq_default ) < 0 ) {
		    syslog( LOG_ERR, "cq_feature: can't find default!" );
		    exit( 1 );
		}
		return( CH_DONE );
	    }
	} else {
	    if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) {
		compop();
		consumetomark( start, stop, in );
		return( CH_DONE );
	    }
	}

	consumetomark( start, stop, in );
    }
}

cq_font_answer( start, stop, out )
    char		*start, *stop;
    struct papfile	*out;
{
    char		*p, *q, buf[ 256 ];
    struct ppd_font	*pfo;

    p = start;
    while ( p < stop ) {
	while (( *p == ' ' || *p == '\t' ) && p < stop ) {
	    p++;
	}

	q = buf;
	while ( *p != ' ' && *p != '\t' &&
		*p != '\n' && *p != '\r' && p < stop ) {
	    *q++ = *p++;
	}

	if ( q != buf ) {
	    *q = '\0';

	    APPEND( out, "/", 1 );
	    APPEND( out, buf, strlen( buf ));
	    APPEND( out, ":", 1 );

	    if (( pfo = ppd_font( buf )) == NULL ) {
		APPEND( out, "No\n", 3 );
	    } else {
		APPEND( out, "Yes\n", 4 );
	    }
	}
    }

    return;
}

cq_font( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    struct comment	*comment = compeek();

    for (;;) {
	switch ( markline( &start, &stop, in )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	if ( comgetflags() == 0 ) {
	    comsetflags( 1 );

	    for ( p = start; p < stop; p++ ) {
		if ( *p == ':' ) {
		    break;
		}
	    }
	    p++;

	    cq_font_answer( p, stop, out );
	} else {
	    if ( comgetflags() == 1 &&
		    comcmp( start, stop, comcont, 0 ) == 0 ) {
		/* continuation */

		for ( p = start; p < stop; p++ ) {
		    if ( *p == ' ' ) {
			break;
		    }
		}
		p++;

		cq_font_answer( p, stop, out );
	    } else {
		comsetflags( 2 );
		if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) {
		    APPEND( out, "*\n", 2 );
		    compop();
		    consumetomark( start, stop, in );
		    return( CH_DONE );
		}
	    }
	}

	consumetomark( start, stop, in );
    }
}

cq_feature( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    struct comment	*comment = compeek();
    struct ppd_feature	*pfe;

    for (;;) {
	switch ( markline( &start, &stop, in )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	if ( comgetflags() == 0 ) {
	    comsetflags( 1 );

	    /* parse for feature */
	    for ( p = start; p < stop; p++ ) {
		if ( *p == ':' ) {
		    break;
		}
	    }
	    p++;
	    while ( *p == ' ' ) {
		p++;
	    }

	    if (( pfe = ppd_feature( p, stop - p )) == NULL ) {
		if ( comswitch( queries, cq_default ) < 0 ) {
		    syslog( LOG_ERR, "cq_feature: can't find default!" );
		    exit( 1 );
		}
		return( CH_DONE );
	    }

	    APPEND( out, pfe->pd_value, strlen( pfe->pd_value ));
	    APPEND( out, "\r", 1 );
	} else {
	    if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) {
		compop();
		consumetomark( start, stop, in );
		return( CH_DONE );
	    }
	}

	consumetomark( start, stop, in );
    }
}

static const char	*psver = "*PSVersion\n";
static const char	*prod = "*Product\n";

cq_printer( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    struct comment	*c, *comment = compeek();
    struct ppd_feature	*pdpsver, *pdprod;

    for (;;) {
	switch ( markline( &start, &stop, in )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	if ( comgetflags() == 0 ) {
	    comsetflags( 1 );

	    if (( pdpsver = ppd_feature( psver, strlen( psver ))) == NULL ) {
		if ( comswitch( queries, cq_default ) < 0 ) {
		    syslog( LOG_ERR, "cq_printer: can't find default!" );
		    exit( 1 );
		}
		return( CH_DONE );
	    }

	    for ( p = pdpsver->pd_value; *p != '\0'; p++ ) {
		if ( *p == ' ' ) {
		    break;
		}
	    }
	    if ( *p == '\0' ) {
		syslog( LOG_ERR, "cq_printer: can't parse PSVersion!" );
		if ( comswitch( queries, cq_default ) < 0 ) {
		    syslog( LOG_ERR, "cq_printer: can't find default!" );
		    exit( 1 );
		}
		return( CH_DONE );
	    }

	    if (( pdprod = ppd_feature( prod, strlen( prod ))) == NULL ) {
		if ( comswitch( queries, cq_default ) < 0 ) {
		    syslog( LOG_ERR, "cq_printer: can't find default!" );
		    exit( 1 );
		}
		return( CH_DONE );
	    }

	    /* revision */
	    APPEND( out, p + 1, strlen( p + 1 ));
	    APPEND( out, "\r", 1 );

	    /* version */
	    APPEND( out, pdpsver->pd_value, p - pdpsver->pd_value );
	    APPEND( out, "\r", 1 );

	    /* product */
	    APPEND( out, pdprod->pd_value, strlen( pdprod->pd_value ));
	    APPEND( out, "\r", 1 );
	} else {
	    if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) {
		compop();
		consumetomark( start, stop, in );
		return( CH_DONE );
	    }
	}

	consumetomark( start, stop, in );
    }
}

static const char	*rmjobfailed = "Failed\n";
static const char	*rmjobok = "Ok\n";

cq_rmjob( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    int			job;

    switch ( markline( &start, &stop, in )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    for ( p = start; p < stop; p++ ) {
	if ( *p == ' ' || *p == '\t' ) {
	    break;
	}
    }
    for ( ; p < stop; p++ ) {
	if ( *p != ' ' && *p != '\t' ) {
	    break;
	}
    }

    *stop = '\0';
    if ( p < stop && ( job = atoi( p )) > 0 ) {
	lp_rmjob( job );
	APPEND( out, rmjobok, strlen( rmjobok ));
    } else {
	APPEND( out, rmjobfailed, strlen( rmjobfailed ));
    }

    compop();
    consumetomark( start, stop, in );
    return( CH_DONE );
}

cq_listq( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop;

    switch ( markline( &start, &stop, in )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    if ( lp_queue( out )) {
	syslog( LOG_ERR, "cq_listq: lp_queue failed" );
    }

    compop();
    consumetomark( start, stop, in );
    return( CH_DONE );
}

/*
 * All queries start with %%?Begin and end with %%?End.  Note that the
 * "Begin"/"End" general queries have to be last.
 */
struct comment	queries[] = {
#ifdef KRB
    { "%%Login: UMICHKerberosIV", 0,			cq_k4login,	0 },
    { "%%?BeginUAMethodsQuery",	"%%?EndUAMethodsQuery:", cq_uameth, C_FULL },
#endif KRB
    { "%UMICHListQueue", 0,				cq_listq, C_FULL },
    { "%UMICHDeleteJob", 0,				cq_rmjob,	0 },
    { "%%?BeginQuery",		"%%?EndQuery",		cq_query,	0 },
    { "%%?BeginFeatureQuery",	"%%?EndFeatureQuery",	cq_feature,	0 },
    { "%%?BeginFontQuery",	"%%?EndFontQuery",	cq_font,	0 },
    { "%%?BeginPrinterQuery",	"%%?EndPrinterQuery",	cq_printer, C_FULL },
    { "%%?Begin",		"%%?End",		cq_default,	0 },
    { 0 },
};
