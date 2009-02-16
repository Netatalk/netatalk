/*
 * $Id: status.c,v 1.22 2009-02-16 14:03:30 franklahm Exp $
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
#include <atalk/unicode.h>

#include "globals.h"  /* includes <netdb.h> */
#include "status.h"
#include "afp_config.h"
#include "icon.h"

static   size_t maxstatuslen = 0;

static void status_flags(char *data, const int notif, const int ipok,
                         const unsigned char passwdbits, const int dirsrvcs _U_, int flags)
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
    status |= AFPSRVRINFO_SRVRDIR; /* AFP 3.1 specs says we need to specify this, but may set the count to 0 */
    /* We don't set the UTF8 name flag here, we don't know whether we have enough space ... */

    if (flags & OPTION_UUID)	/* 05122008 FIXME: can we set AFPSRVRINFO_UUID here ? see AFPSRVRINFO_SRVRDIR*/
	status |= AFPSRVRINFO_UUID;

    status = htons(status);
    memcpy(data + AFPSTATUS_FLAGOFF, &status, sizeof(status));
}

static int status_server(char *data, const char *server, const struct afp_options *options)
{
    char                *start = data;
    char                *Obj, *Type, *Zone;
    char		buf[32];
    u_int16_t           status;
    size_t		len;

    /* make room for all offsets before server name */
    data += AFPSTATUS_PRELEN;

    /* extract the obj part of the server */
    Obj = (char *) server;
    nbp_name(server, &Obj, &Type, &Zone);
    if ((size_t)-1 == (len = convert_string( 
			options->unixcharset, options->maccharset, 
			Obj, strlen(Obj), buf, sizeof(buf))) ) {
	len = MIN(strlen(Obj), 31);
    	*data++ = len;
    	memcpy( data, Obj, len );
	LOG ( log_error, logtype_afpd, "Could not set servername, using fallback");
    } else {
    	*data++ = len;
    	memcpy( data, buf, len );
    }
    if ((len + 1) & 1) /* pad server name and length byte to even boundary */
        len++;
    data += len;

    /* make room for signature and net address offset. save location of
     * signature offset. we're also making room for directory names offset
     * and the utf-8 server name offset.
     *
     * NOTE: technically, we don't need to reserve space for the
     * signature and net address offsets if they're not going to be
     * used. as there are no offsets after them, it doesn't hurt to
     * have them specified though. so, we just do that to simplify
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
    const char		*machine = "Netatalk";
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

/* -------------------------------- 
 * it turns out that a server signature screws up separate
 * servers running on the same machine. to work around that, 
 * i add in an increment.
 * Not great, server signature are config dependent but well.
 */
 
static int           Id = 0;

/* server signature is a 16-byte quantity */
static u_int16_t status_signature(char *data, int *servoffset, DSI *dsi,
                                  const struct afp_options *options)
{
    char                 *status;
    char		 *usersign;
    int                  i;
    u_int16_t            offset, sigoff;
    long                 hostid;
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
    hostid += Id;
    Id++;
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

static size_t status_netaddress(char *data, int *servoffset,
                             const ASP asp, const DSI *dsi,
                             const struct afp_options *options)
{
    char               *begin;
    u_int16_t          offset;
    size_t             addresses_len = 0;

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
    *data++ = ((options->fqdn && dsi)? 1 : 0) + (dsi ? 1 : 0) + (asp ? 1 : 0) +
              (((options->flags & OPTION_ANNOUNCESSH) && options->fqdn && dsi)? 1 : 0);

    /* ip address */
    if (dsi) {
        const struct sockaddr_in *inaddr = &dsi->server;

        if (inaddr->sin_port == htons(DSI_AFPOVERTCP_PORT)) {
            *data++ = 6; /* length */
            *data++ = 0x01; /* basic ip address */
            memcpy(data, &inaddr->sin_addr.s_addr,
                   sizeof(inaddr->sin_addr.s_addr));
            data += sizeof(inaddr->sin_addr.s_addr);
            addresses_len += 7;
        } else {
            /* ip address + port */
            *data++ = 8;
            *data++ = 0x02; /* ip address with port */
            memcpy(data, &inaddr->sin_addr.s_addr,
                   sizeof(inaddr->sin_addr.s_addr));
            data += sizeof(inaddr->sin_addr.s_addr);
            memcpy(data, &inaddr->sin_port, sizeof(inaddr->sin_port));
            data += sizeof(inaddr->sin_port);
            addresses_len += 9;
        }
    }

    /* handle DNS names */
    if (options->fqdn && dsi) {
        size_t len = strlen(options->fqdn);
        if ( len + 2 + addresses_len < maxstatuslen - offset) {
            *data++ = len +2;
            *data++ = 0x04;
            memcpy(data, options->fqdn, len);
            data += len;
            addresses_len += len+2;
        }

        /* Annouce support for SSH tunneled AFP session, 
         * this feature is available since 10.3.2.
         * According to the specs (AFP 3.1 p.225) this should
         * be an IP+Port style value, but it only works with 
         * a FQDN. OSX Server uses FQDN as well.
         */
        if ( len + 2 + addresses_len < maxstatuslen - offset) {
            if (options->flags & OPTION_ANNOUNCESSH) {
                *data++ = len +2;
                *data++ = 0x05;
                memcpy(data, options->fqdn, len);
                data += len;
            }
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

static size_t status_directorynames(char *data, int *diroffset, 
				 const DSI *dsi _U_, 
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
	size_t len;
	char *p = strchr( options->fqdn, ':' );
	if (p) 
	    *p = '\0';
	len = strlen( options->k5service ) 
			+ strlen( options->fqdn )
			+ strlen( options->k5realm );
	len+=2; /* '/' and '@' */
	if ( len > 255 || len+2 > maxstatuslen - offset) {
	    *data++ = 0;
	    LOG ( log_error, logtype_afpd, "status: could not set directory service list, no more room");
	}	 
	else {
	    *data++ = 1; /* DirectoryNamesCount */
	    *data++ = len;
	    snprintf( data, len + 1, "%s/%s@%s", options->k5service,
				options->fqdn, options->k5realm );
	    data += len;
	    if (p)
	        *p = ':';
       }
    } else {
	*data++ = 0;
    }

    /* Calculate and store offset for UTF8ServerName */
    *diroffset += sizeof(u_int16_t);
    offset = htons(data - begin);
    memcpy(begin + *diroffset, &offset, sizeof(u_int16_t));

    /* return length of buffer */
    return (data - begin);
}

static size_t status_utf8servername(char *data, int *nameoffset,
				 const DSI *dsi _U_,
				 const struct afp_options *options)
{
    char *Obj, *Type, *Zone;
    u_int16_t namelen;
    size_t len;
    char *begin = data;
    u_int16_t offset, status;

    memcpy(&offset, data + *nameoffset, sizeof(offset));
    offset = ntohs(offset);
    data += offset;

    /* FIXME:
     * What is the valid character range for an nbpname?
     *
     * Apple's server likes to use the non-qualified hostname
     * This obviously won't work very well if multiple servers are running
     * on the box.
     */

    /* extract the obj part of the server */
    Obj = (char *) (options->server ? options->server : options->hostname);
    nbp_name(options->server ? options->server : options->hostname, &Obj, &Type, &Zone);

    if ((size_t) -1 == (len = convert_string (
					options->unixcharset, CH_UTF8_MAC, 
					Obj, strlen(Obj), data+sizeof(namelen), maxstatuslen-offset )) ) {
	LOG ( log_error, logtype_afpd, "Could not set utf8 servername");

	/* set offset to 0 */
	memset(begin + *nameoffset, 0, sizeof(offset));
        data = begin + offset;
    }
    else {
    	namelen = htons(len);
    	memcpy( data, &namelen, sizeof(namelen));
    	data += sizeof(namelen);
    	data += len;
    	offset = htons(offset);
    	memcpy(begin + *nameoffset, &offset, sizeof(u_int16_t));
        
        /* Now set the flag ... */
	memcpy(&status, begin + AFPSTATUS_FLAGOFF, sizeof(status));
	status = ntohs(status);
	status |= AFPSRVRINFO_SRVUTF8;
	status = htons(status);
	memcpy(begin + AFPSTATUS_FLAGOFF, &status, sizeof(status));
    }

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

/* ---------------------
*/
void status_reset()
{
    Id = 0;
}


/* ---------------------
 */
void status_init(AFPConfig *aspconfig, AFPConfig *dsiconfig,
                 const struct afp_options *options)
{
    ASP asp;
    DSI *dsi;
    char *status = NULL;
    size_t statuslen;
    int c, sigoff;

    if (!(aspconfig || dsiconfig) || !options)
        return;

    if (aspconfig) {
        status = aspconfig->status;
        maxstatuslen=sizeof(aspconfig->status);
        asp = aspconfig->obj.handle;
    } else
        asp = NULL;
	
    if (dsiconfig) {
        status = dsiconfig->status;
        maxstatuslen=sizeof(dsiconfig->status);
        dsi = dsiconfig->obj.handle;
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
		 (options->k5service && options->k5realm && options->fqdn), 
		 options->flags);
    /* returns offset to signature offset */
    c = status_server(status, options->server ? options->server :
                      options->hostname, options);
    status_machine(status);
    status_versions(status);
    status_uams(status, options->uamlist);
    if (options->flags & OPTION_CUSTOMICON)
        status_icon(status, icon, sizeof(icon), c);
    else
        status_icon(status, apple_atalk_icon, sizeof(apple_atalk_icon), c);

    sigoff = status_signature(status, &c, dsi, options);
    /* c now contains the offset where the netaddress offset lives */

    status_netaddress(status, &c, asp, dsi, options);
    /* c now contains the offset where the Directory Names Count offset lives */

    statuslen = status_directorynames(status, &c, dsi, options);
    /* c now contains the offset where the UTF-8 ServerName offset lives */

    if ( statuslen < maxstatuslen) 
        statuslen = status_utf8servername(status, &c, dsi, options);

#ifndef NO_DDP
    if (aspconfig) {
        if (dsiconfig) /* status is dsiconfig->status */
            memcpy(aspconfig->status, status, statuslen);
        asp_setstatus(asp, status, statuslen);
        aspconfig->signature = status + sigoff;
        aspconfig->statuslen = statuslen;
    }
#endif /* ! NO_DDP */

    if (dsiconfig) {
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
AFPObj  *obj;
char	*ibuf _U_, *rbuf;
int	ibuflen _U_, *rbuflen;
{
    AFPConfig *config = obj->config;

    memcpy(rbuf, config->status, config->statuslen);
    *rbuflen = config->statuslen;
    return AFP_OK;
}
