/* 
 * File:   fce_api_internal.h
 * Author: mw
 *
 * Created on 1. Oktober 2010, 23:48
 */

#ifndef _FCE_API_INTERNAL_H
#define	_FCE_API_INTERNAL_H

/* fce_packet.mode */
#define FCE_FILE_MODIFY     1
#define FCE_FILE_DELETE     2
#define FCE_DIR_DELETE      3
#define FCE_FILE_CREATE     4
#define FCE_DIR_CREATE      5
#define FCE_CONN_START     42
#define FCE_CONN_BROKEN    99

/* fce_packet.fce_magic */
#define FCE_PACKET_MAGIC  "at_fcapi"

#define FCE_MAX_PATH_LEN 1024
#define FCE_MAX_UDP_SOCKS 5     /* Allow a maximum of udp listeners for file change events */
#define FCE_MAX_IP_LEN 255      /* Man len of listener name */
#define FCE_SOCKET_RETRY_DELAY_S 600 /* Pause this time in s after socket was broken */
#define FCE_PACKET_VERSION  1
#define FCE_HISTORY_LEN 10  /* This is used to coalesce events */
#define MAX_COALESCE_TIME_MS 1000  /* Events oldeer than this are not coalesced */

struct udp_entry
{
    int sock;
    char ip[FCE_MAX_IP_LEN];
    int port;
    struct sockaddr_in addr;
    time_t next_try_on_error;      /* In case of error set next timestamp to retry */
};

/* This packet goes over the network, so we want to
 *  be shure about datastructs and type sizes between platforms
 */
struct fce_packet
{
    char magic[8];
    unsigned char version;
    unsigned char mode;
    uint16_t len;  /* network byte order */
    uint32_t event_id; /* network byte order */
    char data[FCE_MAX_PATH_LEN];
};

struct fce_history
{
    unsigned char mode;
	int is_dir;
	char path[FCE_MAX_PATH_LEN + 1];
	struct timeval tv;
};


#define PACKET_HDR_LEN (sizeof(struct fce_packet) - FCE_MAX_PATH_LEN)

int fce_handle_coalescation( char *path, int is_dir, int mode );
void fce_initialize_history();


#endif	/* _FCE_API_INTERNAL_H */

