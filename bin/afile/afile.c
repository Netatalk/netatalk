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
 * program afile
 *
 *
 * show creator/type of netatalk file
 *  
 * usage  afile [-a] file ....
 * 
 * looks for MAGIC at start to see if an AppleDouble else assumes it's a
 * data fork and only reports if corresponding  .AppleDouble exists unless -a option set
 * in which case report unknown if not a directory (does not look for
 * extension mappings)
 *
 * returns exit status 0 if all files have corresponding readable  valid .AppleDouble or data fork
 *
 * if both parts of file don't exist in correct form it returns (for last file): 
 *
 *	1  file doesn't exist
 *	2  file is unreadable
 *	3  file is directory
 *	4  file is AppleDouble without data fork
 *      5  file is AppleDouble with unreadable data fork
 * 	6  file is data fork without AppleDouble
 * 	7  file is data fork with unreadable AppleDouble
 *	8  file is data for with short  AppleDouble
 * 	9  bad magic in AppleDouble
 *     99  bad command line options
 *	
 * by gerry.tomlinson@ncl.ac.uk
 * last update 26/2/98
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/in.h>
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
    char adbuf [AD_DATASZ];
    struct adouble  *ad;
    int datafork;
    char *dname;
    char *rname;
    mode_t dmode;
    mode_t rmode;
    char *slash;
    char c;
    char *type = 0;
    char *creator = 0;
    char *p;
    int errflag = 0;
    int aflag = 0;
    int status = 0;
    int gotdata;

    while ((c = getopt(argc, argv, "a")) != -1)
    switch (c) {
	case 'a':
		aflag = 1; break;
	case '?' :
		errflag = 1; break;
    }

    if (errflag ||  argc == optind) { 
	  fprintf(stderr,"usage: afile [-a] file ...\n");
	   exit(99);
    }

    
    for (; optind < argc; optind++) {
        close (fdata);
        close (fres);

	dname = argv[optind];
	rname = 0;
	datafork = 1;	/* assume file is a data fork */
	gotdata = 1;    /* assume data fork ok */

	if (stat(dname, &statbuf)) {
	    fprintf(stderr,"afile: %s does not exist\n",dname);
	    status = 1;
	    continue;
	}

        if ((fdata = open(dname,O_RDONLY,0)) == -1) {
	    fprintf(stderr,"afile: cannot open %s\n",dname);
	    status = 2;
            continue;
 	}
	 
	fstat(fdata, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) { 
	     if (aflag) 
		  printf("directory %s\n",dname);
	     status = 3; 
	     continue;
	}
	
	if ( read(fdata,adbuf,AD_DATASZ)  == AD_DATASZ) {
            ad = (struct adouble*)adbuf;
	    if  (ad->ad_magic == ntohl(AD_MAGIC)) {  /*  AppleDouble Header */
		datafork = 0;
		rmode = statbuf.st_mode;
		rname = dname;
		dname = calloc(strlen(rname)+4,1);
		slash = rindex(rname,'/');
		if (!slash) {
		    strcpy(dname,"../");
		    strcat(dname,rname);
		}
		else {
		    strncpy(dname, rname, slash + 1 - rname);
	            strcat(dname,"../");
		    strcat(dname,slash+1);
		}
	        if (stat(dname, &statbuf)) {
		    status = 4;		/* data fork doesn't exist */
		    gotdata = 0;		    
		} else if ((fdata = open(dname,O_RDONLY,0)) == -1) { 	    
		    status = 5; 	/* can't open data fork for reading */
		    gotdata = 0;
		}
		dmode = statbuf.st_mode;	       
	    }	 
        }

	if (datafork)	{
	    dmode = statbuf.st_mode;
	    rname = calloc(strlen(dname)+strlen(AD)+2, 1);
	    slash = rindex(dname,'/');
	    strncpy(rname,dname,slash ? slash + 1 - dname : 0);
	    strcat(rname,AD);
	    strcat(rname,"/");
	    strcat(rname,slash ? slash+1 : dname);
        
	    if (stat(rname, &statbuf)) {
		if (aflag)
		     printf("unknown %s\n",dname); 
		else
		    status = 6;
		continue;
	    }
	    rmode = statbuf.st_mode;
	    if ((fres = open(rname, O_RDONLY,0)) == -1) {
		    fprintf(stderr,"afile: cannot read   %s\n",rname);
		status = 7;
		continue;
	     }
	    if (read(fres,adbuf,AD_DATASZ)  != AD_DATASZ) {
		 fprintf(stderr,"afile: %s too small\n",rname);
		 status = 8;
		 continue;
	    }
	  
	    ad = (struct adouble*)adbuf;
 	}
	
	if  (ad->ad_magic != ntohl(AD_MAGIC)) {
	    fprintf(stderr,"afile: bad MAGIC in %s\n",rname);
	    status = 9;
	    continue;
	}

	printf("%4.4s %4.4s %s\n",adbuf+ADEDOFF_FINDERI, adbuf+ADEDOFF_FINDERI+4, 
		datafork ? dname : rname);

	if (!gotdata) {
	    if (status == 4)
		fprintf(stderr,"afile: %s does not exist\n",dname);			   
	    if (status == 5)	     
		fprintf(stderr,"afile: cannot read %s\n",dname);
	} else if (dmode != rmode) {
	    fprintf(stderr,"afile: %s mode does not match\n",datafork ? rname : dname);
	    status = 0;
	} 

    }
    exit (status);
}

/* end of program afile */
