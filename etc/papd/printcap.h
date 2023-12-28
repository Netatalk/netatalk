#ifndef PAPD_PRINTCAP_H
#define PAPD_PRINTCAP_H 1

#include <sys/types.h>

int getprent ( register char *, register char *, register int );
int pnchktc ( char * );
int pgetflag ( char * );
void endprent ( void );
int pgetent ( char *, char *, char * );
int pgetnum ( char * );
int pnamatch ( char * );

#endif /* PAPD_PRINTCAP_H */
