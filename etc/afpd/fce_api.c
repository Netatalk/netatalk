/*
 * $Id: fce_api.c,v 0.01 2010-10-01 00:00:0 mw Exp $
 *
 * Copyright (c) 2010 Mark Williams
 *
 * File change event API for netatalk
 *
 * for every detected filesystem change a UDP packet is sent to an arbitrary list
 * of listeners. Each packet contains unix path of modified filesystem element,
 * event reason, and a consecutive event id (32 bit). Technically we are UDP client and are sending
 * out packets synchronuosly as they are created by the afp functions. This should not affect
 * performance measurably. The only delaying calls occur during initialization, if we have to
 * resolve non-IP hostnames to IP. All numeric data inside the packet is network byte order, so use
 * ntohs / ntohl to resolve length and event id. Ideally a listener receives every packet with
 * no gaps in event ids, starting with event id 1 and mode FCE_CONN_START followed by
 * data events from id 2 up to 0xFFFFFFFF, followed by 0 to 0xFFFFFFFF and so on.
 *
 * A gap or not starting with 1 mode FCE_CONN_START or receiving mode FCE_CONN_BROKEN means that
 * the listener has lost at least one filesystem event
 * 
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>


#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <netatalk/at.h>

#include <atalk/adouble.h>
#include <atalk/vfs.h>
#include <atalk/logger.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/cnid.h>
#include <atalk/unix.h>

#include "fork.h"
#include "file.h"
#include "globals.h"
#include "directory.h"
#include "desktop.h"
#include "volume.h"

#include "fce_api.h"

// ONLY USED IN THIS FILE
#include "fce_api_internal.h"

#define FCE_TRUE 1
#define FCE_FALSE 0

/* We store our connection data here */
static struct udp_entry udp_socket_list[FCE_MAX_UDP_SOCKS];
static int udp_sockets = 0;
static int udp_initialized = FCE_FALSE;


static const char *skip_files[] = 
{
	".DS_Store",
	NULL
};

/*
 *
 * Initialize network structs for any listeners
 * We dont give return code because all errors are handled internally (I hope..)
 *
 * */
void fce_init_udp()
{
    if (udp_initialized == FCE_TRUE)
        return;


    for (int i = 0; i < udp_sockets; i++)
    {
        struct udp_entry *udp_entry = udp_socket_list + i;

        /* Close any pending sockets */
        if (udp_entry->sock != -1)
        {
            close( udp_entry->sock );
        }

        /* resolve IP to network address */
        if (inet_aton( udp_entry->ip, &udp_entry->addr.sin_addr ) ==0 )
        {
            /* Hmm, failed try to resolve host */
            struct hostent *hp = gethostbyname( udp_entry->ip );
            if (hp == NULL)
            {
                LOG(log_error, logtype_afpd, "Cannot resolve host name for fce UDP connection: %s (errno %d)", udp_entry->ip, errno  );
                continue;
            }
            memcpy( &udp_entry->addr.sin_addr, &hp->h_addr, sizeof(udp_entry->addr.sin_addr) );
        }

        /* Create UDP socket */
        udp_entry->sock = socket( AF_INET, SOCK_DGRAM, 0 );
        if (udp_entry->sock == -1)
        {
            LOG(log_error, logtype_afpd, "Cannot create socket for fce UDP connection: errno %d", errno  );
            continue;
        }

        /* Set socket address params */
        udp_entry->addr.sin_family = AF_INET;
        udp_entry->addr.sin_port = htons(udp_entry->port);
    }
    udp_initialized = FCE_TRUE;

}
void fce_cleanup()
{
    if (udp_initialized == FCE_FALSE )
        return;

    for (int i = 0; i < udp_sockets; i++)
    {
        struct udp_entry *udp_entry = udp_socket_list + i;

        /* Close any pending sockets */
        if (udp_entry->sock != -1)
        {
            close( udp_entry->sock );
            udp_entry->sock = -1;
        }
    }
    udp_initialized = FCE_FALSE;
}


/*
 * Construct a UDP packet for our listeners and return packet size
 * */
static unsigned short build_fce_packet( struct fce_packet *packet, char *path, int mode, uint32_t event_id )
{
    unsigned short data_len = 0;

    strncpy(packet->magic, FCE_PACKET_MAGIC, sizeof(packet->magic) );
    packet->version = FCE_PACKET_VERSION;
    packet->mode = mode;

    data_len = strlen( path );

    /* This should never happen, but before we bust this server, we send nonsense, fce listener has to cope */
    if (data_len >= FCE_MAX_PATH_LEN)
    {
        data_len = FCE_MAX_PATH_LEN - 1;
    }

    /* This is the payload len. Means: the stream has len bytes more until packet is finished */
    /* A server should read the first 16 byte, decode them and then fetch the rest */
    packet->len = htons( data_len);
    packet->event_id = htonl( event_id );

    strncpy( packet->data, path, data_len );

    /* return the packet len */
    return sizeof(struct fce_packet) - FCE_MAX_PATH_LEN + data_len;
}

/*
 * Send the fce information to all (connected) listeners
 * We dont give return code because all errors are handled internally (I hope..)
 * */
static void send_fce_event( char *path, int mode )
{    
    struct fce_packet packet;
    void *data = &packet;
    static uint32_t event_id = 0; /* the unique packet couter to detect packet/data loss. Going from 0xFFFFFFFF to 0x0 is a valid increment */

    time_t now = time(NULL);

    /* build our data packet */
    int data_len = build_fce_packet( &packet, path, mode, ++event_id );


    for (int i = 0; i < udp_sockets; i++)
    {
        int sent_data = 0;
        struct udp_entry *udp_entry = udp_socket_list + i;

        /* we had a problem earlier ? */
        if (udp_entry->sock == -1)
        {
            /* We still have to wait ?*/
            if (now < udp_entry->next_try_on_error)
                continue;

            /* Reopen socket */
            udp_entry->sock = socket( AF_INET, SOCK_DGRAM, 0 );

            if (udp_entry->sock == -1)
            {
                /* failed again, so go to rest again */
                LOG(log_error, logtype_afpd, "Cannot recreate socket for fce UDP connection: errno %d", errno  );

                udp_entry->next_try_on_error = now + FCE_SOCKET_RETRY_DELAY_S;
                continue;
            }

            udp_entry->next_try_on_error = 0;

            /* Okay, we have a running socket again, send server that we had a problem on our side*/
            data_len = build_fce_packet( &packet, "", FCE_CONN_BROKEN, 0 );

            sendto( udp_entry->sock, data, data_len, 0, &udp_entry->addr, sizeof(udp_entry->addr) );

            /* Rebuild our original data packet */
            data_len = build_fce_packet( &packet, path, mode, event_id );
        }

        sent_data =  sendto( udp_entry->sock, data, data_len, 0, &udp_entry->addr, sizeof(udp_entry->addr) );

        /* Problems ? */
        if (sent_data != data_len)
        {
            /* Argh, socket broke, we close and retry later */
            LOG(log_error, logtype_afpd, "Error while sending packet to %s for fce UDP connection: transfered: %d of %d errno %d",
                    udp_entry->port, sent_data, data_len, errno  );

            close( udp_entry->sock );
            udp_entry->sock = -1;
            udp_entry->next_try_on_error = now + FCE_SOCKET_RETRY_DELAY_S;
        }
    }
}

static int add_udp_socket( char *target_ip, int target_port )
{
    if (target_port == 0)
        target_port = FCE_DEFAULT_PORT;

    if (udp_sockets >= FCE_MAX_UDP_SOCKS)
    {
        LOG(log_error, logtype_afpd, "Too many file change api UDP connections (max %d allowed)", FCE_MAX_UDP_SOCKS );
        return AFPERR_PARAM;
    }

    strncpy( udp_socket_list[udp_sockets].ip, target_ip, FCE_MAX_IP_LEN - 1);
    udp_socket_list[udp_sockets].port = target_port;
    udp_socket_list[udp_sockets].sock = -1;
    memset( &udp_socket_list[udp_sockets].addr, 0, sizeof(struct sockaddr_in) );
    udp_socket_list[udp_sockets].next_try_on_error = 0;

    udp_sockets++;

    return AFP_OK;
}

/*
 *
 * Dispatcher for all incoming file change events
 *
 * */
static int register_fce( char *u_name, int is_dir, int mode )
{
    if (u_name == NULL)
        return AFPERR_PARAM;

    static int first_event = FCE_TRUE;

	/* do some initialization on the fly the first time */
	if (first_event)
	{
		fce_initialize_history();
	}


	/* handle files which should not cause events (.DS_Store atc. ) */
	for (int i = 0; skip_files[i] != NULL; i++)
	{
		if (!strcmp( u_name, skip_files[i]))
			return AFP_OK;
	}


	char full_path_buffer[FCE_MAX_PATH_LEN + 1] = {""};
	const char *cwd = getcwdpath();

	if (!is_dir || mode == FCE_DIR_DELETE)
	{
		if (strlen( cwd ) + strlen( u_name) + 1 >= FCE_MAX_PATH_LEN)
		{
			LOG(log_error, logtype_afpd, "FCE file name too long: %s/%s", cwd, u_name );
			return AFPERR_PARAM;
		}
		sprintf( full_path_buffer, "%s/%s", cwd, u_name );
	}
	else
	{
		if (strlen( cwd ) >= FCE_MAX_PATH_LEN)
		{
			LOG(log_error, logtype_afpd, "FCE directory name too long: %s", cwd);
			return AFPERR_PARAM;
		}
		strcpy( full_path_buffer, cwd);
	}

	/* Can we ignore this event based on type or history? */
	if (fce_handle_coalescation( full_path_buffer, is_dir, mode ))
	{
		LOG(log_debug9, logtype_afpd, "Coalesced fc event <%d> for <%s>", mode, full_path_buffer );
		return AFP_OK;
	}

	LOG(log_debug9, logtype_afpd, "Detected fc event <%d> for <%s>", mode, full_path_buffer );


    /* we do initilization on the fly, no blocking calls in here 
     * (except when using FQDN in broken DNS environment)
     */
    if (first_event == FCE_TRUE)
    {
        fce_init_udp();
        
        /* Notify listeners the we start from the beginning */
        send_fce_event( "", FCE_CONN_START );
        
        first_event = FCE_FALSE;
    }

	/* Handle UDP transport */
    send_fce_event( full_path_buffer, mode );

    return AFP_OK;
}


/******************** External calls start here **************************/

/*
 * API-Calls for file change api, called form outside (file.c directory.c ofork.c filedir.c)
 * */
#ifndef FCE_TEST_MAIN


int fce_register_delete_file( struct path *path )
{
    int ret = AFP_OK;

    if (path == NULL)
        return AFPERR_PARAM;

	
    ret = register_fce( path->u_name, FALSE, FCE_FILE_DELETE );

    return ret;
}
int fce_register_delete_dir( char *name )
{
    int ret = AFP_OK;

    if (name == NULL)
        return AFPERR_PARAM;

	
    ret = register_fce( name, TRUE, FCE_DIR_DELETE);

    return ret;
}

int fce_register_new_dir( struct path *path )
{
    int ret = AFP_OK;

    if (path == NULL)
        return AFPERR_PARAM;

    ret = register_fce( path->u_name, TRUE, FCE_DIR_CREATE );

    return ret;
}


int fce_register_new_file( struct path *path )
{
    int ret = AFP_OK;

    if (path == NULL)
        return AFPERR_PARAM;

    ret = register_fce( path->u_name, FALSE, FCE_FILE_CREATE );

    return ret;
}


int fce_register_file_modification( struct ofork *ofork )
{
    char *u_name = NULL;
    struct dir *dir;
    struct vol *vol;
    int ret = AFP_OK;

    if (ofork == NULL || ofork->of_vol == NULL || ofork->of_dir == NULL)
        return AFPERR_PARAM;

    vol = ofork->of_vol;
    dir = ofork->of_dir;

    if (NULL == (u_name = mtoupath(vol, of_name(ofork), dir->d_did, utf8_encoding()))) 
    {
        return AFPERR_MISC;
    }
    
    ret = register_fce( u_name, FALSE, FCE_FILE_MODIFY );
    
    return ret;    
}
#endif

/*
 *
 * Extern connect to afpd parameter, can be called multiple times for multiple listeners (up to MAX_UDP_SOCKS times)
 *
 * */
int fce_add_udp_socket( char *target )
{
	int port = FCE_DEFAULT_PORT;
	char target_ip[256] = {""};

	strncpy( target_ip, target, sizeof(target_ip) -1);
	char *port_delim = strchr( target_ip, ':' );
	if (port_delim)
	{
		*port_delim = 0;
		port = atoi( port_delim + 1);
	}
	return add_udp_socket( target_ip, port );
}



#ifdef FCE_TEST_MAIN


void shortsleep( unsigned int us )
{    
    usleep( us );
}
int main( int argc, char*argv[] )
{
    int port  = 11250;
    char *host = NULL;
    int delay_between_events = 1000;
    int event_code = FCE_FILE_MODIFY;
    char pathbuff[1024];
    int duration_in_seconds = 0; // TILL ETERNITY

    char *path = getcwd( pathbuff, sizeof(pathbuff) );

    // FULLSPEED TEST IS "-s 1001" -> delay is 0 -> send packets without pause

    if (argc == 1)
    {
        fprintf( stdout, "%s: -p Port -h Listener1 [ -h Listener2 ...] -P path -s Delay_between_events_in_us -e event_code -d Duration \n", argv[0]);
        exit( 1 );
    }
    int ret = AFP_OK;

    for (int i = 1; i < argc; i++)
    {
        char *p = argv[i];
        if (*p == '-' && p[1])
        {
            char *arg = argv[i + 1];
            switch (p[1])
            {
                case 'p': if (arg) port = atoi( arg ), i++; break;
                case 'P': if (arg) path =  arg, i++; break;
                case 's': if (arg) delay_between_events = atoi( arg ), i++; break;
                case 'e': if (arg) event_code = atoi( arg ), i++; break;
                case 'd': if (arg) duration_in_seconds = atoi( arg ), i++; break;
                case 'h':
                {
                    if (arg)
                    {
                        host = arg;
						char target[256];
						sprintf( target, "%s:%d", host, port );
                        ret += fce_add_udp_socket( target );
                        i++;
                    }
                    break;
                }               
            }
        }
    }
	

    if (host == NULL)
	{
		char target[256];
		sprintf( target, "127.0.0.1:%d", port );
		ret += fce_add_udp_socket( target );
	}

    if (ret)
        return ret;


    int ev_cnt = 0;
    time_t start_time = time(NULL);
    time_t end_time = 0;

    if (duration_in_seconds)
        end_time = start_time + duration_in_seconds;

    while (1)
    {
        time_t now = time(NULL);
        if (now > start_time)
        {
            start_time = now;
            fprintf( stdout, "%d events/s\n", ev_cnt );
            ev_cnt = 0;
        }
        if (end_time && now >= end_time)
            break;

        register_fce( path, event_code );
        ev_cnt++;

        
        shortsleep( delay_between_events );
    }
}
#endif /* TESTMAIN*/
