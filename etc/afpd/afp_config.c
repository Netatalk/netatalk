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
#include <atalk/errchk.h>

#ifdef HAVE_LDAP
#include <atalk/ldapconfig.h>
#endif

#include "afp_config.h"
#include "uam_auth.h"
#include "status.h"
#include "volume.h"
#include "afp_zeroconf.h"


/*!
 * Free and cleanup all linked DSI objects from config
 *
 * Preserve object pointed to by "dsi".
 * "dsi" can be NULL in which case all DSI objects are freed
 */
void configfree(AFPObj *obj, DSI *dsi)
{
    DSI *p, *q;

    afp_options_free(&obj->options);

    for (p = obj->dsi; p; p = q) {
        q = p->next;
        if (p == dsi)
            continue;
        close(p->socket);
        free(p);
    }
    if (dsi) {
        dsi->next = NULL;
        obj->dsi = dsi;
    }
    /* the master loaded the volumes for zeroconf, get rid of that */
    unload_volumes();
}

/*!
 * Get everything running
 */
int configinit(AFPObj *obj)
{
    EC_INIT;
    DSI *dsi, **next = &obj->dsi;
    char *p, *q = NULL;

    LOG(log_debug, logtype_afpd, "DSIConfigInit: hostname: %s, listen: %s, port: %s",
        obj->options.hostname,
        obj->options.listen ? obj->options.listen : "(default: hostname)",
        obj->options.port);

    /* obj->options->listen is of the from "IP[:port][,IP:[PORT], ...]" */
    /* obj->options->port is the default port to listen (548) */

    EC_NULL( q = p = strdup(obj->options.listen) );
    EC_NULL( p = strtok(p, ",") );

    while (p) {
        if ((dsi = dsi_init(obj, obj->options.hostname, p, obj->options.port)) == NULL)
            break;

        status_init(obj, dsi);
        *next = dsi;
        next = &dsi->next;

        LOG(log_note, logtype_afpd, "Netatalk AFP/TCP listening on %s:%d",
            getip_string((struct sockaddr *)&dsi->server),
            getip_port((struct sockaddr *)&dsi->server));

        p = strtok(NULL, ",");
    }

    if (obj->dsi == NULL)
        EC_FAIL;

    auth_load(obj->options.uampath, obj->options.uamlist);
    set_signature(&obj->options);

#ifdef HAVE_LDAP
    /* Parse afp_ldap.conf */
    acl_ldap_readconfig(obj->iniconfig);
#endif /* HAVE_LDAP */

    /* Now register with zeroconf, we also need the volumes for that */
    if (! (obj->options.flags & OPTION_NOZEROCONF)) {
        load_volumes(obj);
        zeroconf_register(obj);
    }

EC_CLEANUP:
    if (q)
        free(q);
    EC_EXIT;
}
