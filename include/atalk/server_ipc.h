#ifndef ATALK_SERVER_IPC_H
#define ATALK_SERVER_IPC_H

#include <atalk/server_child.h>
#include <atalk/globals.h>

#define IPC_DISCOLDSESSION   0
#define IPC_GETSESSION       1

extern int ipc_server_uds(const char *name);
extern int ipc_client_uds(const char *name);
extern int reconnect_ipc(AFPObj *);
extern int ipc_server_read(server_child *children, int fd);
extern int ipc_child_write(int fd, uint16_t command, int len, void *token);

#endif /* IPC_GETSESSION_LOGIN */
