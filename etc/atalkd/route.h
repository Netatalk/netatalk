#ifndef ATALKD_ROUTE_H
#define ATALKD_ROUTE_H 1

#include <sys/types.h>

#ifndef BSD4_4
int route(int, struct sockaddr *, struct sockaddr *, int);
#else /* BSD4_4 */
int route(int, struct sockaddr_at *, struct sockaddr_at *, int);
#endif /* BSD4_4 */

#endif /* ATALKD_ROUTE_H */
