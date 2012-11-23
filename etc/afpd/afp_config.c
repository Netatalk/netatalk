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
#include <atalk/netatalk_conf.h>
#include <atalk/fce_api.h>

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
 * "dsi" can be NULL in which case all DSI objects _and_ the options object are freed 
 */
void configfree(AFPObj *obj, DSI *dsi)
{
    DSI *p, *q;

    /* the master loaded the volumes for zeroconf, get rid of that */
    unload_volumes(obj);

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
    } else {
        afp_options_free(&obj->options);
    }
}

/*!
 * Get everything running
 */
int configinit(AFPObj *obj)
{
    EC_INIT;
    DSI *dsi, **next = &obj->dsi;
    char *p = NULL, *q = NULL, *savep;
    const char *r;

    auth_load(obj->options.uampath, obj->options.uamlist);
    set_signature(&obj->options);

    LOG(log_debug, logtype_afpd, "DSIConfigInit: hostname: %s, listen: %s, port: %s",
        obj->options.hostname,
        obj->options.listen ? obj->options.listen : "(default: hostname)",
        obj->options.port);

    /* obj->options->listen is of the from "IP[:port][,IP:[PORT], ...]" */
    /* obj->options->port is the default port to listen (548) */

    if (obj->options.listen) {
        EC_NULL( q = p = strdup(obj->options.listen) );
        EC_NULL( p = strtok_r(p, ", ", &savep) );
    }

    while (1) {
        if ((dsi = dsi_init(obj, obj->options.hostname, p, obj->options.port)) == NULL)
            break;

        status_init(obj, dsi);
        *next = dsi;
        next = &dsi->next;
        dsi->AFPobj = obj;

        LOG(log_note, logtype_afpd, "Netatalk AFP/TCP listening on %s:%d",
            getip_string((struct sockaddr *)&dsi->server),
            getip_port((struct sockaddr *)&dsi->server));

        if (p)
            /* p is NULL if ! obj->options.listen */
            p = strtok_r(NULL, ", ", &savep);
        if (!p)
            break;
    }

#ifdef HAVE_LDAP
    /* Parse afp.conf */
    acl_ldap_readconfig(obj->iniconfig);
#endif /* HAVE_LDAP */

    /* Now register with zeroconf, we also need the volumes for that */
    if (! (obj->options.flags & OPTION_NOZEROCONF)) {
        load_volumes(obj);
        zeroconf_register(obj);
    }

    if ((r = iniparser_getstring(obj->iniconfig, INISEC_GLOBAL, "fce listener", NULL))) {
		LOG(log_note, logtype_afpd, "Adding FCE listener: %s", r);
		fce_add_udp_socket(r);
    }
    if ((r = iniparser_getstring(obj->iniconfig, INISEC_GLOBAL, "fce coalesce", NULL))) {
		LOG(log_note, logtype_afpd, "Fce coalesce: %s", r);
		fce_set_coalesce(r);
    }
    if ((r = iniparser_getstring(obj->iniconfig, INISEC_GLOBAL, "fce events", NULL))) {
		LOG(log_note, logtype_afpd, "Fce events: %s", r);
		fce_set_events(r);
    }

EC_CLEANUP:
    if (q)
        free(q);
    EC_EXIT;
}
