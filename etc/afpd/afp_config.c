/* 
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <syslog.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/nbp.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/server_child.h>

#include "globals.h"
#include "afp_config.h"
#include "uam_auth.h"
#include "status.h"

#define LINESIZE 1024  

/* get rid of unneeded configurations. i use reference counts to deal
 * w/ multiple configs sharing the same afp_options. oh, to dream of
 * garbage collection ... */
void configfree(AFPConfig *configs, const AFPConfig *config)
{
  AFPConfig *p, *q;

  for (p = configs; p; p = q) {
    q = p->next;
    if (p == config)
      continue;

    /* do a little reference counting */
    if (--(*p->optcount) < 1) {
      afp_options_free(&p->obj.options, p->defoptions);
      free(p->optcount);
    }

    switch (p->obj.proto) {
#ifndef NO_DDP
    case AFPPROTO_ASP:
      free(p->obj.Obj);
      free(p->obj.Type);
      free(p->obj.Zone);
      atp_close(((ASP) p->obj.handle)->asp_atp);
      free(p->obj.handle);
      break;
#endif /* no afp/asp */
    case AFPPROTO_DSI:
      close(p->fd);
      free(p->obj.handle);
      break;
    }
    free(p);
  }
}

#ifndef NO_DDP
static void asp_cleanup(const AFPConfig *config)
{
  nbp_unrgstr(config->obj.Obj, config->obj.Type, config->obj.Zone,
	      &config->obj.options.ddpaddr);
}

/* these two are almost identical. it should be possible to collapse them
 * into one with minimal junk. */
static int asp_start(AFPConfig *config, AFPConfig *configs, 
		     server_child *server_children) 
{
  ASP asp;

  if (!(asp = asp_getsession(config->obj.handle, server_children, 
			     config->obj.options.tickleval))) {
    syslog( LOG_ERR, "main: asp_getsession: %m" );
    exit( 1 );
  } 
  
  if (asp->child) {
    configfree(configs, config); /* free a bunch of stuff */
    afp_over_asp(&config->obj);
    exit (0);
  }

  return 0;
}
#endif /* no afp/asp */

static int dsi_start(AFPConfig *config, AFPConfig *configs,
		     server_child *server_children)
{
  DSI *dsi;

  if (!(dsi = dsi_getsession(config->obj.handle, server_children,
			     config->obj.options.tickleval))) {
    syslog( LOG_ERR, "main: dsi_getsession: %m" );
    exit( 1 );
  } 
  
  /* we've forked. */
  if (dsi->child) {
    configfree(configs, config);
    afp_over_dsi(&config->obj); /* start a session */
    exit (0);
  }

  return 0;
}

#ifndef NO_DDP
static AFPConfig *ASPConfigInit(const struct afp_options *options,
				unsigned char *refcount)
{
  AFPConfig *config;
  ATP atp;
  ASP asp;
  char *Obj, *Type = "AFPServer", *Zone = "*";
  
  if ((config = (AFPConfig *) calloc(1, sizeof(AFPConfig))) == NULL)
    return NULL;
  
  if ((atp = atp_open(ATADDR_ANYPORT, &options->ddpaddr)) == NULL)  {
    syslog( LOG_ERR, "main: atp_open: %m");
    free(config);
    return NULL;
  }
  
  if ((asp = asp_init( atp )) == NULL) {
    syslog( LOG_ERR, "main: asp_init: %m" );
    atp_close(atp);
    free(config);
    return NULL;
  }
  
  /* register asp server */
  Obj = (char *) options->hostname;
  if (nbp_name(options->server, &Obj, &Type, &Zone )) {
    syslog( LOG_ERR, "main: can't parse %s", options->server );
    goto serv_free_return;
  }

  /* dup Obj, Type and Zone as they get assigned to a single internal
   * buffer by nbp_name */
  if ((config->obj.Obj  = strdup(Obj)) == NULL) 
    goto serv_free_return;

  if ((config->obj.Type = strdup(Type)) == NULL) {
    free(config->obj.Obj);
    goto serv_free_return;
  }

  if ((config->obj.Zone = strdup(Zone)) == NULL) {
    free(config->obj.Obj);
    free(config->obj.Type);
    goto serv_free_return;
  }
  
  /* make sure we're not registered */
  nbp_unrgstr(Obj, Type, Zone, &options->ddpaddr); 
  if (nbp_rgstr( atp_sockaddr( atp ), Obj, Type, Zone ) < 0 ) {
    syslog( LOG_ERR, "Can't register %s:%s@%s", Obj, Type, Zone );
    free(config->obj.Obj);
    free(config->obj.Type);
    free(config->obj.Zone);
    goto serv_free_return;
  }

  syslog( LOG_INFO, "%s:%s@%s started on %u.%u:%u (%s)", Obj, Type, Zone,
	  ntohs( atp_sockaddr( atp )->sat_addr.s_net ),
	  atp_sockaddr( atp )->sat_addr.s_node,
	  atp_sockaddr( atp )->sat_port, VERSION );
  
  config->fd = atp_fileno(atp);
  config->obj.handle = asp;
  config->obj.config = config;
  config->obj.proto = AFPPROTO_ASP;

  memcpy(&config->obj.options, options, sizeof(struct afp_options));
  config->optcount = refcount;
  (*refcount)++;

  config->server_start = asp_start;
  config->server_cleanup = asp_cleanup;

  return config;

serv_free_return:
  asp_close(asp);
  free(config);
  return NULL;
}
#endif /* no afp/asp */


static AFPConfig *DSIConfigInit(const struct afp_options *options,
				unsigned char *refcount,
				const dsi_proto protocol) 
{
  AFPConfig *config;
  DSI *dsi;
  char *p, *q;

  if ((config = (AFPConfig *) calloc(1, sizeof(AFPConfig))) == NULL) {
    syslog( LOG_ERR, "DSIConfigInit: malloc(config): %m" );
    return NULL;
  }

  if ((dsi = dsi_init(protocol, "afpd", options->hostname,
		      options->ipaddr, options->port, 
		      options->flags & OPTION_PROXY, 
		      options->server_quantum)) == NULL) {
    syslog( LOG_ERR, "main: dsi_init: %m" );
    free(config);
    return NULL;
  }

  if (options->flags & OPTION_PROXY) {
    syslog(LOG_INFO, "ASIP proxy initialized for %s:%d (%s)",
	   inet_ntoa(dsi->server.sin_addr), ntohs(dsi->server.sin_port), 
	   VERSION);
  } else {
    syslog(LOG_INFO, "ASIP started on %s:%d(%d) (%s)",
	   inet_ntoa(dsi->server.sin_addr), ntohs(dsi->server.sin_port), 
	   dsi->serversock, VERSION);
  }

  config->fd = dsi->serversock;
  config->obj.handle = dsi;
  config->obj.config = config;
  config->obj.proto = AFPPROTO_DSI;

  memcpy(&config->obj.options, options, sizeof(struct afp_options));
  /* get rid of any appletalk info. we use the fact that the DSI
   * stuff is done after the ASP stuff. */
  p = config->obj.options.server;
  if (p && (q = strchr(p, ':')))
    *q = '\0';
    
  config->optcount = refcount;
  (*refcount)++;

  config->server_start = dsi_start;
  return config;
}

/* allocate server configurations. this should really store the last
 * entry in config->last or something like that. that would make
 * supporting multiple dsi transports easier. */
static AFPConfig *AFPConfigInit(const struct afp_options *options,
				const struct afp_options *defoptions)
{
  AFPConfig *config = NULL, *next = NULL;
  unsigned char *refcount;

  if ((refcount = (unsigned char *) 
       calloc(1, sizeof(unsigned char))) == NULL) {
    syslog( LOG_ERR, "AFPConfigInit: calloc(refcount): %m" );
    return NULL;
  }

#ifndef NO_DDP
  /* handle asp transports */
  if ((options->transports & AFPTRANS_DDP) && 
      (config = ASPConfigInit(options, refcount)))
    config->defoptions = defoptions;
#endif

  /* handle dsi transports and dsi proxies. we only proxy
   * for DSI connections. */

  /* this should have something like the following:
   * for (i=mindsi; i < maxdsi; i++)
   *   if (options->transports & (1 << i) && 
   *     (next = DSIConfigInit(options, refcount, i)))
   *     next->defoptions = defoptions;
   */
  if ((options->transports & AFPTRANS_TCP) &&
      (((options->flags & OPTION_PROXY) == 0) ||
       ((options->flags & OPTION_PROXY) && config))
      && (next = DSIConfigInit(options, refcount, DSI_TCPIP)))
    next->defoptions = defoptions;

  /* load in all the authentication modules. we can load the same
     things multiple times if necessary. however, loading different
     things with the same names will cause complaints. by not loading
     in any uams with proxies, we prevent ddp connections from succeeding.
  */
  auth_load(options->uampath, options->uamlist);

  /* this should be able to accept multiple dsi transports. i think
   * the only thing that gets affected is the net addresses. */
  status_init(config, next, options);

  /* attach dsi config to tail of asp config */
  if (config) {
    config->next = next;
    return config;
  } 
    
  return next;
}

/* fill in the appropriate bits for each interface */
AFPConfig *configinit(struct afp_options *cmdline)
{
  FILE *fp;
  char buf[LINESIZE + 1], *p, have_option = 0;
  struct afp_options options;
  AFPConfig *config, *first = NULL;

  /* if config file doesn't exist, load defaults */
  if ((fp = fopen(cmdline->configfile, "r")) == NULL) 
    return AFPConfigInit(cmdline, cmdline);

  /* scan in the configuration file */
  while (!feof(fp)) {
    if (!fgets(buf, sizeof(buf), fp) || buf[0] == '#')
      continue;
    
    /* a little pre-processing to get rid of spaces and end-of-lines */
    p = buf;
    while (p && isspace(*p)) 
      p++;
    if (!p || (*p == '\0'))
      continue;

    have_option = 1;

    memcpy(&options, cmdline, sizeof(options));
    if (!afp_options_parseline(p, &options))
      continue;

    /* this should really get a head and a tail to simplify things. */
    if (!first) {
      if ((first = AFPConfigInit(&options, cmdline)))
	config = first->next ? first->next : first;
    } else if ((config->next = AFPConfigInit(&options, cmdline))) {
      config = config->next->next ? config->next->next : config->next;
    }
  }

  fclose(fp);

  if (!have_option)
    return AFPConfigInit(cmdline, cmdline);

  return first;
}
