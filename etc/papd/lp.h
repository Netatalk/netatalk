#ifndef PAPD_LP_H
#define PAPD_LP_H 1

#include <sys/socket.h>
#include <sys/types.h>
#include "file.h"

void lp_person(char *);
int lp_pagecost(void);
void lp_host(char *);
void lp_job(char *);
void lp_for(char *);
void lp_origin(int);
int lp_rmjob(int);
int lp_queue(struct papfile *);

/*! Cancel the job of the session */
int lp_cancel(void);
/*! Hand the spooled job to the print system */
int lp_print(void);
/*! Open the spool file or the printer pipe of a job */
int lp_open(struct papfile *, struct sockaddr_at *);
/*! Write job data to the spool file or the printer pipe */
int lp_write(struct papfile *, char *, size_t);
/*! Close the spool file or the printer pipe of the job */
int lp_close(void);

#ifndef HAVE_RRESVPORT
extern int rresvport(int *__alport);
#endif

#endif /* PAPD_LP_H */
