/*
 * $Id: afp_config.h,v 1.5 2009-10-13 22:55:36 didg Exp $
 */

#ifndef AFPD_CONFIG_H
#define AFPD_CONFIG_H 1

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif /* HAVE_SYS_CDEFS_H */

#include <atalk/server_child.h>
#include <atalk/atp.h>
#include <atalk/globals.h>

typedef struct AFPConfig {
    AFPObj obj;
    int fd, statuslen;
    unsigned char *optcount;
    char status[1400];
    const void *defoptions, *signature;
    afp_child_t *(*server_start) (struct AFPConfig *, struct AFPConfig *,
                             server_child *);
    void (*server_cleanup) (const struct AFPConfig *);
    struct AFPConfig *next;
} AFPConfig;

extern AFPConfig *configinit (struct afp_options *);
extern void configfree (AFPConfig *, const AFPConfig *);

#endif
