/*
 * $Id: printcap.h,v 1.5 2009-10-13 22:55:37 didg Exp $
 */

#ifndef PAPD_PRINTCAP_H
#define PAPD_PRINTCAP_H 1

#include <sys/cdefs.h>

int getprent ( register char *, register char *, register int );
int pnchktc ( char * );
int pgetflag ( char * );
void endprent ( void );
int pgetent ( char *, char *, char * );
int pgetnum ( char * );
int pnamatch ( char * );

#endif /* PAPD_PRINTCAP_H */
