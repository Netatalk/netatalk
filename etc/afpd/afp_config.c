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

#ifdef USE_SRVLOC
#include <slp.h>
#endif /* USE_SRVLOC */

#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/nbp.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/server_child.h>

#ifdef HAVE_LDAP
#include <atalk/ldapconfig.h>
#endif

#include <atalk/globals.h>
#include "afp_config.h"
#include "uam_auth.h"
#include "status.h"
#include "volume.h"
#include "afp_zeroconf.h"

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

    /* the master loaded the volumes for zeroconf, get rid of that */
    unload_volumes_and_extmap();
}

#ifdef USE_SRVLOC
static void SRVLOC_callback(SLPHandle hslp _U_, SLPError errcode, void *cookie) {
    *(SLPError*)cookie = errcode;
}

static char hex[17] = "0123456789abcdef";

static char * srvloc_encode(const struct afp_options *options, const char *name)
{
	static char buf[512];
	char *conv_name;
	unsigned char *p;
	unsigned int i = 0;
#ifndef NO_DDP
	char *Obj, *Type = "", *Zone = "";
#endif

	/* Convert name to maccharset */
        if ((size_t)-1 ==(convert_string_allocate( options->unixcharset, options->maccharset,
			 name, -1, &conv_name)) )
		return (char*)name;

	/* Escape characters */
	p = conv_name;
	while (*p && i<(sizeof(buf)-4)) {
	    if (*p == '@')
		break;
	    else if (isspace(*p)) {
	        buf[i++] = '%';
           	buf[i++] = '2';
           	buf[i++] = '0';
		p++;
	    }	
	    else if ((!isascii(*p)) || *p <= 0x2f || *p == 0x3f ) {
	        buf[i++] = '%';
           	buf[i++] = hex[*p >> 4];
           	buf[i++] = hex[*p++ & 15];
	    }
	    else {
		buf[i++] = *p++;
	    }
	}
	buf[i] = '\0';

#ifndef NO_DDP
	/* Add ZONE,  */
        if (nbp_name(options->server, &Obj, &Type, &Zone )) {
        	LOG(log_error, logtype_afpd, "srvloc_encode: can't parse %s", options->server );
    	}
	else {
		snprintf( buf+i, sizeof(buf)-i-1 ,"&ZONE=%s", Zone);
	}
#endif
	free (conv_name);

	return buf;
}
#endif /* USE_SRVLOC */

static void dsi_cleanup(const AFPConfig *config)
{
#ifdef USE_SRVLOC
    SLPError err;
    SLPError callbackerr;
    SLPHandle hslp;
    DSI *dsi = (DSI *)config->obj.handle;

    /*  Do nothing if we didn't register.  */
    if (!dsi || dsi->srvloc_url[0] == '\0')
        return;

    err = SLPOpen("en", SLP_FALSE, &hslp);
    if (err != SLP_OK) {
        LOG(log_error, logtype_afpd, "dsi_cleanup: Error opening SRVLOC handle");
        goto srvloc_dereg_err;
    }

    err = SLPDereg(hslp,
                   dsi->srvloc_url,
                   SRVLOC_callback,
                   &callbackerr);
    if (err != SLP_OK) {
        LOG(log_error, logtype_afpd, "dsi_cleanup: Error unregistering %s from SRVLOC", dsi->srvloc_url);
        goto srvloc_dereg_err;
    }

    if (callbackerr != SLP_OK) {
        LOG(log_error, logtype_afpd, "dsi_cleanup: Error in callback while trying to unregister %s from SRVLOC (%d)", dsi->srvloc_url, callbackerr);
        goto srvloc_dereg_err;
    }

srvloc_dereg_err:
    dsi->srvloc_url[0] = '\0';
    SLPClose(hslp);
#endif /* USE_SRVLOC */
}

#ifndef NO_DDP
static void asp_cleanup(const AFPConfig *config)
{
    /* we need to stop tickle handler */
    asp_stop_tickle();
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
        LOG(log_error, logtype_afpd, "main: asp_getsession: %s", strerror(errno) );
        exit( EXITERR_CLNT );
    }

    if (asp->child) {
        configfree(configs, config); /* free a bunch of stuff */
        afp_over_asp(&config->obj);
        exit (0);
    }

    return 0;
}
#endif /* no afp/asp */

static afp_child_t *dsi_start(AFPConfig *config, AFPConfig *configs,
                              server_child *server_children)
{
    DSI *dsi = config->obj.handle;
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

#ifndef NO_DDP
static AFPConfig *ASPConfigInit(const struct afp_options *options,
                                unsigned char *refcount)
{
    AFPConfig *config;
    ATP atp;
    ASP asp;
    char *Obj, *Type = "AFPServer", *Zone = "*";
    char *convname = NULL;

    if ((config = (AFPConfig *) calloc(1, sizeof(AFPConfig))) == NULL)
        return NULL;

    if ((atp = atp_open(ATADDR_ANYPORT, &options->ddpaddr)) == NULL)  {
        LOG(log_error, logtype_afpd, "main: atp_open: %s", strerror(errno) );
        free(config);
        return NULL;
    }

    if ((asp = asp_init( atp )) == NULL) {
        LOG(log_error, logtype_afpd, "main: asp_init: %s", strerror(errno) );
        atp_close(atp);
        free(config);
        return NULL;
    }

    /* register asp server */
    Obj = (char *) options->hostname;
    if (options->server && (size_t)-1 ==(convert_string_allocate( options->unixcharset, options->maccharset,
                         options->server, strlen(options->server), &convname)) ) {
        if ((convname = strdup(options->server)) == NULL ) {
            LOG(log_error, logtype_afpd, "malloc: %s", strerror(errno) );
            goto serv_free_return;
        }
    }

    if (nbp_name(convname, &Obj, &Type, &Zone )) {
        LOG(log_error, logtype_afpd, "main: can't parse %s", options->server );
        goto serv_free_return;
    }
    if (convname)
        free (convname);

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
        LOG(log_error, logtype_afpd, "Can't register %s:%s@%s", Obj, Type, Zone );
        free(config->obj.Obj);
        free(config->obj.Type);
        free(config->obj.Zone);
        goto serv_free_return;
    }

    LOG(log_info, logtype_afpd, "%s:%s@%s started on %u.%u:%u (%s)", Obj, Type, Zone,
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
        LOG(log_error, logtype_afpd, "DSIConfigInit: malloc(config): %s", strerror(errno) );
        return NULL;
    }

    LOG(log_debug, logtype_afpd, "DSIConfigInit: hostname: %s, ip/port: %s/%s, ",
        options->hostname,
        options->ipaddr ? options->ipaddr : "default",
        options->port ? options->port : "548");

    if ((dsi = dsi_init(protocol, "afpd", options->hostname,
                        options->ipaddr, options->port,
                        options->flags & OPTION_PROXY,
                        options->server_quantum)) == NULL) {
        LOG(log_error, logtype_afpd, "main: dsi_init: %s", strerror(errno) );
        free(config);
        return NULL;
    }
    dsi->dsireadbuf = options->dsireadbuf;

    if (options->flags & OPTION_PROXY) {
        LOG(log_note, logtype_afpd, "AFP/TCP proxy initialized for %s:%d (%s)",
            getip_string((struct sockaddr *)&dsi->server), getip_port((struct sockaddr *)&dsi->server), VERSION);
    } else {
        LOG(log_note, logtype_afpd, "AFP/TCP started, advertising %s:%d (%s)",
            getip_string((struct sockaddr *)&dsi->server), getip_port((struct sockaddr *)&dsi->server), VERSION);
    }

#ifdef USE_SRVLOC
    dsi->srvloc_url[0] = '\0';	/*  Mark that we haven't registered.  */
    if (!(options->flags & OPTION_NOSLP)) {
        SLPError err;
        SLPError callbackerr;
        SLPHandle hslp;
        unsigned int afp_port;
        int   l;
        char *srvloc_hostname;
        const char *hostname;

	err = SLPOpen("en", SLP_FALSE, &hslp);
	if (err != SLP_OK) {
	    LOG(log_error, logtype_afpd, "DSIConfigInit: Error opening SRVLOC handle");
	    goto srvloc_reg_err;
	}

	/* XXX We don't want to tack on the port number if we don't have to.
	 * Why?
	 * Well, this seems to break MacOS < 10.  If the user _really_ wants to
	 * use a non-default port, they can, but be aware, this server might
	 * not show up int the Network Browser.
	 */
	afp_port = getip_port((struct sockaddr *)&dsi->server);
	/* If specified use the FQDN to register with srvloc, otherwise use IP. */
	p = NULL;
	if (options->fqdn) {
	    hostname = options->fqdn;
	    p = strchr(hostname, ':');
	}	
	else 
	    hostname = getip_string((struct sockaddr *)&dsi->server);

	srvloc_hostname = srvloc_encode(options, (options->server ? options->server : options->hostname));

	if ((p) || afp_port == 548) {
	    l = snprintf(dsi->srvloc_url, sizeof(dsi->srvloc_url), "afp://%s/?NAME=%s", hostname, srvloc_hostname);
	}
	else {
	    l = snprintf(dsi->srvloc_url, sizeof(dsi->srvloc_url), "afp://%s:%d/?NAME=%s", hostname, afp_port, srvloc_hostname);
	}

	if (l == -1 || l >= (int)sizeof(dsi->srvloc_url)) {
	    LOG(log_error, logtype_afpd, "DSIConfigInit: Hostname is too long for SRVLOC");
	    dsi->srvloc_url[0] = '\0';
	    goto srvloc_reg_err;
	}

	err = SLPReg(hslp,
		     dsi->srvloc_url,
		     SLP_LIFETIME_MAXIMUM,
		     "afp",
		     "",
		     SLP_TRUE,
		     SRVLOC_callback,
		     &callbackerr);
	if (err != SLP_OK) {
	    LOG(log_error, logtype_afpd, "DSIConfigInit: Error registering %s with SRVLOC", dsi->srvloc_url);
	    dsi->srvloc_url[0] = '\0';
	    goto srvloc_reg_err;
	}

	if (callbackerr != SLP_OK) {
	    LOG(log_error, logtype_afpd, "DSIConfigInit: Error in callback trying to register %s with SRVLOC", dsi->srvloc_url);
	    dsi->srvloc_url[0] = '\0';
	    goto srvloc_reg_err;
	}

	LOG(log_info, logtype_afpd, "Sucessfully registered %s with SRVLOC", dsi->srvloc_url);
	config->server_cleanup = dsi_cleanup;

srvloc_reg_err:
	SLPClose(hslp);
    }
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
    return config;
}

/* allocate server configurations. this should really store the last
 * entry in config->last or something like that. that would make
 * supporting multiple dsi transports easier. */
static AFPConfig *AFPConfigInit(struct afp_options *options,
                                const struct afp_options *defoptions)
{
    AFPConfig *config = NULL, *next = NULL;
    unsigned char *refcount;

    if ((refcount = (unsigned char *)
                    calloc(1, sizeof(unsigned char))) == NULL) {
        LOG(log_error, logtype_afpd, "AFPConfigInit: calloc(refcount): %s", strerror(errno) );
        return NULL;
    }

#ifndef NO_DDP
    /* handle asp transports */
    if ((options->transports & AFPTRANS_DDP) &&
            (config = ASPConfigInit(options, refcount)))
        config->defoptions = defoptions;
#endif /* NO_DDP */


    /* set signature */
    set_signature(options);

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
    size_t len;
    struct afp_options options;
    AFPConfig *config=NULL, *first = NULL; 

    /* if config file doesn't exist, load defaults */
    if ((fp = fopen(cmdline->configfile, "r")) == NULL)
    {
        LOG(log_debug, logtype_afpd, "ConfigFile %s not found, assuming defaults",
            cmdline->configfile);
        return AFPConfigInit(cmdline, cmdline);
    }

    /* scan in the configuration file */
    len = 0;
    while (!feof(fp)) {
	if (!fgets(&buf[len], LINESIZE - len, fp) || buf[len] == '#')
            continue;
	len = strlen(buf);
	if ( len >= 2 && buf[len-2] == '\\' ) {
	    len -= 2;
	    continue;
	} else
	    len = 0;

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

        /* AFPConfigInit can return two linked configs due to DSI and ASP */
        if (!first) {
            if ((first = AFPConfigInit(&options, cmdline)))
                config = first->next ? first->next : first;
        } else if ((config->next = AFPConfigInit(&options, cmdline))) {
            config = config->next->next ? config->next->next : config->next;
        }
    }

#ifdef HAVE_LDAP
    /* Parse afp_ldap.conf */
    acl_ldap_readconfig(_PATH_ACL_LDAPCONF);
#endif /* HAVE_LDAP */

    LOG(log_debug, logtype_afpd, "Finished parsing Config File");
    fclose(fp);

    if (!have_option)
        first = AFPConfigInit(cmdline, cmdline);

    /* Now register with zeroconf, we also need the volumes for that */
    if (! (first->obj.options.flags & OPTION_NOZEROCONF)) {
        load_volumes(&first->obj);
        zeroconf_register(first);
    }

    return first;
}
