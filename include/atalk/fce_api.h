/* 
 * File:   fce_api.h
 * Author: mw
 *
 * Created on 1. Oktober 2010, 21:35
 *
 * API calls for file change event api
 */

#ifndef _FCE_API_H
#define	_FCE_API_H

#include <atalk/globals.h>

#define FCE_PACKET_VERSION  2

/* fce_packet.mode */
#define FCE_FILE_MODIFY     1
#define FCE_FILE_DELETE     2
#define FCE_DIR_DELETE      3
#define FCE_FILE_CREATE     4
#define FCE_DIR_CREATE      5
#define FCE_FILE_MOVE       6
#define FCE_DIR_MOVE        7
#define FCE_LOGIN           8
#define FCE_LOGOUT          9
#define FCE_CONN_START     42
#define FCE_CONN_BROKEN    99

#define FCE_FIRST_EVENT     FCE_FILE_MODIFY /* keep in sync with last file event above */
#define FCE_LAST_EVENT      FCE_LOGOUT      /* keep in sync with last file event above */

/* fce_packet.fce_magic */
#define FCE_PACKET_MAGIC  "at_fcapi"

/* flags for "fce_ev_info" of additional info to send in events */
#define FCE_EV_INFO_PID     (1 << 0)
#define FCE_EV_INFO_USER    (1 << 1)
#define FCE_EV_INFO_SRCPATH (1 << 2)

/*
 * Network payload of an FCE packet, version 1
 *
 *      1         2         3         4         5         6         7          8
 * +---------+---------+---------+---------+---------+---------+----------+----------+
 * |                                   FCE magic                                     |
 * +---------+---------+---------+---------+---------+---------+----------+----------+
 * | version |
 * +---------+
 * |  event  |
 * +---------+-----------------------------+
 * |               event ID                |
 * +-------------------+-------------------+ . . . .
 * |     pathlen       | path
 * +-------------------+------ . . . . . . . . . . .
 *
 *
 * Network payload of an FCE packet, version 2
 *
 *      1         2         3         4         5         6         7          8
 * +---------+---------+---------+---------+---------+---------+----------+----------+
 * |                                   FCE magic                                     |
 * +---------+---------+---------+---------+---------+---------+----------+----------+
 * | version |
 * +---------+
 * | options |
 * +---------+
 * |  event  |
 * +---------+
 * | padding |
 * +---------+---------+---------+---------+---------+---------+----------+----------+
 * |                                    reserved                                     |
 * +---------+---------+---------+---------+---------+---------+----------+----------+
 * |               event ID                |
 * +---------+---------+---------+---------+
 * ... optional:
 * +---------+---------+---------+---------+---------+---------+----------+----------+
 * |                                      pid                                        |
 * +---------+---------+---------+---------+---------+---------+----------+----------+
 * ...
 * ... optional:
 * +-------------------+----------  . . . .
 * |  username length  | username
 * +-------------------+----------  . . . .
 * ...
 * +-------------------+------  . . . . . .
 * |     pathlen       | path
 * +-------------------+------  . . . . . .
 * ... optional:
 * +-------------------+------------- . . .
 * |     pathlen       | source path
 * +-------------------+------------- . . .
 *
 * version      = 2
 * options      = bitfield:
 *                    0: pid present
 *                    1: username present
 *                    2: source path present
 * pid          = optional pid
 * username     = optional username
 * source path  = optional source path
 */

struct fce_packet {
    char          fcep_magic[8];
    unsigned char fcep_version;
    unsigned char fcep_options;
    unsigned char fcep_event;
    uint32_t      fcep_event_id;
    uint64_t      fcep_pid;
    uint16_t      fcep_userlen;
    char          fcep_user[MAXPATHLEN];
    uint16_t      fcep_pathlen1;
    char          fcep_path1[MAXPATHLEN];
    uint16_t      fcep_pathlen2;
    char          fcep_path2[MAXPATHLEN];
};

typedef uint32_t fce_ev_t;

struct path;
struct ofork;

void fce_pending_events(const AFPObj *obj);
int fce_register(const AFPObj *obj, fce_ev_t event, const char *path, const char *oldpath);
int fce_add_udp_socket(const char *target );  // IP or IP:Port
int fce_set_coalesce(const char *coalesce_opt ); // all|delete|create
int fce_set_events(const char *events);     /* fmod,fdel,ddel,fcre,dcre */

#define FCE_DEFAULT_PORT 12250
#define FCE_DEFAULT_PORT_STRING "12250"

#endif	/* _FCE_API_H */

