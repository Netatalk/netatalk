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
#include "uam_auth.h"

cq_default( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    struct comment	*comment = compeek();

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	stop = start+linelength;

	if ( comgetflags() == 0 ) {	/* started */
	    if ( comment->c_end ) {
		comsetflags( 1 );
	    } else {
		compop();
		CONSUME( in, linelength + crlflength );
		return( CH_DONE );
	    }
	} else {
	    /* return default */
	    if ( comcmp( start, start+linelength, comment->c_end, 0 ) == 0 ) {
		for ( p = start; p < stop; p++ ) {
		    if ( *p == ':' ) {
			break;
		    }
		}
		p++;
		while ( *p == ' ' ) {
		    p++;
		}

		append( out, p, stop - p + crlflength );
		compop();
		CONSUME( in, linelength + crlflength );
		return( CH_DONE );
	    }
	}

	CONSUME( in, linelength + crlflength );
    }
}

#ifdef KRB
char	*LoginOK = "LoginOK\n";
char	*LoginFailed = "LoginFailed\n";

#define h2b(x)	(isdigit((x))?(x)-'0':(isupper((x))?(x)-'A':(x)-'a')+10)

cq_k4login( in, out )
    struct papfile	*in, *out;
{
    char		*start, *p;
    int			linelength, crlflength;
    unsigned char	*t;
    struct comment	*comment = compeek();
    KTEXT_ST		tkt;
    AUTH_DAT		ad;
    int			rc, i;

    switch ( markline( in, &start, &linelength, &crlflength )) {
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
    stop = start+linelength;
    for ( i = 0, t = tkt.dat; p < stop; p += 2, t++, i++ ) {
	*t = ( h2b( (unsigned char)*p ) << 4 ) +
		h2b( (unsigned char)*( p + 1 ));
    }
    tkt.length = i;

    if (( rc = krb_rd_req( &tkt, "LaserWriter", printer->p_name,
	    0, &ad, "" )) != RD_AP_OK ) {
	syslog( LOG_ERR, "cq_k4login: %s", krb_err_txt[ rc ] );
	append( out, LoginFailed, strlen( LoginFailed ));
	compop();
	CONSUME( in, linelength + crlflength );
	return( CH_DONE );
    }
    syslog( LOG_INFO, "cq_k4login: %s.%s@%s", ad.pname, ad.pinst,
	    ad.prealm );
    lp_person( ad.pname );
    lp_host( ad.prealm );

    append( out, LoginOK, strlen( LoginOK ));
    compop();
    CONSUME( in, linelength + crlflength);
    return( CH_DONE );
}

char	*uameth = "UMICHKerberosIV\n*\n";

cq_uameth( in, out )
    struct papfile	*in, *out;
{
    char		*start;
    int			linelength, crlflength;
    struct comment	*c, *comment = compeek();

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	if ( comgetflags() == 0 ) {	/* start */
	    if (( printer->p_flags & P_KRB ) == 0 ) {	/* no kerberos */
		if ( comswitch( queries, cq_default ) < 0 ) {
		    syslog( LOG_ERR, "cq_uameth: can't find default!" );
		    exit( 1 );
		}
		return( CH_DONE );
	    }
	    comsetflags( 1 );
	} else {
	    if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) { /* end */
		append( out, uameth, strlen( uameth ));
		compop();
		return( CH_DONE );
	    }
	}

	CONSUME( in, linelength + crlflength );
    }
}
#endif KRB

gq_true( out )
    struct papfile	*out;
{
    if ( printer->p_flags & P_SPOOLED ) {
	append( out, "true\n", 5 );
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
	append( out, printer->p_pagecost_msg,
		strlen( printer->p_pagecost_msg ));
    } else if ( printer->p_flags & P_ACCOUNT ) {
#ifdef ABS_PRINT
	lp_pagecost();
#endif ABS_PRINT
	sprintf( cost, "%d", printer->p_pagecost );
	append( out, cost, strlen( cost ));
    } else {
	return( -1 );
    }
    append( out, "\n", 1 );
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
    append( out, balance, strlen( balance ));
    return( 0 );
}
#endif ABS_PRINT


/*
 * Handler for RBISpoolerID
 */

static const char *spoolerid = "(PAPD Spooler) 2.1 (2.1.4 pre-release)\n";

gq_rbispoolerid( out )
    struct papfile	*out;
{
    append( out, spoolerid, strlen( spoolerid ));
    return(0);
}



/*
 * Handler for RBIUAMListQuery
 */

static const char *nouams = "*\n";

gq_rbiuamlist( out )
    struct papfile      *out;
{
    char uamnames[128] = "\0";

    if (printer->p_flags & P_AUTH_PSSP) {
	if (getuamnames(UAM_SERVER_PRINTAUTH, uamnames) < 0) {
	    append(out, nouams, strlen(nouams));
	    return(0);
	} else {
	    append(out, uamnames, strlen(uamnames));
	    return(0);
	}
    } else {
	append(out, nouams, strlen(nouams));
	return(0);
    }
}


struct genquery {
    char	*gq_name;
    int		(*gq_handler)();
} genqueries[] = {
    { "UMICHCostPerPage", gq_pagecost },
#ifdef notdef
    { "UMICHUserBalance", gq_balance },
#endif 
    { "RBISpoolerID",	gq_rbispoolerid },
    { "RBIUAMListQuery", gq_rbiuamlist },
    { "UMICHListQueue", gq_true },
    { "UMICHDeleteJob", gq_true },
    { NULL },
};

cq_query( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p, *q;
    int			linelength, crlflength;
    struct comment	*comment = compeek();
    struct genquery	*gq;


    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	stop = start+linelength;

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
		CONSUME( in, linelength + crlflength );
		return( CH_DONE );
	    }
	}

	CONSUME( in, linelength + crlflength );
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

	    append( out, "/", 1 );
	    append( out, buf, strlen( buf ));
	    append( out, ":", 1 );

	    if (( pfo = ppd_font( buf )) == NULL ) {
		append( out, "No\n", 3 );
	    } else {
		append( out, "Yes\n", 4 );
	    }
	}
    }

    return;
}

cq_font( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    struct comment	*comment = compeek();

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	stop = start + linelength;

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
		    append( out, "*\n", 2 );
		    compop();
		    CONSUME( in, linelength + crlflength );
		    return( CH_DONE );
		}
	    }
	}

        CONSUME( in, linelength + crlflength );
    }
}

cq_feature( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    struct comment	*comment = compeek();
    struct ppd_feature	*pfe;

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );
	}

	stop = start + linelength;

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

	    append( out, pfe->pd_value, strlen( pfe->pd_value ));
	    append( out, "\r", 1 );
	} else {
	    if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) {
		compop();
		CONSUME( in, linelength + crlflength );
		return( CH_DONE );
	    }
	}

	CONSUME( in, linelength + crlflength );
    }
}

static const char	*psver = "*PSVersion\n";
static const char	*prod = "*Product\n";

cq_printer( in, out )
    struct papfile	*in, *out;
{
    char		*start, *p;
    int			linelength, crlflength;
    struct comment	*comment = compeek();
    struct ppd_feature	*pdpsver, *pdprod;

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
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
	    append( out, p + 1, strlen( p + 1 ));
	    append( out, "\r", 1 );

	    /* version */
	    append( out, pdpsver->pd_value, p - pdpsver->pd_value );
	    append( out, "\r", 1 );

	    /* product */
	    append( out, pdprod->pd_value, strlen( pdprod->pd_value ));
	    append( out, "\r", 1 );
	} else {
	    if ( comcmp( start, start+linelength, comment->c_end, 0 ) == 0 ) {
		compop();
		CONSUME( in, linelength + crlflength );
		return( CH_DONE );
	    }
	}

	CONSUME( in, linelength + crlflength );
    }
}

static const char	*rmjobfailed = "Failed\n";
static const char	*rmjobok = "Ok\n";

cq_rmjob( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    int			job;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    stop = start + linelength;

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
	append( out, rmjobok, strlen( rmjobok ));
    } else {
	append( out, rmjobfailed, strlen( rmjobfailed ));
    }

    compop();
    CONSUME( in, linelength + crlflength );
    return( CH_DONE );
}

cq_listq( in, out )
    struct papfile	*in, *out;
{
    char		*start;
    int			linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    if ( lp_queue( out )) {
	syslog( LOG_ERR, "cq_listq: lp_queue failed" );
    }

    compop();
    CONSUME( in, linelength + crlflength );
    return( CH_DONE );
}


/*
 * Handler for RBILogin
 */

static struct uam_obj *papd_uam = NULL;
static const char *rbiloginok = "0\r";
static const char *rbiloginbad = "-1\r";
static const char *rbiloginerrstr = "%%[Error: SecurityError; \
SecurityViolation: Unknown user, incorrect password or log on is \
disabled ]%%\r%%Flushing: rest of job (to end-of-file) will be \
ignored ]%%\r";

cq_rbilogin( in, out )
    struct papfile      *in, *out;
{
    char        *start, *stop, *p, *begin;
    int		linelength, crlflength;
    char        username[9] = "\0";
    struct comment      *comment = compeek();
    char	uamtype[20] = "\0";

    for (;;) {
        switch ( markline( in, &start, &linelength, &crlflength )) {
        case 0 :
            return( 0 );

        case -1 :
            return( CH_MORE );
        }

	stop = start + linelength;

        if ( comgetflags() == 0 ) { /* first line */
	    begin = start + strlen(comment->c_begin);
	    p = begin;

	    while (*p != ' ') {
		p++;
	    }

	    strncat(uamtype, begin, p - begin);

	    if ((papd_uam = auth_uamfind(UAM_SERVER_PRINTAUTH,
				uamtype, strlen(uamtype))) == NULL) {
		syslog(LOG_INFO, "Could not find uam: %s", uamtype);
		append(out, rbiloginbad, strlen(rbiloginbad));
		append(out, rbiloginerrstr, strlen(rbiloginerrstr));
	    } else {
                if ( (papd_uam->u.uam_printer(p,stop,username,out)) == 0 ) {
                    lp_person( username );
                } else {
                    append(out, rbiloginbad, strlen( rbiloginbad));
                    append(out, rbiloginerrstr, strlen(rbiloginerrstr));
                }
	    }
            comsetflags( 1 );
        } else {
            if ( comcmp( start, stop, comment->c_end, 0 ) == 0 ) {
                compop();
                return( CH_DONE );
            }
        }

        CONSUME( in, linelength + crlflength );
    }
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
    { "%%?BeginQuery: RBILogin ", "%%?EndQuery",	cq_rbilogin,	0 },
    { "%%?BeginQuery",		"%%?EndQuery",		cq_query,	0 },
    { "%%?BeginFeatureQuery",	"%%?EndFeatureQuery",	cq_feature,	0 },
    { "%%?BeginFontQuery",	"%%?EndFontQuery",	cq_font,	0 },
    { "%%?BeginPrinterQuery",	"%%?EndPrinterQuery",	cq_printer, C_FULL },
    { "%%?Begin",		"%%?End",		cq_default,	0 },
    { 0 },
};
