/* 
 * File:   fce_api_internal.h
 * Author: mw
 *
 * Created on 1. Oktober 2010, 23:48
 */

#ifndef _FCE_API_INTERNAL_H
#define	_FCE_API_INTERNAL_H

#define FCE_MAX_UDP_SOCKS 5     /* Allow a maximum of udp listeners for file change events */
#define FCE_SOCKET_RETRY_DELAY_S 600 /* Pause this time in s after socket was broken */
#define FCE_PACKET_VERSION  1
#define FCE_HISTORY_LEN 10  /* This is used to coalesce events */
#define MAX_COALESCE_TIME_MS 1000  /* Events oldeer than this are not coalesced */

#define FCE_COALESCE_CREATE (1 << 0)
#define FCE_COALESCE_DELETE (1 << 1)
#define FCE_COALESCE_ALL    (FCE_COALESCE_CREATE | FCE_COALESCE_DELETE)

struct udp_entry
{
    int sock;
    char *addr;
    char *port;
    struct addrinfo addrinfo;
    struct sockaddr_storage sockaddr;
    time_t next_try_on_error;      /* In case of error set next timestamp to retry */
};

struct fce_history
{
    unsigned char mode;
	int is_dir;
	char path[MAXPATHLEN + 1];
	struct timeval tv;
};

struct fce_close_event {
    time_t time;
	char path[MAXPATHLEN + 1];
};

#define PACKET_HDR_LEN (sizeof(struct fce_packet) - FCE_MAX_PATH_LEN)

int fce_handle_coalescation( char *path, int is_dir, int mode );
void fce_initialize_history();


#endif	/* _FCE_API_INTERNAL_H */

