/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/dsi.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/server_child.h>
#include <atalk/globals.h>

#ifdef HAVE_LDAP
#include <atalk/ldapconfig.h>
#endif

#include "afp_config.h"
#include "uam_auth.h"
#include "status.h"
#include "volume.h"
#include "afp_zeroconf.h"

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

        afp_options_free(&p->obj.options, p->defoptions);

        switch (p->obj.proto) {
        case AFPPROTO_DSI:
            close(p->fd);
            free(p->obj.dsi);
            break;
        }
        free(p);
    }

    /* the master loaded the volumes for zeroconf, get rid of that */
    unload_volumes_and_extmap();
}


static void dsi_cleanup(const AFPConfig *config)
{
    return;
}

static afp_child_t *dsi_start(AFPConfig *config, AFPConfig *configs,
                              server_child *server_children)
{
    DSI *dsi = config->obj.dsi;
    afp_child_t *child = NULL;

    if (!(child = dsi_getsession(dsi,
                                 server_children,
                                 config->obj.options.tickleval))) {
        LOG(log_error, logtype_afpd, "dsi_start: session error: %s", strerror(errno));
        return NULL;
    }

    /* we've forked. */
    if (parent_or_child == 1) {
        configfree(configs, config);
        config->obj.ipc_fd = child->ipc_fds[1];
        close(child->ipc_fds[0]); /* Close parent IPC fd */
        free(child);
        afp_over_dsi(&config->obj); /* start a session */
        exit (0);
    }

    return child;
}

static AFPConfig *DSIConfigInit(const struct afp_options *options,
                                unsigned char *refcount,
                                const dsi_proto protocol)
{
    AFPConfig *config;
    DSI *dsi;
    char *p, *q;

    if ((config = (AFPConfig *) calloc(1, sizeof(AFPConfig))) == NULL) {
        LOG(log_error, logtype_afpd, "DSIConfigInit: malloc(config): %s", strerror(errno) );
        return NULL;
    }

    LOG(log_debug, logtype_afpd, "DSIConfigInit: hostname: %s, ip/port: %s/%s, ",
        options->hostname,
        options->ipaddr ? options->ipaddr : "default",
        options->port ? options->port : "548");

    if ((dsi = dsi_init(protocol, "afpd", options->hostname,
                        options->ipaddr, options->port,
                        0, options->server_quantum)) == NULL) {
        LOG(log_error, logtype_afpd, "main: dsi_init: %s", strerror(errno) );
        free(config);
        return NULL;
    }
    dsi->dsireadbuf = options->dsireadbuf;

    LOG(log_note, logtype_afpd, "AFP/TCP started, advertising %s:%d (%s)",
        getip_string((struct sockaddr *)&dsi->server), getip_port((struct sockaddr *)&dsi->server), VERSION);

    config->dsi = dsi;

    memcpy(&config->obj.options, options, sizeof(struct afp_options));
    /* get rid of any appletalk info. we use the fact that the DSI
     * stuff is done after the ASP stuff. */
    p = config->obj.options.server;
    if (p && (q = strchr(p, ':')))
        *q = '\0';

    return config;
}

/* allocate server configurations. this should really store the last
 * entry in config->last or something like that. that would make
 * supporting multiple dsi transports easier. */
static AFPConfig *AFPConfigInit(struct afp_options *options,
                                const struct afp_options *defoptions)
{
    AFPConfig *next = NULL;
    unsigned char *refcount;

    if ((refcount = (unsigned char *)
                    calloc(1, sizeof(unsigned char))) == NULL) {
        LOG(log_error, logtype_afpd, "AFPConfigInit: calloc(refcount): %s", strerror(errno) );
        return NULL;
    }

    /* set signature */
    set_signature(options);

    if ((next = DSIConfigInit(options, refcount, DSI_TCPIP)))
        /* load in all the authentication modules. we can load the same
           things multiple times if necessary. however, loading different
           things with the same names will cause complaints. by not loading
           in any uams with proxies, we prevent ddp connections from succeeding.
        */
        auth_load(options->uampath, options->uamlist);

    /* this should be able to accept multiple dsi transports. i think
     * the only thing that gets affected is the net addresses. */
    status_init(next, options);

    return next;
}

/*!
 * Get everything running
 */
int configinit(AFPObj *AFPObj)
{
    AFPConfigInit(AFPObj);

#ifdef HAVE_LDAP
    /* Parse afp_ldap.conf */
    acl_ldap_readconfig(AFPObj->iniconfig);
#endif /* HAVE_LDAP */

    /* Now register with zeroconf, we also need the volumes for that */
    if (! (AFPObj->options.flags & OPTION_NOZEROCONF)) {
        load_volumes(AFPObj);
        zeroconf_register(AFPObj);
    }

    return first;
}
