#ifndef PAPD_LP_H
#define PAPD_LP_H 1

#include <sys/socket.h>
#include "file.h"

void lp_person __P(( char * ));
int lp_pagecost __P(( void ));
void lp_host __P(( char * ));
void lp_job __P(( char * ));
int lp_rmjob __P(( int ));
int lp_queue __P(( struct papfile * ));

/* initialize printing interface */
int lp_init __P(( struct papfile *, struct sockaddr_at * ));
/* cancel current job */
int lp_cancel __P(( void ));
/* print current job */
int lp_print __P(( void ));
/* open a file for spooling */
int lp_open __P(( struct papfile *, struct sockaddr_at * ));
/* open a buffer to the current open file */
int lp_write __P(( char *, int ));
/* close current spooling file */
int lp_close __P(( void ));

#endif /* PAPD_LP_H */
