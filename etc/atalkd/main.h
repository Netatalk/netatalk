#ifndef ATALKD_MAIN_H
#define ATALKD_MAIN_H

#include <sys/cdefs.h>

int ifconfig __P(( char *, unsigned long, struct sockaddr_at * ));
void setaddr __P(( struct interface *, u_int8_t, u_int16_t,
        u_int8_t, u_int16_t, u_int16_t ));
void bootaddr __P(( struct interface * ));
void dumpconfig __P(( struct interface * ));

#endif /* ATALKD_MAIN_H */
