/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#include <itc.h>
#include <stdio.h>
#include <string.h>
#include <r/xdr.h>
#include <r/r.h>
#include <afs/comauth.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <afs/cellconfig.h>

#define	DB_PASSWD   0

extern int r_errno;

afs_changepw( ibuf, ibuflen, rbuf, rbuflen )
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    char	cell[ MAXCELLCHARS ], name[ 20 ], oldpw[ 10 ], newpw[ 10 ];
    char	*p;
    int		len;
    u_int16_t	clen;

    len = (unsigned char )*ibuf++;
    ibuf[ len ] = '\0';
    if (( p = strchr( ibuf, '@' )) != NULL ) {
	*p++ = '\0';
	strcpy( cell, p );
	ucase( cell );
    } else {
	if ( GetLocalCellName() != CCONF_SUCCESS ) {
	    *rbuflen = 0;
	    return( AFPERR_BADUAM );
	}
	strcpy( cell, LclCellName );
    }

    if ( strlen( ibuf ) > 20 ) {
	*rbuflen = 0;
	return( AFPERR_PARAM );
    }
    strcpy( name, ibuf );
    ibuf += len;


    if (U_InitRPC() != 0) {
	*rbuflen = 0;
	return( AFPERR_BADUAM );
    }

    memcpy( &clen, ibuf, sizeof( clen ));
    ibuf += sizeof( short );
    pcbc_encrypt((C_Block *)ibuf, (C_Block *)ibuf,
	    clen, seskeysched, seskey, 0 );

    len = (unsigned char) *ibuf++;
    if ( len > 9 ) {
	*rbuflen = 0;
	return( AFPERR_PARAM );
    }
    memcpy( oldpw, ibuf, len );
    oldpw[ len ] = '\0';

    len = (unsigned char) *ibuf++;
    if ( len > 9 ) {
	*rbuflen = 0;
	return( AFPERR_PARAM );
    }
    memcpy( newpw, ibuf, len );
    newpw[ len ] = '\0';

    rc = U_CellChangePassword( name, newpw, name, oldpw, cell ) != 0 ) {

    if ( rc != 0 ) {
	*rbuflen = 0;
	if ( rc < 0 && r_errno = R_ERROR ) {
	    return( AFPERR_NOTAUTH );
	} else {
	    return( AFPERR_BADUAM );
	}
    }

    return( AFP_OK );
}
