#ifndef AFPD_CONFIG_H
#define AFPD_CONFIG_H 1

#include <atalk/server_child.h>
#include <atalk/globals.h>

extern AFPConfig *configinit (struct afp_options *);
extern void configfree (AFPConfig *, const AFPConfig *);

#endif
