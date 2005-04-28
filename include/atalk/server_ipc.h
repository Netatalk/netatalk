
#include <atalk/server_child.h>

#define IPC_KILLTOKEN   1
#define IPC_GETSESSION  2

void *server_ipc_create(void);
int server_ipc_child(void *obj);
int server_ipc_parent(void *obj);
int server_ipc_read(server_child *children);
int server_ipc_write(u_int16_t command, int len, void *token);





