/*
 * $Id: afp_config.h,v 1.4 2005-04-28 20:49:39 bfernhomberg Exp $
 */

#ifndef AFPD_CONFIG_H
#define AFPD_CONFIG_H 1

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif /* HAVE_SYS_CDEFS_H */

#include <atalk/server_child.h>
#include <atalk/atp.h>
#include "globals.h"

typedef struct AFPConfig {
    AFPObj obj;
    int fd, statuslen;
    unsigned char *optcount;
    char status[1400];
    const void *defoptions, *signature;
    int (*server_start) __P((struct AFPConfig *, struct AFPConfig *,
                             server_child *));
    void (*server_cleanup) __P((const struct AFPConfig *));
    struct AFPConfig *next;
} AFPConfig;

extern AFPConfig *configinit __P((struct afp_options *));
extern void configfree __P((AFPConfig *, const AFPConfig *));

#endif
