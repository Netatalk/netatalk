/*
 * Copyright 1998 The University of Newcastle upon Tyne
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose other than its commercial exploitation
 * is hereby granted without fee, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of The University of Newcastle upon Tyne not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. The University of
 * Newcastle upon Tyne makes no representations about the suitability of
 * this software for any purpose. It is provided "as is" without express
 * or implied warranty.
 * 
 * THE UNIVERSITY OF NEWCASTLE UPON TYNE DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF
 * NEWCASTLE UPON TYNE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * 
 * Author:  Gerry Tomlinson (gerry.tomlinson@newcastle.ac.uk)
 *          Computing Science Department, University of Newcastle upon Tyne, UK
 */



/*
 *
 * program achfile
 *
 *
 * change creator/type of netatalk file
 *  
 * usage  achfile [-t type] [-c creator] file ....
 *
 * returns exit status 0 if all files changed successfully
 *
 * by gerry.tomlinson@newcastle.ac.uk 
 * last update 26/2/98
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <atalk/adouble.h>

extern char *optarg;
extern int optind, opterr;

#define AD  ".AppleDouble"


main(int argc, char *argv[])

{
    int fdata, fres;
    struct stat statbuf;
    int n,i;
    char rbuf [AD_DATASZ];
    struct adouble  *ad = (struct adouble*)&rbuf;
    char *dname;
    char *rname;
    char *slash;
    char c;
    char *type = 0;
    char *creator = 0;
    char *p;
    int errflag = 0;
    int status = 0;

    while ((c = getopt(argc, argv, "t:c:")) != -1)
    switch (c) {
	case 't':
		type = optarg;
		if (strlen(type) != 4)
		    {fprintf(stderr,"achfile: bad type length\n");exit(1);}
		break;
	case 'c':
		creator = optarg; 
		if (strlen(creator) != 4)
		    {fprintf(stderr,"achfile: bad creator length\n");exit(1);}
		break;
	case '?' :
		errflag = 1; break;
    }

    if (errflag ||  argc == optind || (!type && !creator) ) 
		{fprintf(stderr,"usage: achfile [-t type] [-c creator]  file ...\n");exit(1);}

    
    for (; optind < argc; optind++) {
        close (fdata);
        close (fres);

	dname = argv[optind];

        if ((fdata = open(dname,O_RDONLY,0)) == -1)
	    {fprintf(stderr,"achfile: can't open %s\n",dname); status = 2; continue;}
 
	fstat(fdata, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) 
	    {status = 2; continue;}
	
	rname = calloc(strlen(dname)+strlen(AD)+2, 1);
	slash = rindex(dname,'/');
	strncpy(rname,dname,slash ? slash + 1 - dname : 0);
	strcat(rname,AD);
	strcat(rname,"/");
	strcat(rname,slash ? slash+1 : dname);
       
	if ((fres = open(rname, O_RDWR,0)) == -1)
	    {fprintf(stderr, "can't open %s\n",rname); status = 2; continue;}
	if ( read(fres,rbuf,AD_DATASZ)  != AD_DATASZ)
	    {fprintf(stderr,"can't read %s\n",rname); status = 2; continue;}
	if  (ad->ad_magic != AD_MAGIC) 
	    {fprintf(stderr,"bad MAGIC in %s\n",rname); status = 2 ; continue;}
  	if (type) 
	    for (i=0,p=type;i<4;i++) *(rbuf + ADEDOFF_FINDERI +i) = *p++;
	if (creator)
	    for (i=0,p=creator;i<4;i++) *(rbuf + ADEDOFF_FINDERI +4 +i) = *p++;
	lseek(fres,0,0);
	if (write(fres,rbuf,AD_DATASZ)  != AD_DATASZ)
	    {fprintf(stderr,"can't update %s\n",rname); status = 3; continue;}
	
    }
    exit (0);
}

/* end of program achfile */
