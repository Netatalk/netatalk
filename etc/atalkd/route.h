/*
 * $Id: route.h,v 1.2 2001-06-19 18:04:39 rufustfirefly Exp $
 */

#ifndef ATALKD_ROUTE_H
#define ATALKD_ROUTE_H 1

#include <sys/cdefs.h>

#ifndef BSD4_4
int route __P(( int, struct sockaddr *, struct sockaddr *, int ));
#else /* BSD4_4 */
int route __P(( int, struct sockaddr_at *, struct sockaddr_at *, int));
#endif /* BSD4_4 */

#endif /* ATALKD_ROUTE_H */
