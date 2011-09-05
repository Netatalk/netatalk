/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <atalk/logger.h>

#if 0
#ifdef BSD4_4
#include <sys/param.h>
#ifndef HAVE_GETHOSTID
#include <sys/sysctl.h>
#endif /* HAVE_GETHOSTID */
#endif /* BSD4_4 */
#endif

#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/nbp.h>
#include <atalk/unicode.h>
#include <atalk/util.h>
#include <atalk/globals.h>

#include "status.h"
#include "afp_config.h"
#include "icon.h"

static   size_t maxstatuslen = 0;

static void status_flags(char *data, const int notif, const int ipok,
                         const unsigned char passwdbits, const int dirsrvcs _U_, int flags)
{
    uint16_t           status;

    status = AFPSRVRINFO_COPY
           | AFPSRVRINFO_SRVSIGNATURE
           | AFPSRVRINFO_SRVMSGS
           | AFPSRVRINFO_FASTBOZO
           | AFPSRVRINFO_SRVRDIR
           | AFPSRVRINFO_SRVUTF8
           | AFPSRVRINFO_EXTSLEEP;

    if (passwdbits & PASSWD_SET) /* some uams may not allow this. */
        status |= AFPSRVRINFO_PASSWD;
    if (passwdbits & PASSWD_NOSAVE)
        status |= AFPSRVRINFO_NOSAVEPASSWD;
    if (ipok) /* only advertise tcp/ip if we have a valid address */        
        status |= AFPSRVRINFO_TCPIP;
    if (notif) /* Default is yes */        
        status |= AFPSRVRINFO_SRVNOTIFY;
    if (flags & OPTION_UUID)
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
#ifndef NO_DDP
    nbp_name(server, &Obj, &Type, &Zone);
#endif
    if ((size_t)-1 == (len = convert_string( 
                           options->unixcharset, options->maccharset, 
                           Obj, -1, buf, sizeof(buf))) ) {
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
    const char		*machine = "Netatalk%s";
#endif /* AFS */
    char buf[AFPSTATUS_MACHLEN+1];

    memcpy(&status, start + AFPSTATUS_MACHOFF, sizeof(status));
    data += ntohs( status );

    if ((strlen(machine) + strlen(VERSION)) <= AFPSTATUS_MACHLEN) {
        len = snprintf(buf, AFPSTATUS_MACHLEN + 1, machine, VERSION);
    } else {
        if (strlen(VERSION) > AFPSTATUS_MACHLEN) {
            len = snprintf(buf, AFPSTATUS_MACHLEN + 1, VERSION);
        } else {
            (void)snprintf(buf, AFPSTATUS_MACHLEN + 1, machine, "");
            (void)snprintf(buf + AFPSTATUS_MACHLEN - strlen(VERSION),
                           strlen(VERSION) + 1,
                           VERSION);
            len = AFPSTATUS_MACHLEN;
        }
    }

    *data++ = len;
    memcpy( data, buf, len );
    data += len;

    status = htons(data - start);
    memcpy(start + AFPSTATUS_VERSOFF, &status, sizeof(status));
}

/* server signature is a 16-byte quantity */
static u_int16_t status_signature(char *data, int *servoffset,
                                  const struct afp_options *options)
{
    char                 *status;
    u_int16_t            offset, sigoff;

    status = data;

    /* get server signature offset */
    memcpy(&offset, data + *servoffset, sizeof(offset));
    sigoff = offset = ntohs(offset);

    /* jump to server signature offset */
    data += offset;

    memset(data, 0, 16);
    memcpy(data, options->signature, 16);
    data += 16;

    /* calculate net address offset */
    *servoffset += sizeof(offset);
    offset = htons(data - status);
    memcpy(status + *servoffset, &offset, sizeof(offset));
    return sigoff;
}

static size_t status_netaddress(char *data, int *servoffset,
#ifndef NO_DDP
                                const ASP asp,
#endif
                                const DSI *dsi,
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
    *data++ = ((options->fqdn && dsi)? 1 : 0) + (dsi ? 1 : 0) +
#ifndef NO_DDP
        (asp ? 1 : 0) +
#endif
              (((options->flags & OPTION_ANNOUNCESSH) && options->fqdn && dsi)? 1 : 0);

    /* ip address */
    if (dsi) {
        if (dsi->server.ss_family == AF_INET) { /* IPv4 */
            const struct sockaddr_in *inaddr = (struct sockaddr_in *)&dsi->server;
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
        } else { /* IPv6 */
            const struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&dsi->server;
            if (sa6->sin6_port == htons(DSI_AFPOVERTCP_PORT)) {
                *data++ = 18; /* length */
                *data++ = 6; /* type */
                memcpy(data, &sa6->sin6_addr.s6_addr, sizeof(sa6->sin6_addr.s6_addr));
                data += sizeof(sa6->sin6_addr.s6_addr);
                addresses_len += 19;
            } else {
                /* ip address + port */
                *data++ = 20; /* length */
                *data++ = 7; /* type*/
                memcpy(data, &sa6->sin6_addr.s6_addr, sizeof(sa6->sin6_addr.s6_addr));
                data += sizeof(sa6->sin6_addr.s6_addr);
                memcpy(data, &sa6->sin6_port, sizeof(sa6->sin6_port));
                data += sizeof(sa6->sin6_port);
                addresses_len += 21;
            }

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
#ifndef NO_DDP
    nbp_name(options->server ? options->server : options->hostname, &Obj, &Type, &Zone);
#endif
    if ((size_t) -1 == (len = convert_string (
					options->unixcharset, CH_UTF8_MAC, 
					Obj, -1, data+sizeof(namelen), maxstatuslen-offset )) ) {
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
void status_init(AFPConfig *aspconfig, AFPConfig *dsiconfig,
                 const struct afp_options *options)
{
#ifndef NO_DDP
    ASP asp;
#endif
    DSI *dsi;
    char *status = NULL;
    size_t statuslen;
    int c, sigoff, ipok;

    if (!(aspconfig || dsiconfig) || !options)
        return;

#ifndef NO_DDP
    if (aspconfig) {
        status = aspconfig->status;
        maxstatuslen=sizeof(aspconfig->status);
        asp = aspconfig->obj.handle;
    } else
        asp = NULL;
#endif
	
    ipok = 0;
    if (dsiconfig) {
        status = dsiconfig->status;
        maxstatuslen=sizeof(dsiconfig->status);
        dsi = dsiconfig->obj.handle;
        if (dsi->server.ss_family == AF_INET) { /* IPv4 */
            const struct sockaddr_in *sa4 = (struct sockaddr_in *)&dsi->server;
            ipok = sa4->sin_addr.s_addr ? 1 : 0;
        } else { /* IPv6 */
            const struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&dsi->server;
            for (int i=0; i<16; i++) {
                if (sa6->sin6_addr.s6_addr[i]) {
                    ipok = 1;
                    break;
                }
            }
        }
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

    status_flags(status,
                 options->server_notif,
                 (options->fqdn || ipok),
                 options->passwdbits, 
                 (options->k5service && options->k5realm && options->fqdn),
                 options->flags);
    /* returns offset to signature offset */
    c = status_server(status, options->server ? options->server :
                      options->hostname, options);
    status_machine(status);
    status_versions(status,
#ifndef NO_DDP
                    asp,
#endif
                    dsi);
    status_uams(status, options->uamlist);
    if (options->flags & OPTION_CUSTOMICON)
        status_icon(status, icon, sizeof(icon), c);
    else
        status_icon(status, apple_atalk_icon, sizeof(apple_atalk_icon), c);

    sigoff = status_signature(status, &c, options);
    /* c now contains the offset where the netaddress offset lives */

    status_netaddress(status, &c,
#ifndef NO_DDP
                      asp,
#endif
                      dsi, options);
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

/* set_signature()                                                    */
/*                                                                    */
/* If found in conf file, use it.                                     */
/* If not found in conf file, genarate and append in conf file.       */
/* If conf file don't exist, create and genarate.                     */
/* If cannot open conf file, use one-time signature.                  */
/* If -signature user:xxxxx, use it.                                  */

void set_signature(struct afp_options *options) {
    char *usersign;
    int fd, i;
    struct stat tmpstat;
    char *servername_conf;
    int header = 0;
    char buf[1024], *p;
    FILE *fp = NULL, *randomp;
    size_t len;
    char *server_tmp;
    
    server_tmp = (options->server ? options->server : options->hostname);
    if (strcmp(options->signatureopt, "auto") == 0) {
        goto server_signature_auto;   /* default */
    } else if (strcmp(options->signatureopt, "host") == 0) {
        LOG(log_warning, logtype_afpd, "WARNING: option \"-signature host\" is obsoleted. Switching back to auto.", options->signatureopt);
        goto server_signature_auto;   /* same as auto */
    } else if (strncmp(options->signatureopt, "user", 4) == 0) {
        goto server_signature_user;   /*  user string */
    } else {
        LOG(log_error, logtype_afpd, "ERROR: option \"-signature %s\" is not valid. Switching back to auto.", options->signatureopt);
        goto server_signature_auto;   /* switch back to auto*/
    }
    
server_signature_user:
    
    /* Signature type is user string */
    len = strlen(options->signatureopt);
    if (len <= 5) {
        LOG(log_warning, logtype_afpd, "WARNING: option \"-signature %s\" is not valid. Switching back to auto.", options->signatureopt);
        goto server_signature_auto;
    }
    usersign = options->signatureopt + 5;
    len = len - 5;
    if (len > 16) {
        LOG(log_warning, logtype_afpd, "WARNING: signature user string %s is very long !",  usersign);
        len = 16;
    } else if (len >= 3) {
        LOG(log_info, logtype_afpd, "signature user string is %s.", usersign);
    } else {
        LOG(log_warning, logtype_afpd, "WARNING: signature user string %s is very short !",  usersign);
    }
    memset(options->signature, 0, 16);
    memcpy(options->signature, usersign, len);
    goto server_signature_done;
    
server_signature_auto:
    
    /* Signature type is auto, using afp_signature.conf */
    if (!stat(options->sigconffile, &tmpstat)) {                /* conf file exists? */
        if ((fp = fopen(options->sigconffile, "r")) != NULL) {  /* read open? */
            /* scan in the conf file */
            while (fgets(buf, sizeof(buf), fp) != NULL) { 
                p = buf;
                while (p && isblank(*p))
                    p++;
                if (!p || (*p == '#') || (*p == '\n'))
                    continue;                             /* invalid line */
                if (*p == '"') {
                    p++;
                    if ((servername_conf = strtok( p, "\"" )) == NULL)
                        continue;                         /* syntax error: invalid quoted servername */
                } else {
                    if ((servername_conf = strtok( p, " \t" )) == NULL)
                        continue;                         /* syntax error: invalid servername */
                }
                p = strchr(p, '\0');
                p++;
                if (*p == '\0')
                    continue;                             /* syntax error: missing signature */
                
                if (strcmp(server_tmp, servername_conf))
                    continue;                             /* another servername */
                
                while (p && isblank(*p))
                    p++;
                if ( 16 == sscanf(p, "%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",
                                  &options->signature[ 0], &options->signature[ 1],
                                  &options->signature[ 2], &options->signature[ 3],
                                  &options->signature[ 4], &options->signature[ 5],
                                  &options->signature[ 6], &options->signature[ 7],
                                  &options->signature[ 8], &options->signature[ 9],
                                  &options->signature[10], &options->signature[11],
                                  &options->signature[12], &options->signature[13],
                                  &options->signature[14], &options->signature[15]
                         )) {
                    fclose(fp);
                    goto server_signature_done;                 /* found in conf file */
                }
            }
            if ((fp = freopen(options->sigconffile, "a+", fp)) != NULL) { /* append because not found */
                fseek(fp, 0L, SEEK_END);
                if(ftell(fp) == 0) {                     /* size = 0 */
                    header = 1;
                    goto server_signature_random;
                } else {
                    fseek(fp, -1L, SEEK_END);
                    if(fgetc(fp) != '\n') fputc('\n', fp); /* last char is \n? */
                    goto server_signature_random;
                }                    
            } else {
                LOG(log_error, logtype_afpd, "ERROR: Cannot write in %s (%s). Using one-time signature.",
                    options->sigconffile, strerror(errno));
                goto server_signature_random;
            }
        } else {
            LOG(log_error, logtype_afpd, "ERROR: Cannot read %s (%s). Using one-time signature.",
                options->sigconffile, strerror(errno));
            goto server_signature_random;
        }
    } else {                                                          /* conf file don't exist */
        if (( fd = creat(options->sigconffile, 0644 )) < 0 ) {
            LOG(log_error, logtype_afpd, "ERROR: Cannot create %s (%s). Using one-time signature.",
                options->sigconffile, strerror(errno));
            goto server_signature_random;
        }
        if (( fp = fdopen( fd, "w" )) == NULL ) {
            LOG(log_error, logtype_afpd, "ERROR: Cannot fdopen %s (%s). Using one-time signature.",
                options->sigconffile, strerror(errno));
            close(fd);
            goto server_signature_random;
        }
        header = 1;
        goto server_signature_random;
    }
    
server_signature_random:
    
    /* generate signature from random number */
    randombytes(options->signature, 16);

    if (fp && header) { /* conf file is created or size=0 */
        fprintf(fp, "# DON'T TOUCH NOR COPY THOUGHTLESSLY!\n");
        fprintf(fp, "# This file is auto-generated by afpd.\n");
        fprintf(fp, "# \n");
        fprintf(fp, "# ServerSignature is unique identifier used to prevent logging on to\n");
        fprintf(fp, "# the same server twice.\n");
        fprintf(fp, "# \n");
        fprintf(fp, "# If setting \"-signature user:xxxxx\" in afpd.conf, this file is not used.\n\n");
    }
    
    if (fp) {
        fprintf(fp, "\"%s\"\t", server_tmp);
        for (i=0 ; i<16 ; i++) {
            fprintf(fp, "%02X", (options->signature)[i]);
        }
        fprintf(fp, "%s", "\n");
        fclose(fp);
    }
    
server_signature_done:
    
    /* retrun */
    LOG(log_info, logtype_afpd,
        " \"%s\"'s signature is  %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
        server_tmp,
        (options->signature)[ 0], (options->signature)[ 1],
        (options->signature)[ 2], (options->signature)[ 3],
        (options->signature)[ 4], (options->signature)[ 5],
        (options->signature)[ 6], (options->signature)[ 7],
        (options->signature)[ 8], (options->signature)[ 9],
        (options->signature)[10], (options->signature)[11],
        (options->signature)[12], (options->signature)[13],
        (options->signature)[14], (options->signature)[15]);
    
    return;
}

/* this is the same as asp/dsi_getstatus */
int afp_getsrvrinfo(AFPObj *obj, char *ibuf _U_, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    AFPConfig *config = obj->config;

    memcpy(rbuf, config->status, config->statuslen);
    *rbuflen = config->statuslen;
    return AFP_OK;
}
