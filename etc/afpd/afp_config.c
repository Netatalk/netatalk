/*
 * $Id: afp_config.c,v 1.15 2002-01-19 21:29:55 jmarcus Exp $
 *
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

/* STDC check */
#if STDC_HEADERS
#include <string.h>
#else /* STDC_HEADERS */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr index
#endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif /* STDC_HEADERS */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <ctype.h>
#include <atalk/logger.h>

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
#ifdef USE_SRVLOC
#include <slp.h>
static char srvloc_url[512];
#endif /* USE_SRVLOC */

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

#ifdef USE_SRVLOC
static void SRVLOC_callback(SLPHandle hslp, SLPError errcode, void *cookie) {
    *(SLPError*)cookie = errcode;
}
#endif /* USE_SRVLOC */

#ifdef USE_SRVLOC
static void dsi_cleanup(const AFPConfig *config)
{
    SLPError err;
    SLPError callbackerr;
    SLPHandle hslp;
    err = SLPOpen("en", SLP_FALSE, &hslp);
    if (err != SLP_OK) {
        LOG(log_error, logtype_default, "dsi_cleanup: Error opening SRVLOC handle");
        goto srvloc_dereg_err;
    }

    err = SLPDereg(hslp,
                   srvloc_url,
                   SRVLOC_callback,
                   &callbackerr);
    if (err != SLP_OK) {
        LOG(log_error, logtype_default, "dsi_cleanup: Error unregistering %s from SRVLOC", srvloc_url);
        goto srvloc_dereg_err;
    }

    if (callbackerr != SLP_OK) {
        LOG(log_error, logtype_default, "dsi_cleanup: Error in callback while trying to unregister %s from SRVLOC (%d)", srvloc_url, callbackerr);
        goto srvloc_dereg_err;
    }

srvloc_dereg_err:
    SLPClose(hslp);
}
#endif /* USE_SRVLOC */

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
        LOG(log_error, logtype_default, "main: asp_getsession: %s", strerror(errno) );
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
        LOG(log_error, logtype_default, "main: dsi_getsession: %s", strerror(errno) );
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
        LOG(log_error, logtype_default, "main: atp_open: %s", strerror(errno) );
        free(config);
        return NULL;
    }

    if ((asp = asp_init( atp )) == NULL) {
        LOG(log_error, logtype_default, "main: asp_init: %s", strerror(errno) );
        atp_close(atp);
        free(config);
        return NULL;
    }

    /* register asp server */
    Obj = (char *) options->hostname;
    if (nbp_name(options->server, &Obj, &Type, &Zone )) {
        LOG(log_error, logtype_default, "main: can't parse %s", options->server );
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
        LOG(log_error, logtype_default, "Can't register %s:%s@%s", Obj, Type, Zone );
        free(config->obj.Obj);
        free(config->obj.Type);
        free(config->obj.Zone);
        goto serv_free_return;
    }

    LOG(log_info, logtype_default, "%s:%s@%s started on %u.%u:%u (%s)", Obj, Type, Zone,
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
#ifdef USE_SRVLOC
    SLPError err;
    SLPError callbackerr;
    SLPHandle hslp;
    struct servent *afpovertcp;
#endif /* USE_SRVLOC */

    if ((config = (AFPConfig *) calloc(1, sizeof(AFPConfig))) == NULL) {
        LOG(log_error, logtype_default, "DSIConfigInit: malloc(config): %s", strerror(errno) );
        return NULL;
    }

    if ((dsi = dsi_init(protocol, "afpd", options->hostname,
                        options->ipaddr, options->port,
                        options->flags & OPTION_PROXY,
                        options->server_quantum)) == NULL) {
        LOG(log_error, logtype_default, "main: dsi_init: %s", strerror(errno) );
        free(config);
        return NULL;
    }

    if (options->flags & OPTION_PROXY) {
        LOG(log_info, logtype_default, "ASIP proxy initialized for %s:%d (%s)",
            inet_ntoa(dsi->server.sin_addr), ntohs(dsi->server.sin_port),
            VERSION);
    } else {
        LOG(log_info, logtype_default, "ASIP started on %s:%d(%d) (%s)",
            inet_ntoa(dsi->server.sin_addr), ntohs(dsi->server.sin_port),
            dsi->serversock, VERSION);
    }

#ifdef USE_SRVLOC
    err = SLPOpen("en", SLP_FALSE, &hslp);
    if (err != SLP_OK) {
        LOG(log_error, logtype_default, "DSIConfigInit: Error opening SRVLOC handle");
        goto srvloc_reg_err;
    }

    /* XXX We don't want to tack on the port number if we don't have to.  Why?
     * Well, this seems to break MacOS < 10.  If the user _really_ wants to
     * use a non-default port, they can, but be aware, this server might not
     * show up int the Network Browser. */
    afpovertcp = getservbyname("afpovertcp", "tcp");
    if (strlen(options->hostname) > (sizeof(srvloc_url) - strlen(inet_ntoa(dsi->server.sin_addr)) - 21)) {
        LOG(log_error, logtype_default, "DSIConfigInit: Hostname is too long for SRVLOC");
        goto srvloc_reg_err;
    }
    if (dsi->server.sin_port == afpovertcp->s_port) {
        sprintf(srvloc_url, "afp://%s/?NAME=%s", inet_ntoa(dsi->server.sin_addr), options->hostname);
    }
    else {
        sprintf(srvloc_url, "afp://%s:%d/?NAME=%s", inet_ntoa(dsi->server.sin_addr), ntohs(dsi->server.sin_port), options->hostname);
    }

    err = SLPReg(hslp,
                 srvloc_url,
                 SLP_LIFETIME_MAXIMUM,
                 "",
                 "",
                 SLP_TRUE,
                 SRVLOC_callback,
                 &callbackerr);
    if (err != SLP_OK) {
        LOG(log_error, logtype_default, "DSIConfigInit: Error registering %s with SRVLOC", srvloc_url);
        goto srvloc_reg_err;
    }

    if (callbackerr != SLP_OK) {
        LOG(log_error, logtype_default, "DSIConfigInit: Error in callback trying to register %s with SRVLOC", srvloc_url);
        goto srvloc_reg_err;
    }

    LOG(log_info, logtype_default, "Sucessfully registered %s with SRVLOC", srvloc_url);

srvloc_reg_err:
    SLPClose(hslp);
#endif /* USE_SRVLOC */


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
#ifdef USE_SRVLOC
    config->server_cleanup = dsi_cleanup;
#endif /* USE_SRVLOC */
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
        LOG(log_error, logtype_default, "AFPConfigInit: calloc(refcount): %s", strerror(errno) );
        return NULL;
    }

#ifndef NO_DDP
    /* handle asp transports */
    if ((options->transports & AFPTRANS_DDP) &&
            (config = ASPConfigInit(options, refcount)))
        config->defoptions = defoptions;
#endif /* NO_DDP */

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
