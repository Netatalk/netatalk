#ifndef AFPD_CONFIG_H
#define AFPD_CONFIG_H 1

#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/server_child.h>

extern int configinit (AFPObj *, AFPObj *);
extern void configfree (AFPObj *, DSI *);

#endif
