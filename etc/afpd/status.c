/*
 * $Id: status.c,v 1.15 2003-06-09 14:42:40 srittau Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <atalk/logger.h>

#ifdef BSD4_4
#include <sys/param.h>
#ifndef USE_GETHOSTID
#include <sys/sysctl.h>
#endif /* USE_GETHOSTID */
#endif /* BSD4_4 */

#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/nbp.h>

#include "globals.h"  /* includes <netdb.h> */
#include "status.h"
#include "afp_config.h"
#include "icon.h"

static void status_flags(char *data, const int notif, const int ipok,
                         const unsigned char passwdbits, const int dirsrvcs)
{
    u_int16_t           status;

    status = AFPSRVRINFO_COPY;
    if (passwdbits & PASSWD_SET) /* some uams may not allow this. */
        status |= AFPSRVRINFO_PASSWD;
    if (passwdbits & PASSWD_NOSAVE)
        status |= AFPSRVRINFO_NOSAVEPASSWD;
    status |= AFPSRVRINFO_SRVSIGNATURE;
    /* only advertise tcp/ip if we have a valid address */
    if (ipok)
        status |= AFPSRVRINFO_TCPIP;
    status |= AFPSRVRINFO_SRVMSGS;
    /* Allow the user to decide if we should support server notifications.
     * With this turned off, the clients will poll for directory changes every
     * 10 seconds.  This might be too costly to network resources, so make
     * this an optional thing.  Default will be to _not_ support server
     * notifications. */
    if (notif) {
        status |= AFPSRVRINFO_SRVNOTIFY;
    }
    status |= AFPSRVRINFO_FASTBOZO;
    if (dirsrvcs)
	status |= AFPSRVRINFO_SRVRDIR;
    status = htons(status);
    memcpy(data + AFPSTATUS_FLAGOFF, &status, sizeof(status));
}

static int status_server(char *data, const char *server)
{
    char                *start = data;
    char                *Obj, *Type, *Zone;
    u_int16_t           status;
    int			len;

    /* make room for all offsets before server name */
    data += AFPSTATUS_PRELEN;

    /* extract the obj part of the server */
    Obj = (char *) server;
    nbp_name(server, &Obj, &Type, &Zone);
    len = strlen(Obj);
    *data++ = len;
    memcpy( data, Obj, len );
    if ((len + 1) & 1) /* pad server name and length byte to even boundary */
        len++;
    data += len;

    /* Make room for signature and net address offset. Save location of
     * signature offset. We're also making room for directory names offset
     * and the utf-8 server name offset.
     *
     * NOTE: Technically, we don't need to reserve space for the
     * signature and net address offsets if they're not going to be
     * used. As there are no offsets after them, it doesn't hurt to
     * have them specified though. So, we just do that to simplify
     * things.  
     *
     * NOTE2: AFP3.1 Documentation states that the directory names offset
     * is a required feature, even though it can be set to zero.
     */
    len = data - start;
    status = htons(len + AFPSTATUS_POSTLEN);
    memcpy(start + AFPSTATUS_MACHOFF, &status, sizeof(status));
    return len; /* return the offset to beginning of signature offset */
}

static void status_machine(char *data)
{
    char                *start = data;
    u_int16_t           status;
    int			len;
#ifdef AFS
    const char		*machine = "afs";
#else /* !AFS */
    const char		*machine = "unix";
#endif /* AFS */

    memcpy(&status, start + AFPSTATUS_MACHOFF, sizeof(status));
    data += ntohs( status );
    len = strlen( machine );
    *data++ = len;
    memcpy( data, machine, len );
    data += len;
    status = htons(data - start);
    memcpy(start + AFPSTATUS_VERSOFF, &status, sizeof(status));
}


/* server signature is a 16-byte quantity */
static u_int16_t status_signature(char *data, int *servoffset, DSI *dsi,
                                  const struct afp_options *options)
{
    char                 *status;
    char		 *usersign;
    int                  i;
    u_int16_t            offset, sigoff;
    long                 hostid;
    static int           id = 0;
#ifdef BSD4_4
    int                  mib[2];
    size_t               len;
#endif /* BSD4_4 */

    status = data;

    /* get server signature offset */
    memcpy(&offset, data + *servoffset, sizeof(offset));
    sigoff = offset = ntohs(offset);

    /* jump to server signature offset */
    data += offset;

    /* Signature type is user string */
    if (strncmp(options->signature, "user", 4) == 0) {
        if (strlen(options->signature) <= 5) {
	    LOG(log_warning, logtype_afpd, "Signature %s id not valid. Switching back to hostid.",
			    options->signature);
	    goto server_signature_hostid;
    }
    usersign = options->signature + 5;
        if (strlen(usersign) < 3) 
	    LOG(log_warning, logtype_afpd, "Signature %s is very short !", 
			    options->signature);
    
        memset(data, 0, 16);
        strncpy(data, usersign, 16);
	data += 16;
        goto server_signature_done;
    } /* signature = user */

    /* If signature type is a standard hostid... */
server_signature_hostid:
    /* 16-byte signature consists of copies of the hostid */
#if defined(BSD4_4) && defined(USE_GETHOSTID)
    mib[0] = CTL_KERN;
    mib[1] = KERN_HOSTID;
    len = sizeof(hostid);
    sysctl(mib, 2, &hostid, &len, NULL, 0);
#else /* BSD4_4 && USE_GETHOSTID */
    hostid = gethostid();
#endif /* BSD4_4 && USE_GETHOSTID */
    if (!hostid) {
        if (dsi)
            hostid = dsi->server.sin_addr.s_addr;
        else {
            struct hostent *host;

            if ((host = gethostbyname(options->hostname)))
                hostid = ((struct in_addr *) host->h_addr)->s_addr;
        }
    }

    /* it turns out that a server signature screws up separate
     * servers running on the same machine. to work around that, 
     * i add in an increment */
    hostid += id;
    id++;
    for (i = 0; i < 16; i += sizeof(hostid)) {
        memcpy(data, &hostid, sizeof(hostid));
        data += sizeof(hostid);
     }

server_signature_done:
    /* calculate net address offset */
    *servoffset += sizeof(offset);
    offset = htons(data - status);
    memcpy(status + *servoffset, &offset, sizeof(offset));
    return sigoff;
}

static int status_netaddress(char *data, int *servoffset,
                             const ASP asp, const DSI *dsi,
                             const char *fqdn)
{
    char               *begin;
    u_int16_t          offset;

    begin = data;

    /* get net address offset */
    memcpy(&offset, data + *servoffset, sizeof(offset));
    data += ntohs(offset);

    /* format:
       Address count (byte) 
       len (byte = sizeof(length + address type + address) 
       address type (byte, ip address = 0x01, ip + port = 0x02,
                           ddp address = 0x03, fqdn = 0x04) 
       address (up to 254 bytes, ip = 4, ip + port = 6, ddp = 4)
       */

    /* number of addresses. this currently screws up if we have a dsi
       connection, but we don't have the ip address. to get around this,
       we turn off the status flag for tcp/ip. */
    *data++ = ((fqdn && dsi)? 1 : 0) + (dsi ? 1 : 0) + (asp ? 1 : 0);

    /* handle DNS names */
    if (fqdn && dsi) {
        int len = strlen(fqdn);
        *data++ = len +2;
        *data++ = 0x04;
        memcpy(data, fqdn, len);
        data += len;
    }

    /* ip address */
    if (dsi) {
        const struct sockaddr_in *inaddr = &dsi->server;

        if (inaddr->sin_port == htons(DSI_AFPOVERTCP_PORT)) {
            *data++ = 6; /* length */
            *data++ = 0x01; /* basic ip address */
            memcpy(data, &inaddr->sin_addr.s_addr,
                   sizeof(inaddr->sin_addr.s_addr));
            data += sizeof(inaddr->sin_addr.s_addr);
        } else {
            /* ip address + port */
            *data++ = 8;
            *data++ = 0x02; /* ip address with port */
            memcpy(data, &inaddr->sin_addr.s_addr,
                   sizeof(inaddr->sin_addr.s_addr));
            data += sizeof(inaddr->sin_addr.s_addr);
            memcpy(data, &inaddr->sin_port, sizeof(inaddr->sin_port));
            data += sizeof(inaddr->sin_port);
        }
    }

#ifndef NO_DDP
    if (asp) {
        const struct sockaddr_at *ddpaddr = atp_sockaddr(asp->asp_atp);

        /* ddp address */
        *data++ = 6;
        *data++ = 0x03; /* ddp address */
        memcpy(data, &ddpaddr->sat_addr.s_net, sizeof(ddpaddr->sat_addr.s_net));
        data += sizeof(ddpaddr->sat_addr.s_net);
        memcpy(data, &ddpaddr->sat_addr.s_node,
               sizeof(ddpaddr->sat_addr.s_node));
        data += sizeof(ddpaddr->sat_addr.s_node);
        memcpy(data, &ddpaddr->sat_port, sizeof(ddpaddr->sat_port));
        data += sizeof(ddpaddr->sat_port);
    }
#endif /* ! NO_DDP */

    /* calculate/store Directory Services Names offset */
    offset = htons(data - begin); 
    *servoffset += sizeof(offset);
    memcpy(begin + *servoffset, &offset, sizeof(offset));

    /* return length of buffer */
    return (data - begin);
}

static int status_directorynames(char *data, int *diroffset, 
				 const DSI *dsi, 
				 const struct afp_options *options)
{
    char *begin = data;
    u_int16_t offset;
    memcpy(&offset, data + *diroffset, sizeof(offset));
    offset = ntohs(offset);
    data += offset;

    /* I can not find documentation of any other purpose for the
     * DirectoryNames field.
     */
    /*
     * Try to synthesize a principal:
     * service '/' fqdn '@' realm
     */
    if (options->k5service && options->k5realm && options->fqdn) {
	/* should k5princ be utf8 encoded? */
	u_int8_t len;
	char *p = strchr( options->fqdn, ':' );
	if (p) 
	    *p = '\0';
	len = strlen( options->k5service ) 
			+ strlen( options->fqdn )
			+ strlen( options->k5realm );
	len+=2; /* '/' and '@' */
	*data++ = 1; /* DirectoryNamesCount */
	*data++ = len;
	snprintf( data, len + 1, "%s/%s@%s", options->k5service,
				options->fqdn, options->k5realm );
	data += len;
	if (p)
	    *p = ':';
    } else {
	memset(begin + *diroffset, 0, sizeof(offset));
    }

    /* Calculate and store offset for UTF8ServerName */
    *diroffset += sizeof(u_int16_t);
    offset = htons(data - begin);
    memcpy(begin + *diroffset, &offset, sizeof(u_int16_t));

    /* return length of buffer */
    return (data - begin);
}

static int status_utf8servername(char *data, int *nameoffset,
				 const DSI *dsi,
				 const struct afp_options *options)
{
    char *begin = data;
    u_int16_t offset;

    memcpy(&offset, data + *nameoffset, sizeof(offset));
    offset = ntohs(offset);
    data += offset;

    /* FIXME: For now we set the UTF-8 ServerName offset to 0
     * It's irrelevent anyway until we set the appropriate Flags value.
     * Later this can be set to something meaningful.
     *
     * What is the valid character range for an nbpname?
     *
     * Apple's server likes to use the non-qualified hostname
     * This obviously won't work very well if multiple servers are running
     * on the box.
     */
    memset(begin + *nameoffset, 0, sizeof(offset));

    /* return length of buffer */
    return (data - begin);

}

/* returns actual offset to signature */
static void status_icon(char *data, const unsigned char *icondata,
                        const int iconlen, const int sigoffset)
{
    char                *start = data;
    char                *sigdata = data + sigoffset;
    u_int16_t		ret, status;

    memcpy(&status, start + AFPSTATUS_ICONOFF, sizeof(status));
    if ( icondata == NULL ) {
        ret = status;
        memset(start + AFPSTATUS_ICONOFF, 0, sizeof(status));
    } else {
        data += ntohs( status );
        memcpy( data, icondata, iconlen);
        data += iconlen;
        ret = htons(data - start);
    }

    /* put in signature offset */
    if (sigoffset)
        memcpy(sigdata, &ret, sizeof(ret));
}

void status_init(AFPConfig *aspconfig, AFPConfig *dsiconfig,
                 const struct afp_options *options)
{
    ASP asp;
    DSI *dsi;
    char *status;
    int statuslen, c, sigoff;

    if (!(aspconfig || dsiconfig) || !options)
        return;

    if (aspconfig) {
        asp = aspconfig->obj.handle;
        status = aspconfig->status;
    } else
        asp = NULL;

    if (dsiconfig) {
        dsi = dsiconfig->obj.handle;
        if (!aspconfig)
            status = dsiconfig->status;
    } else
        dsi = NULL;

    /*
     * These routines must be called in order -- earlier calls
     * set the offsets for later calls.
     *
     * using structs is a bad idea, but that's what the original code
     * does. solaris, in fact, will segfault with them. so, now
     * we just use the powers of #defines and memcpy.
     *
     * reply block layout (offsets are 16-bit quantities):
     * machine type offset -> AFP version count offset ->
     * UAM count offset -> vol icon/mask offset -> flags ->
     *
     * server name [padded to even boundary] -> signature offset ->
     * network address offset ->
     *
     * at the appropriate offsets:
     * machine type, afp versions, uams, server signature
     * (16-bytes), network addresses, volume icon/mask 
     */

    status_flags(status, options->server_notif, options->fqdn ||
                 (dsiconfig && dsi->server.sin_addr.s_addr),
                 options->passwdbits, 
                 (options->k5service && options->k5realm && options->fqdn));
    /* returns offset to signature offset */
    c = status_server(status, options->server ? options->server :
                      options->hostname);
    status_machine(status);
    status_versions(status);
    status_uams(status, options->uamlist);
    if (options->flags & OPTION_CUSTOMICON)
        status_icon(status, icon, sizeof(icon), c);
    else
        status_icon(status, apple_atalk_icon, sizeof(apple_atalk_icon), c);

    sigoff = status_signature(status, &c, dsi, options);
    /* c now contains the offset where the netaddress offset lives */

    status_netaddress(status, &c, asp, dsi, options->fqdn);
    /* c now contains the offset where the Directory Names Count offset lives */

    status_directorynames(status, &c, dsi, options);
    /* c now contains the offset where the UTF-8 ServerName offset lives */

    statuslen = status_utf8servername(status, &c, dsi, options);


#ifndef NO_DDP
    if (aspconfig) {
        asp_setstatus(asp, status, statuslen);
        aspconfig->signature = status + sigoff;
        aspconfig->statuslen = statuslen;
    }
#endif /* ! NO_DDP */

    if (dsiconfig) {
        if (aspconfig) { /* copy to dsiconfig */
            memcpy(dsiconfig->status, status, ATP_MAXDATA);
            status = dsiconfig->status;
        }

        if ((options->flags & OPTION_CUSTOMICON) == 0) {
            status_icon(status, apple_tcp_icon, sizeof(apple_tcp_icon), 0);
        }
        dsi_setstatus(dsi, status, statuslen);
        dsiconfig->signature = status + sigoff;
        dsiconfig->statuslen = statuslen;
    }
}

/* this is the same as asp/dsi_getstatus */
int afp_getsrvrinfo(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    AFPConfig *config = obj->config;

    memcpy(rbuf, config->status, config->statuslen);
    *rbuflen = config->statuslen;
    return AFP_OK;
}
