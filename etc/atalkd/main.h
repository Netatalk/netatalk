/*
 * $Id: main.h,v 1.5 2009-10-13 22:55:37 didg Exp $
 */

#ifndef ATALKD_MAIN_H
#define ATALKD_MAIN_H

#include <sys/cdefs.h>
#include "config.h"

int ifconfig ( const char *, unsigned long, struct sockaddr_at * );
void setaddr ( struct interface *, u_int8_t, u_int16_t,
        u_int8_t, u_int16_t, u_int16_t );
void bootaddr ( struct interface * );
void dumpconfig ( struct interface * );

#ifdef linux
int ifsetallmulti ( const char *, int);
#endif

#endif /* ATALKD_MAIN_H */
