#ifndef ATALKD_MAIN_H
#define ATALKD_MAIN_H

#define _PATH_ATALKDTMP		"atalkd.tmp"

#include <sys/types.h>
#include "config.h"

extern int transition;
extern int stabletimer, newrtmpdata;

int ifconfig(const char *, unsigned long, struct sockaddr_at *);
void setaddr(struct interface *, uint8_t, uint16_t,
             uint8_t, uint16_t, uint16_t);
void bootaddr(struct interface *);
void dumpconfig(struct interface *);

#endif /* ATALKD_MAIN_H */
