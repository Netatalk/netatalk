/*
 * $Id: queries.c,v 1.22 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <atalk/logger.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netatalk/at.h>
#include <atalk/atp.h>

#ifdef KRB
#ifdef SOLARIS
#include <kerberos/krb.h>
#else /* SOLARIS */
#include <krb.h>
#endif /* SOLARIS */
#endif /* KRB */

#include "file.h"
#include "comment.h"
#include "printer.h"
#include "ppd.h"
#include "lp.h"
#include "uam_auth.h"

int cq_default( struct papfile *, struct papfile * );
int cq_k4login( struct papfile *, struct papfile * );
int cq_uameth( struct papfile *, struct papfile * );

int gq_balance( struct papfile * );
int gq_pagecost( struct papfile * );
int gq_true( struct papfile * );
int gq_rbispoolerid( struct papfile * );
int gq_rbiuamlist( struct papfile * );

int cq_query( struct papfile *, struct papfile * );
void cq_font_answer( char *, char *, struct papfile * );
int cq_font( struct papfile *, struct papfile * );
int cq_feature( struct papfile *, struct papfile * );
int cq_printer( struct papfile *, struct papfile * );
int cq_rmjob( struct papfile *, struct papfile * );
#ifndef HAVE_CUPS
int cq_listq( struct papfile *, struct papfile * );
int cq_rbilogin( struct papfile *, struct papfile * );
#endif  /* HAVE_CUPS */



int cq_default( struct papfile *in, struct papfile *out)
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    struct papd_comment	*comment = compeek();

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );

        case -2 :
            return( CH_ERROR );
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
		if (p < stop) {
		    p++;
		    while ( *p == ' ' ) {
		        p++;
                    }
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

int cq_k4login( struct papfile *in, struct papfile *out)
{
    char		*start, *p;
    int			linelength, crlflength;
    unsigned char	*t;
    struct papd_comment	*comment = compeek();
    KTEXT_ST		tkt;
    AUTH_DAT		ad;
    int			rc, i;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );

    case -2 :
        return( CH_ERROR );
    }

    p = start + strlen( comment->c_begin );
    while ( *p == ' ' ) {
	p++;
    }

    bzero( &tkt, sizeof( tkt ));
    stop = start+linelength;
    /* FIXME */
    for ( i = 0, t = tkt.dat; p < stop; p += 2, t++, i++ ) {
	*t = ( h2b( (unsigned char)*p ) << 4 ) +
		h2b( (unsigned char)*( p + 1 ));
    }
    tkt.length = i;

    if (( rc = krb_rd_req( &tkt, "LaserWriter", printer->p_name,
	    0, &ad, "" )) != RD_AP_OK ) {
	LOG(log_error, logtype_papd, "cq_k4login: %s", krb_err_txt[ rc ] );
	append( out, LoginFailed, strlen( LoginFailed ));
	compop();
	CONSUME( in, linelength + crlflength );
	return( CH_DONE );
    }
    LOG(log_info, logtype_papd, "cq_k4login: %s.%s@%s", ad.pname, ad.pinst,
	    ad.prealm );
    lp_person( ad.pname );
    lp_host( ad.prealm );

    append( out, LoginOK, strlen( LoginOK ));
    compop();
    CONSUME( in, linelength + crlflength);
    return( CH_DONE );
}

char	*uameth = "UMICHKerberosIV\n*\n";

int cq_uameth( struct papfile *in, struct papfile *out)
{
    char		*start;
    int			linelength, crlflength;
    struct papd_comment	*c, *comment = compeek();

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );

        case -2 :
            return( CH_ERROR );
	}

	if ( comgetflags() == 0 ) {	/* start */
	    if (( printer->p_flags & P_KRB ) == 0 ) {	/* no kerberos */
		if ( comswitch( queries, cq_default ) < 0 ) {
		    LOG(log_error, logtype_papd, "cq_uameth: can't find default!" );
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
#endif /* KRB */

int gq_true( struct papfile *out)
{
    if ( printer->p_flags & P_SPOOLED ) {
	append( out, "true\n", 5 );
	return( 0 );
    } else {
	return( -1 );
    }
}

int gq_pagecost( struct papfile *out)
{
    char		cost[ 60 ];

    /* check for spooler? XXX */
    if ( printer->p_pagecost_msg != NULL ) {
	append( out, printer->p_pagecost_msg,
		strlen( printer->p_pagecost_msg ));
    } else if ( printer->p_flags & P_ACCOUNT ) {
#ifdef ABS_PRINT
	lp_pagecost();
#endif /* ABS_PRINT */
	sprintf( cost, "%d", printer->p_pagecost );
	append( out, cost, strlen( cost ));
    } else {
	return( -1 );
    }
    append( out, "\n", 1 );
    return( 0 );
}

#ifdef ABS_PRINT
int gq_balance( struct papfile *out)
{
    char		balance[ 60 ];

    if ( lp_pagecost() != 0 ) {
	return( -1 );
    }
    sprintf( balance, "$%1.2f\n", printer->p_balance );
    append( out, balance, strlen( balance ));
    return( 0 );
}
#endif /* ABS_PRINT */


/*
 * Handler for RBISpoolerID
 */

static const char *spoolerid = "(PAPD Spooler) 1.0 (" VERSION ")\n";

int gq_rbispoolerid( struct papfile *out)
{
    append( out, spoolerid, strlen( spoolerid ));
    return(0);
}



/*
 * Handler for RBIUAMListQuery
 */

static const char *nouams = "*\n";

int gq_rbiuamlist( struct papfile *out)
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
#endif /* notdef */
    { "RBISpoolerID",	gq_rbispoolerid },
    { "RBIUAMListQuery", gq_rbiuamlist },
    { "ADOIsBinaryOK?", gq_true },
    { "UMICHListQueue", gq_true },
    { "UMICHDeleteJob", gq_true },
    { NULL, NULL },
};

int cq_query( struct papfile *in, struct papfile *out)
{
    char		*start, *stop, *p, *q;
    int			linelength, crlflength;
    struct papd_comment	*comment = compeek();
    struct genquery	*gq;


    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );

        case -2 :
            return( CH_ERROR );
	}

	stop = start+linelength;

	if ( comgetflags() == 0 ) {	/* started */
	    comsetflags( 1 );

	    for ( p = start; p < stop; p++ ) {
		if ( *p == ':' ) {
		    break;
		}
	    }

	    if (p < stop) for ( p++; p < stop; p++ ) {
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
		if (( strlen( gq->gq_name ) == (size_t)(q - p) ) &&
			( strncmp( gq->gq_name, p, q - p ) == 0 )) {
		    break;
		}
	    }
	    if ( gq->gq_name == NULL || gq->gq_handler == NULL ||
		    (gq->gq_handler)( out ) < 0 ) {
		if ( comswitch( queries, cq_default ) < 0 ) {
		    LOG(log_error, logtype_papd, "cq_feature: can't find default!" );
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

void cq_font_answer( char *start, char *stop, struct papfile *out)
{
    char		*p, *q, buf[ 256 ];
    struct ppd_font	*pfo;

    p = start;
    while ( p < stop ) {
        unsigned int count = 0;
	while (( *p == ' ' || *p == '\t' ) && p < stop ) {
	    p++;
	}

	q = buf;
	while ( *p != ' ' && *p != '\t' &&
		*p != '\n' && *p != '\r' && p < stop && count < sizeof(buf)) {
	    *q++ = *p++;
	    count++;
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

int cq_font(struct papfile *in, struct papfile *out)
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    struct papd_comment	*comment = compeek();

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );

        case -2 :
            return( CH_ERROR );
	}

	stop = start + linelength;

	if ( comgetflags() == 0 ) {
	    comsetflags( 1 );

	    for ( p = start; p < stop; p++ ) {
		if ( *p == ':' ) {
		    break;
		}
	    }
	    if (p < stop)
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
		if (p < stop)
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

int cq_feature( struct papfile *in, struct papfile *out)
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    struct papd_comment	*comment = compeek();
    struct ppd_feature	*pfe;

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );

        case -2 :
            return( CH_ERROR );
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
	    if (p < stop)
	        p++;
	    while ( *p == ' ' ) {
		p++;
	    }

	    if (( pfe = ppd_feature( p, stop - p )) == NULL ) {
		if ( comswitch( queries, cq_default ) < 0 ) {
		    LOG(log_error, logtype_papd, "cq_feature: can't find default!" );
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

int cq_printer(struct papfile *in, struct papfile *out)
{
    char		*start, *p;
    int			linelength, crlflength;
    struct papd_comment	*comment = compeek();
    struct ppd_feature	*pdpsver, *pdprod;

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    return( 0 );

	case -1 :
	    return( CH_MORE );

        case -2 :
            return( CH_ERROR );
	}

	if ( comgetflags() == 0 ) {
	    comsetflags( 1 );

	    if (( pdpsver = ppd_feature( psver, strlen( psver ))) == NULL ) {
		if ( comswitch( queries, cq_default ) < 0 ) {
		    LOG(log_error, logtype_papd, "cq_printer: can't find default!" );
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
		LOG(log_error, logtype_papd, "cq_printer: can't parse PSVersion!" );
		if ( comswitch( queries, cq_default ) < 0 ) {
		    LOG(log_error, logtype_papd, "cq_printer: can't find default!" );
		    exit( 1 );
		}
		return( CH_DONE );
	    }

	    if (( pdprod = ppd_feature( prod, strlen( prod ))) == NULL ) {
		if ( comswitch( queries, cq_default ) < 0 ) {
		    LOG(log_error, logtype_papd, "cq_printer: can't find default!" );
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

#ifndef HAVE_CUPS

static const char	*rmjobfailed = "Failed\n";
static const char	*rmjobok = "Ok\n";

int cq_rmjob( struct papfile *in, struct papfile *out)
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    int			job;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );

    case -2 :
        return( CH_ERROR );
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

int cq_listq( struct papfile *in, struct papfile *out)
{
    char		*start;
    int			linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );

    case -2 :
        return( CH_ERROR );
    }

    if ( lp_queue( out )) {
	LOG(log_error, logtype_papd, "cq_listq: lp_queue failed" );
    }

    compop();
    CONSUME( in, linelength + crlflength );
    return( CH_DONE );
}
#endif /* HAVE_CUPS */


/*
 * Handler for RBILogin
 */

static struct uam_obj *papd_uam = NULL;
static const char *rbiloginok = "0\r";
static const char *rbiloginbad = "-1\r";
static const char *rbiloginerrstr = "%%[Error: SecurityError; \
SecurityViolation: Unknown user, incorrect password or log on is \
disabled ]%%\r%%[Flushing: rest of job (to end-of-file) will be \
ignored ]%%\r";

int cq_rbilogin( struct papfile *in, struct papfile *out)
{
    char        	*start, *stop, *p, *begin;
    int			linelength, crlflength;
    char        	username[UAM_USERNAMELEN + 1] = "\0";
    struct papd_comment	*comment = compeek();
    char		uamtype[20];

    for (;;) {
        switch ( markline( in, &start, &linelength, &crlflength )) {
        case 0 :
            return( 0 );

        case -1 :
            return( CH_MORE );

        case -2 :
            return( CH_ERROR );
        }

	stop = start + linelength;

        if ( comgetflags() == 0 ) { /* first line */
	    begin = start + strlen(comment->c_begin);
	    p = begin;

	    while (*p != ' ' && p < stop) {
		p++;
	    }

	    memset(uamtype, 0, sizeof(uamtype));
	    if ((size_t)(p -begin) <= sizeof(uamtype) -1) {
	        strncpy(uamtype, begin, p - begin);
            }

	    if ( !*uamtype || (papd_uam = auth_uamfind(UAM_SERVER_PRINTAUTH,
				uamtype, strlen(uamtype))) == NULL) {
		LOG(log_info, logtype_papd, "Could not find uam: %s", uamtype);
		append(out, rbiloginbad, strlen(rbiloginbad));
		append(out, rbiloginerrstr, strlen(rbiloginerrstr));
	    } else {
                if ( (papd_uam->u.uam_printer(p,stop,username,out)) == 0 ) {
                    lp_person( username );
                   append(out, rbiloginok, strlen( rbiloginok ));
                   LOG(log_info, logtype_papd, "RBILogin: Logged in '%s'", username);
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
struct papd_comment	queries[] = {
#ifdef KRB
    { "%%Login: UMICHKerberosIV", NULL,			cq_k4login,	0 },
    { "%%?BeginUAMethodsQuery",	"%%?EndUAMethodsQuery:", cq_uameth,C_FULL },
#endif /* KRB */
#ifndef HAVE_CUPS
    { "%UMICHListQueue",	NULL,			cq_listq,  C_FULL },
    { "%UMICHDeleteJob",	NULL,			cq_rmjob,	0 },
#endif /* HAVE_CUPS */
    { "%%?BeginQuery: RBILogin ", "%%?EndQuery",	cq_rbilogin,	0 },
    { "%%?BeginQuery",		"%%?EndQuery",		cq_query,	0 },
    { "%%?BeginFeatureQuery",	"%%?EndFeatureQuery",	cq_feature,	0 },
    { "%%?BeginFontQuery",	"%%?EndFontQuery",	cq_font,	0 },
    { "%%?BeginPrinterQuery",	"%%?EndPrinterQuery",	cq_printer,C_FULL },
    { "%%?Begin",		"%%?End",		cq_default,	0 },
    { NULL,			NULL,			NULL,		0 },
};
