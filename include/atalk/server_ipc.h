#ifndef ATALK_SERVER_IPC_H
#define ATALK_SERVER_IPC_H

#include <stdint.h>

#include <atalk/cnid.h>
#include <atalk/globals.h>
#include <atalk/server_child.h>

/* IPC wire format constants — moved from server_ipc.c so receivers (Eg dircache.c)
 * can parse the wire format. 14-byte header: cmd(2) + pid(4) + uid(4) + len(4) */
#define IPC_HEADERLEN  14
#define IPC_MAXMSGSIZE 90

/* Remember to add IPC commands to server_ipc.c:ipc_cmd_str[] */
#define IPC_DISCOLDSESSION   0
#define IPC_GETSESSION       1
#define IPC_STATE            2  /*!< pass AFP session state */
#define IPC_VOLUMES          3  /*!< pass list of open volumes */
#define IPC_LOGINDONE        4
#define IPC_CACHE_HINT       5  /*!< Cross-process dircache invalidation hint */

/* Cache hint types — carried in struct ipc_cache_hint_payload.event */
#define CACHE_HINT_REFRESH          0  /*!< ostat + dir_modify(DCMOD_STAT) */
#define CACHE_HINT_DELETE           1  /*!< direct dir_remove() by CNID */
#define CACHE_HINT_DELETE_CHILDREN  2  /*!< dircache_remove_children() + remove/refresh parent */

/* Hint payload — sent inside standard IPC wire header framing.
 * Packed to guarantee sizeof == 8. vid and cnid in network byte order. */
struct __attribute__((packed)) ipc_cache_hint_payload {
    uint8_t   event;       /* Hint type: CACHE_HINT_REFRESH/DELETE/DELETE_CHILDREN */
    uint8_t   reserved;    /* Alignment padding (could be version field in future) */
    uint16_t  vid;         /* Volume ID — network byte order (matches vol->v_vid) */
    cnid_t    cnid;        /* CNID of affected file/dir — network byte order */
};

_Static_assert(sizeof(struct ipc_cache_hint_payload) == 8,
               "Hint payload must be exactly 8 bytes");

extern int ipc_server_read(server_child_t *children, int fd);
extern int ipc_child_write(int fd, uint16_t command, size_t len, void *token);
extern int ipc_child_state(AFPObj *obj, uint16_t state);

extern int ipc_send_cache_hint(const AFPObj *obj, uint16_t vid, cnid_t cnid,
                               uint8_t event);
extern unsigned long long ipc_get_hints_sent(void);

#endif /* ATALK_SERVER_IPC_H */
