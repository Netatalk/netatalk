/*
 * $Id: kuam.c,v 1.4 2001-06-25 20:13:45 rufustfirefly Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef UAM_AFSKRB

#include <mit-copyright.h>
#include <krb.h>
#include <des.h>
#include <prot.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

/* use the bsd time.h struct defs for PC too! */
#include <sys/time.h>
#include <sys/types.h>

int     swap_bytes;

/*
 * krb_get_in_tkt() gets a ticket for a given principal to use a given
 * service and stores the returned ticket and session key for future
 * use.
 *
 * The "user", "instance", and "realm" arguments give the identity of
 * the client who will use the ticket.  The "service" and "sinstance"
 * arguments give the identity of the server that the client wishes
 * to use.  (The realm of the server is the same as the Kerberos server
 * to whom the request is sent.)  The "life" argument indicates the
 * desired lifetime of the ticket; the "key_proc" argument is a pointer
 * to the routine used for getting the client's private key to decrypt
 * the reply from Kerberos.  The "decrypt_proc" argument is a pointer
 * to the routine used to decrypt the reply from Kerberos; and "arg"
 * is an argument to be passed on to the "key_proc" routine.
 *
 * If all goes well, krb_get_in_tkt() returns INTK_OK, otherwise it
 * returns an error code:  If an AUTH_MSG_ERR_REPLY packet is returned
 * by Kerberos, then the error code it contains is returned.  Other
 * error codes returned by this routine include INTK_PROT to indicate
 * wrong protocol version, INTK_BADPW to indicate bad password (if
 * decrypted ticket didn't make sense), INTK_ERR if the ticket was for
 * the wrong server or the ticket store couldn't be initialized.
 *
 * The format of the message sent to Kerberos is as follows:
 *
 * Size			Variable		Field
 * ----			--------		-----
 *
 * 1 byte		KRB_PROT_VERSION	protocol version number
 * 1 byte		AUTH_MSG_KDC_REQUEST |	message type
 *			HOST_BYTE_ORDER		local byte order in lsb
 * string		user			client's name
 * string		instance		client's instance
 * string		realm			client's realm
 * 4 bytes		tlocal.tv_sec		timestamp in seconds
 * 1 byte		life			desired lifetime
 * string		service			service's name
 * string		sinstance		service's instance
 */

kuam_get_in_tkt(user, instance, realm, service, sinstance, life, rpkt )
    char	*user;
    char	*instance;
    char	*realm;
    char	*service;
    char	*sinstance;
    int		life;
    KTEXT	rpkt;
{
    KTEXT_ST pkt_st;
    KTEXT pkt = &pkt_st;	/* Packet to KDC */
    KTEXT_ST cip_st;
    KTEXT cip = &cip_st;	/* Returned Ciphertext */
    KTEXT_ST tkt_st;
    KTEXT tkt = &tkt_st;	/* Current ticket */
    unsigned char *v = pkt->dat; /* Prot vers no */
    unsigned char *t = (pkt->dat+1); /* Prot msg type */
    int msg_byte_order;
    int kerror;
    struct timeval t_local;
    u_int32_t rep_err_code;


    /* BUILD REQUEST PACKET */

    /* Set up the fixed part of the packet */
    *v = (unsigned char) KRB_PROT_VERSION;
    *t = (unsigned char) AUTH_MSG_KDC_REQUEST;
    *t |= HOST_BYTE_ORDER;

    /* Now for the variable info */
    (void) strcpy((char *)(pkt->dat+2),user); /* aname */
    pkt->length = 3 + strlen(user);
    (void) strcpy((char *)(pkt->dat+pkt->length),
		  instance);	/* instance */
    pkt->length += 1 + strlen(instance);
    (void) strcpy((char *)(pkt->dat+pkt->length),realm); /* realm */
    pkt->length += 1 + strlen(realm);

    (void) gettimeofday(&t_local,(struct timezone *) 0);
    /* timestamp */
    memcpy((pkt->dat+pkt->length), &(t_local.tv_sec), 4);
    pkt->length += 4;

    *(pkt->dat+(pkt->length)++) = (char) life;
    (void) strcpy((char *)(pkt->dat+pkt->length),service);
    pkt->length += 1 + strlen(service);
    (void) strcpy((char *)(pkt->dat+pkt->length),sinstance);
    pkt->length += 1 + strlen(sinstance);

    rpkt->length = 0;

    /* SEND THE REQUEST AND RECEIVE THE RETURN PACKET */

    if (kerror = send_to_kdc(pkt, rpkt, realm)) return(kerror);

    /* check packet version of the returned packet */
    if (pkt_version(rpkt) != KRB_PROT_VERSION)
        return(INTK_PROT);

    /* Check byte order */
    msg_byte_order = pkt_msg_type(rpkt) & 1;
    swap_bytes = 0;
    if (msg_byte_order != HOST_BYTE_ORDER) {
        swap_bytes++;
    }

    switch (pkt_msg_type(rpkt) & ~1) {
    case AUTH_MSG_KDC_REPLY:
        break;
    case AUTH_MSG_ERR_REPLY:
        memcpy(&rep_err_code,pkt_err_code(rpkt),4);
        if (swap_bytes) swap_u_long(rep_err_code);
        return((int)rep_err_code);
    default:
        return(INTK_PROT);
    }

    return( INTK_OK );
}

kuam_set_in_tkt( user, instance, realm, service, sinstance, ptr)
    char	*user, *instance, *realm, *service, *sinstance, *ptr;
{
    KTEXT_ST		tkt_st;
    KTEXT		tkt = &tkt_st;
    struct timeval	t_local;
    int			lifetime, kvno, kerror;
    int32_t		kdc_time;
    C_Block		ses;
    char		s_name[ SNAME_SZ ], s_instance[ INST_SZ ];
    char		rlm[ REALM_SZ ];

    /* extract session key */
    memcpy(ses, ptr, 8);
    ptr += 8;

    /* extract server's name */
    (void) strcpy(s_name,ptr);
    ptr += strlen(s_name) + 1;

    /* extract server's instance */
    (void) strcpy(s_instance,ptr);
    ptr += strlen(s_instance) + 1;

    /* extract server's realm */
    (void) strcpy(rlm,ptr);
    ptr += strlen(rlm) + 1;

    /* extract ticket lifetime, server key version, ticket length */
    /* be sure to avoid sign extension on lifetime! */
    lifetime = (unsigned char) ptr[0];
    kvno = (unsigned char) ptr[1];
    tkt->length = (unsigned char) ptr[2];
    ptr += 3;

    /* extract ticket itself */
    memcpy( tkt->dat, ptr, tkt->length);
    ptr += tkt->length;

    if (strcmp(s_name, service) || strcmp(s_instance, sinstance) ||
        strcmp(rlm, realm))	/* not what we asked for */
	return(INTK_ERR);	/* we need a better code here XXX */

    /* check KDC time stamp */
    memcpy(&kdc_time, ptr, 4); /* Time (coarse) */
    if (swap_bytes) swap_u_long(kdc_time);

    ptr += 4;

    (void) gettimeofday(&t_local,(struct timezone *) 0);
    if (abs((int)(t_local.tv_sec - kdc_time)) > CLOCK_SKEW) {
        return(RD_AP_TIME);		/* XXX should probably be better
					   code */
    }

    /* initialize ticket cache */
    if (in_tkt(user,instance) != KSUCCESS)
	return(INTK_ERR);

    /* stash ticket, session key, etc. for future use */
    if (kerror = save_credentials(s_name, s_instance, rlm, ses,
				  lifetime, kvno, tkt, t_local.tv_sec))
	return(kerror);

    return(INTK_OK);
}

#endif /* UAM_AFSKRB */
