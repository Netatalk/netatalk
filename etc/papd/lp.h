#ifndef PAPD_LP_H
#define PAPD_LP_H 1

#include <sys/socket.h>
#include <sys/types.h>
#include "file.h"

void lp_person ( char * );
int lp_pagecost ( void );
void lp_host ( char * );
void lp_job ( char * );
void lp_for ( char * );
void lp_origin ( int );
int lp_rmjob ( int );
int lp_queue ( struct papfile * );

/* cancel current job */
int lp_cancel ( void );
/* print current job */
int lp_print ( void );
/* open a file for spooling */
int lp_open ( struct papfile *, struct sockaddr_at * );
/* open a buffer to the current open file */
int lp_write ( struct papfile *,char *, size_t );
/* close current spooling file */
int lp_close ( void );

#ifndef HAVE_RRESVPORT
extern int rresvport (int *__alport);
#endif

#endif /* PAPD_LP_H */
