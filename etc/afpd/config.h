#ifndef AFPD_CONFIG_H
#define AFPD_CONFIG_H 1

#include <sys/cdefs.h>
#include <atalk/server_child.h>
#include <atalk/atp.h>
#include "globals.h"

typedef struct AFPConfig {
  AFPObj obj;
  int fd, statuslen;
  unsigned char *optcount;
  char status[ATP_MAXDATA];
  const void *defoptions, *signature;
  int (*server_start) __P((struct AFPConfig *, struct AFPConfig *,
			   server_child *));
  void (*server_cleanup) __P((const struct AFPConfig *));
  struct AFPConfig *next;
} AFPConfig;

extern AFPConfig *configinit __P((struct afp_options *));
extern void configfree __P((AFPConfig *, const AFPConfig *));
#endif
