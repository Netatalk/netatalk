/*
 * $Id: psorder.h,v 1.3 2009-10-13 22:55:36 didg Exp $
 *
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

#ifndef	STDIN
#	define	STDIN	"-"
#endif /* ! STDIN */
#ifndef FALSE
#	define	FALSE	0
#	define	TRUE	1
#endif /* ! FALSE */

#define REVCHAR		'd'
#define FORWCHAR	'u'
#define FORCECHAR	'f'
#define OPTSTR		"duf"

#define WHITESPACE	" \t"
#define ATEND		"(atend)"
#define PPSADOBE	"%!PS-Adobe-"
#define	PPAGE		"%%Page:"
#define PPAGES		"%%Pages:"
#define PTRAILER	"%%Trailer"
#define PBEGINDOC	"%%BeginDocument:"
#define PENDDOC		"%%EndDocument"
#define PBEGINBIN	"%%BeginBinary:"
#define PENDBIN		"%%EndBinary"

#define REWIND		0L
#define REVERSE		1
#define FORWARD		2

#define LABELLEN	32
#define ORDLEN		4
struct pspage_st {
    struct pspage_st	*nextpage;
    struct pspage_st	*prevpage;
    off_t		offset;
    char		lable[ LABELLEN ];
    char		ord[ ORDLEN ];
};

#define NUMLEN		10
#define ORDERLEN	3
struct pages_st {
    off_t		offset;
    off_t		end;
    char		num[ NUMLEN ];
    char		order[ ORDERLEN ];
};

struct psinfo_st {
    struct pspage_st	*firstpage;
    struct pspage_st	*lastpage;
    off_t		trailer;
    struct pages_st	pages;
};

