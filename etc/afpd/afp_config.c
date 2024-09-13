/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>
#endif

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/errchk.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>

#ifdef HAVE_LDAP
#include <atalk/ldapconfig.h>
#endif

#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/server_child.h>
#include <atalk/util.h>

#include "afp_config.h"
#include "uam_auth.h"
#include "status.h"
#include "volume.h"

/*!
 * Free and cleanup config and DSI
 *
 * "dsi" can be NULL in which case all DSI objects and the config object is freed,
 * otherwise its an afpd session child and only any unneeded DSI objects are freed
 */
void configfree(AFPObj *obj, DSI *dsi)
{
#ifndef NO_DDP
    /* just free the asp only resources and get out */
    if (obj->proto == AFPPROTO_ASP)
    {
        obj->Obj = NULL;
        obj->Type = NULL;
        obj->Zone = NULL;
        free(obj->Obj);
        free(obj->Type);
        free(obj->Zone);
	if (obj->handle)
        {
            atp_close(((ASP) (obj->handle))->asp_atp);
            free(obj->handle);
            obj->handle = NULL;
        }
        return;
    }
#endif /* no afp/asp */

    DSI *p, *q;

    if (!dsi) {
        /* Master afpd reloading config */
        auth_unload();
    }

    unload_volumes(obj);

    /* Master and child releasing unneeded DSI handles */
    for (p = obj->dsi; p; p = q) {
        q = p->next;
        if (p == dsi)
            continue;
        dsi_free(p);
        free(p);
    }
    obj->dsi = NULL;

    /* afpd session child passes dsi handle to obj handle */
    if (dsi) {
        dsi->next = NULL;
        obj->dsi = dsi;
    }
}


/*!
 * Get everything running
 */
int configinit(AFPObj *dsi_obj, AFPObj *asp_obj)
{
    EC_INIT;
    DSI *dsi = NULL;
    DSI **next = &dsi_obj->dsi;
    char *p = NULL, *q = NULL, *savep;
    const char *r;
    struct ifaddrs *ifaddr _U_, *ifa _U_;
    int family _U_, s _U_;
    static char interfaddr[NI_MAXHOST] _U_;

    auth_load(dsi_obj, dsi_obj->options.uampath, dsi_obj->options.uamlist);
    set_signature(&dsi_obj->options);
    dsi_obj->proto = AFPPROTO_DSI;

#ifdef HAVE_LDAP
    acl_ldap_freeconfig();
#endif /* HAVE_LDAP */

#ifndef NO_DDP
    /* make sure the asp socket doesn't get opened if init isn't successful */
    asp_obj->fd = -1;
    /* if appletalk is enabled, set it up first */
    if (asp_obj->options.flags & OPTION_DDP)
    {
        ATP atp;
        ASP asp;
        char* Obj, * Type = "AFPServer", * Zone = "*";
        char* convname = NULL;

        if ((atp = atp_open(ATADDR_ANYPORT, &asp_obj->options.ddpaddr)) == NULL) {
            LOG(log_error, logtype_afpd, "main: atp_open: %s", strerror(errno));
            goto serv_free_return;
        }

        if ((asp = asp_init(atp)) == NULL) {
            LOG(log_error, logtype_afpd, "main: asp_init: %s", strerror(errno));
            atp_close(atp);
            goto serv_free_return;
        }

        /* register asp server */
        Obj = (char*)asp_obj->options.hostname;

        if (asp_obj->options.zone && (size_t)-1 == (convert_string_allocate(asp_obj->options.unixcharset, asp_obj->options.maccharset,
            asp_obj->options.zone, strlen(asp_obj->options.zone), &convname))) {
            if ((convname = strdup(asp_obj->options.zone)) == NULL) {
                LOG(log_error, logtype_afpd, "malloc: %s", strerror(errno));
                asp_close(asp);
                goto serv_free_return;
            }
        }

        /* set a custom zone if user requested one */
        if (asp_obj->options.zone) {
            Zone = strdup(convname);
        }

        if (convname)
            free(convname);

        /* dup Obj, Type and Zone as they get assigned to a single internal
         * buffer by nbp_name */
        if ((asp_obj->Obj = strdup(Obj)) == NULL) {
            asp_close(asp);
            goto serv_free_return;
        }

        if ((asp_obj->Type = strdup(Type)) == NULL) {
            free(asp_obj->Obj);
            asp_close(asp);
            goto serv_free_return;
        }

        if ((asp_obj->Zone = strdup(Zone)) == NULL) {
            free(asp_obj->Obj);
            free(asp_obj->Type);
            asp_close(asp);
            goto serv_free_return;
        }

        /* make sure we're not registered */
        nbp_unrgstr(Obj, Type, Zone, &asp_obj->options.ddpaddr);
        if (nbp_rgstr(atp_sockaddr(atp), Obj, Type, Zone) < 0) {
            LOG(log_error, logtype_afpd, "Can't register %s:%s@%s", Obj, Type, Zone);
            free(asp_obj->Obj);
            free(asp_obj->Type);
            free(asp_obj->Zone);
            asp_close(asp);
            goto serv_free_return;
        }

        set_signature(&asp_obj->options);
        asp_obj->proto = AFPPROTO_ASP;
        asp_obj->fd = atp_fileno(atp);
        asp_obj->handle = asp;

    serv_free_return:
        if (asp_obj->handle)
        {
            LOG(log_note, logtype_afpd, "%s:%s@%s started on %u.%u:%u (%s)", Obj, Type, Zone,
                ntohs(atp_sockaddr(atp)->sat_addr.s_net),
                atp_sockaddr(atp)->sat_addr.s_node,
                atp_sockaddr(atp)->sat_port, VERSION);
        }
        else
        {
            LOG(log_note, logtype_afpd, "AppleTalk support disabled. Is atalkd running?");
        }

    }
#endif /* no afp/asp */

    LOG(log_debug, logtype_afpd, "DSIConfigInit: hostname: %s, listen: %s, interfaces: %s, port: %s",
        dsi_obj->options.hostname,
        dsi_obj->options.listen ? dsi_obj->options.listen : "-",
        dsi_obj->options.interfaces ? dsi_obj->options.interfaces : "-",
        dsi_obj->options.port);

    /*
     * Setup addresses we listen on from hostname and/or "afp listen" option
     */
    if (dsi_obj->options.listen) {
        EC_NULL( q = p = strdup(dsi_obj->options.listen) );
        EC_NULL( p = strtok_r(p, ", ", &savep) );
        while (p) {
            if ((dsi = dsi_init(dsi_obj, dsi_obj->options.hostname, p, dsi_obj->options.port)) == NULL)
                break;

            status_init(dsi_obj, asp_obj, dsi);
            *next = dsi;
            next = &dsi->next;
            dsi->AFPobj = dsi_obj;

            LOG(log_note, logtype_afpd, "Netatalk AFP/TCP listening on %s:%d",
                getip_string((struct sockaddr *)&dsi->server),
                getip_port((struct sockaddr *)&dsi->server));

            p = strtok_r(NULL, ", ", &savep);
        }
        if (q) {
            free(q);
            q = NULL;
        }
    }

   /*
    * Setup addresses we listen on from "afp interfaces".
    * We use getifaddrs() instead of if_nameindex() because the latter appears still
    * to be unable to return ipv4 addresses
    */
    if (dsi_obj->options.interfaces) {
#ifndef HAVE_GETIFADDRS
        LOG(log_error, logtype_afpd, "option \"afp interfaces\" not supported");
#else
        if (getifaddrs(&ifaddr) == -1) {
            LOG(log_error, logtype_afpd, "getinterfaddr: getifaddrs() failed: %s", strerror(errno));
            EC_FAIL;
        }

        EC_NULL( q = p = strdup(dsi_obj->options.interfaces) );
        EC_NULL( p = strtok_r(p, ", ", &savep) );
        while (p) {
            for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL)
                    continue;
                if (STRCMP(ifa->ifa_name, !=, p))
                    continue;

                family = ifa->ifa_addr->sa_family;
                if (family == AF_INET || family == AF_INET6) {
                    if (getnameinfo(ifa->ifa_addr,
                                    (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                                    interfaddr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) != 0) {
                        LOG(log_error, logtype_afpd, "getinterfaddr: getnameinfo() failed %s", gai_strerror(errno));
                        continue;
                    }

                    if ((dsi = dsi_init(dsi_obj, dsi_obj->options.hostname, interfaddr, dsi_obj->options.port)) == NULL)
                        continue;

                    status_init(dsi_obj, asp_obj, dsi);
                    *next = dsi;
                    next = &dsi->next;
                    dsi->AFPobj = dsi_obj;

                    LOG(log_note, logtype_afpd, "Netatalk AFP/TCP listening on interface %s with address %s:%d",
                        p,
                        getip_string((struct sockaddr *)&dsi->server),
                        getip_port((struct sockaddr *)&dsi->server));
                } /* if (family == AF_INET || family == AF_INET6) */
            } /* for (ifa != NULL) */
            p = strtok_r(NULL, ", ", &savep);
        }
        freeifaddrs(ifaddr);
#endif
    }

    /*
     * Check whether we got a valid DSI from options.listen or options.interfaces,
     * if not add a DSI that accepts all connections and goes though the list of
     * network interaces for determining an IP we can advertise in DSIStatus
     */
    if (dsi == NULL) {
        if ((dsi = dsi_init(dsi_obj, dsi_obj->options.hostname, NULL, dsi_obj->options.port)) == NULL)
            EC_FAIL_LOG("no suitable network address found, use \"afp listen\" or \"afp interfaces\"", 0);
        status_init(dsi_obj, asp_obj, dsi);
        *next = dsi;
        next = &dsi->next;
        dsi->AFPobj = dsi_obj;

        LOG(log_note, logtype_afpd, "Netatalk AFP/TCP listening on %s:%d",
            getip_string((struct sockaddr *)&dsi->server),
            getip_port((struct sockaddr *)&dsi->server));
    }

#ifdef HAVE_LDAP
    /* Parse afp.conf */
    acl_ldap_readconfig(dsi_obj->iniconfig);
#endif /* HAVE_LDAP */

    if ((r = atalk_iniparser_getstring(dsi_obj->iniconfig, INISEC_GLOBAL, "fce listener", NULL))) {
		LOG(log_note, logtype_afpd, "Adding FCE listener: %s", r);
		fce_add_udp_socket(r);
    }
    if ((r = atalk_iniparser_getstring(dsi_obj->iniconfig, INISEC_GLOBAL, "fce coalesce", NULL))) {
		LOG(log_note, logtype_afpd, "Fce coalesce: %s", r);
		fce_set_coalesce(r);
    }
    if ((r = atalk_iniparser_getstring(dsi_obj->iniconfig, INISEC_GLOBAL, "fce events", NULL))) {
		LOG(log_note, logtype_afpd, "Fce events: %s", r);
		fce_set_events(r);
    }
    r = atalk_iniparser_getstring(dsi_obj->iniconfig, INISEC_GLOBAL, "fce version", "1");
    LOG(log_debug, logtype_afpd, "Fce version: %s", r);
    dsi_obj->fce_version = atoi(r);

    if ((r = atalk_iniparser_getstring(dsi_obj->iniconfig, INISEC_GLOBAL, "fce ignore names", ".DS_Store"))) {
        dsi_obj->fce_ign_names = strdup(r);
    }
    if ((r = atalk_iniparser_getstring(dsi_obj->iniconfig, INISEC_GLOBAL, "fce ignore directories", NULL))) {
            dsi_obj->fce_ign_directories = strdup(r);
    }

    if ((r = atalk_iniparser_getstring(dsi_obj->iniconfig, INISEC_GLOBAL, "fce notify script", NULL))) {
        dsi_obj->fce_notify_script = strdup(r);
    }



EC_CLEANUP:
    if (q)
        free(q);
    EC_EXIT;
}
