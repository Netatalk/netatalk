#ifndef AFPD_MISC_H
#define AFPD_MISC_H 1

#include <atalk/globals.h>

/* FP functions */
/* messages.c */
int afp_getsrvrmesg(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                    size_t *rbuflen);

#define afp_getdiracl	NULL
#define afp_setdiracl	NULL
#define afp_afschangepw	NULL

#endif
