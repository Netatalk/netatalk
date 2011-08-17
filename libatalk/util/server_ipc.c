/*
 * All rights reserved. See COPYRIGHT.
 *
 * IPC over socketpair between parent and children.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif 

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <atalk/server_child.h>
#include <atalk/server_ipc.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/errchk.h>
#include <atalk/paths.h>
#include <atalk/globals.h>
#include <atalk/dsi.h>

#define IPC_HEADERLEN 14
#define IPC_MAXMSGSIZE 90

typedef struct ipc_header {
	uint16_t command;
    pid_t	 child_pid;
    uid_t    uid;
    uint32_t len;
	char 	 *msg;
    int      afp_socket;
    uint16_t DSI_requestID;
} ipc_header_t;

static char *ipc_cmd_str[] = { "IPC_DISCOLDSESSION",
                               "IPC_GETSESSION"};

/*
 * Pass afp_socket to old disconnected session if one has a matching token (token = pid)
 * @returns -1 on error, 0 if no matching session was found, 1 if session was found and socket passed
 */
static int ipc_kill_token(struct ipc_header *ipc, server_child *children)
{
    pid_t pid;

    if (ipc->len != sizeof(pid_t)) {
        return -1;
    }
    /* assume signals SA_RESTART set */
    memcpy (&pid, ipc->msg, sizeof(pid_t));

    return server_child_transfer_session(children,
                                         CHILD_DSIFORK,
                                         pid,
                                         ipc->uid,
                                         ipc->afp_socket,
                                         ipc->DSI_requestID);
}

/* ----------------- */
static int ipc_get_session(struct ipc_header *ipc, server_child *children)
{
    u_int32_t boottime;
    u_int32_t idlen;
    char     *clientid, *p;

    
    if (ipc->len < (sizeof(idlen) + sizeof(boottime)) )
        return -1;

    p = ipc->msg;
    memcpy (&idlen, p, sizeof(idlen));
    idlen = ntohl (idlen);
    p += sizeof(idlen); 

    memcpy (&boottime, p, sizeof(boottime));
    p += sizeof(boottime);
    
    if (ipc->len < idlen + sizeof(idlen) + sizeof(boottime))
        return -1;

    if (NULL == (clientid = (char*) malloc(idlen)) )
        return -1;
    memcpy (clientid, p, idlen);
  
    LOG(log_debug, logtype_afpd, "ipc_get_session(pid: %u, uid: %u, time: 0x%08x)",
        ipc->child_pid, ipc->uid, boottime); 

    server_child_kill_one_by_id(children,
                                CHILD_DSIFORK,
                                ipc->child_pid,
                                ipc->uid,
                                idlen,
                                clientid,
                                boottime);

    return 0;
}

/***********************************************************************************
 * Public functions
 ***********************************************************************************/

/*!
 * Listen on UNIX domain socket "name" for IPC from old sesssion
 *
 * @args name    (r) file name to use for UNIX domain socket
 * @returns      socket fd, -1 on error
 */
int ipc_server_uds(const char *name)
{
    EC_INIT;
    struct sockaddr_un address;
    socklen_t address_length;
    int fd = -1;

    EC_NEG1_LOG( fd = socket(PF_UNIX, SOCK_STREAM, 0) );
    EC_ZERO_LOG( setnonblock(fd, 1) );
    unlink(name);
    address.sun_family = AF_UNIX;
    address_length = sizeof(address.sun_family) + sprintf(address.sun_path, "%s", name);
    EC_ZERO_LOG( bind(fd, (struct sockaddr *)&address, address_length) );
    EC_ZERO_LOG( listen(fd, 1024) );

EC_CLEANUP:
    if (ret != 0) {
        return -1;
    }

    return fd;
}

/*!
 * Connect to UNIX domain socket "name" for IPC with new afpd master
 *
 * 1. Connect
 * 2. send pid, which establishes a child structure for us in the master
 *
 * @args name    (r) file name to use for UNIX domain socket
 * @returns      socket fd, -1 on error
 */
int ipc_client_uds(const char *name)
{
    EC_INIT;
    struct sockaddr_un address;
    socklen_t address_length;
    int fd = -1;
    pid_t pid = getpid();

    EC_NEG1_LOG( fd = socket(PF_UNIX, SOCK_STREAM, 0) );
    address.sun_family = AF_UNIX;
    address_length = sizeof(address.sun_family) + sprintf(address.sun_path, "%s", name);

    EC_ZERO_LOG( connect(fd, (struct sockaddr *)&address, address_length) ); /* 1 */
    LOG(log_debug, logtype_afpd, "ipc_client_uds: connected to master");

    EC_ZERO_LOG( setnonblock(fd, 1) );

    if (writet(fd, &pid, sizeof(pid_t), 0, 1) != sizeof(pid_t)) {
        LOG(log_error, logtype_afpd, "ipc_client_uds: writet: %s", strerror(errno));
        EC_FAIL;
    }

EC_CLEANUP:
    if (ret != 0) {
        return -1;
    }
    LOG(log_debug, logtype_afpd, "ipc_client_uds: fd: %d", fd);
    return fd;
}

int reconnect_ipc(AFPObj *obj)
{
    int retrycount = 0;

    LOG(log_debug, logtype_afpd, "reconnect_ipc: start");

    close(obj->ipc_fd);
    obj->ipc_fd = -1;

    sleep((getpid() % 5) + 15);  /* give it enough time to start */

    while (retrycount++ < 10) {
        if ((obj->ipc_fd = ipc_client_uds(_PATH_AFP_IPC)) == -1) {
            LOG(log_error, logtype_afpd, "reconnect_ipc: cant reconnect to master");
            sleep(1);
            continue;
        }
        LOG(log_debug, logtype_afpd, "reconnect_ipc: succesfull IPC reconnect");
        return 0;
    }
    return -1;
}

/* ----------------- 
 * Ipc format
 * command
 * pid
 * uid
 * 
 */

/*!
 * Read a IPC message from a child
 *
 * This is using an fd with non-blocking IO, so EAGAIN is not an error
 *
 * @args children  (rw) pointer to our structure with all childs
 * @args fd        (r)  IPC socket with child
 *
 * @returns -1 on error, 0 on success
 */
int ipc_server_read(server_child *children, int fd)
{
    int       ret;
    struct ipc_header ipc;
    char      buf[IPC_MAXMSGSIZE], *p;

    if ((ret = read(fd, buf, IPC_HEADERLEN)) != IPC_HEADERLEN) {
        if (ret != 0) {
            if (errno == EAGAIN)
                return 0;
            LOG(log_error, logtype_afpd, "Reading IPC header failed (%i of %u bytes read): %s",
                ret, IPC_HEADERLEN, strerror(errno));
        }
        return -1;
    }

    p = buf;

    memcpy(&ipc.command, p, sizeof(ipc.command));
    p += sizeof(ipc.command);

    memcpy(&ipc.child_pid, p, sizeof(ipc.child_pid));
    p += sizeof(ipc.child_pid);

    memcpy(&ipc.uid, p, sizeof(ipc.uid));
    p += sizeof(ipc.uid);

    memcpy(&ipc.len, p, sizeof(ipc.len));

    /* This should never happen */
    if (ipc.len > (IPC_MAXMSGSIZE - IPC_HEADERLEN)) {
        LOG (log_info, logtype_afpd, "IPC message exceeds allowed size (%u)", ipc.len);
        return -1;
    }

    memset (buf, 0, IPC_MAXMSGSIZE);
    if ( ipc.len != 0) {
	    if ((ret = read(fd, buf, ipc.len)) != (int) ipc.len) {
            LOG(log_info, logtype_afpd, "Reading IPC message failed (%u of %u  bytes read): %s",
                ret, ipc.len, strerror(errno));
            return -1;
    	}	 
    }
    ipc.msg = buf;

    LOG(log_debug, logtype_afpd, "ipc_server_read(%s): pid: %u",
        ipc_cmd_str[ipc.command], ipc.child_pid); 

    int afp_socket;

    switch (ipc.command) {

	case IPC_DISCOLDSESSION:
        if (readt(fd, &ipc.DSI_requestID, 2, 0, 2) != 2) {
            LOG (log_error, logtype_afpd, "ipc_read(%s:child[%u]): couldnt read DSI id: %s",
                 ipc_cmd_str[ipc.command], ipc.child_pid, strerror(errno));
            return -1;
        }
        if ((ipc.afp_socket = recv_fd(fd, 1)) == -1) {
            LOG (log_error, logtype_afpd, "ipc_read(%s:child[%u]): recv_fd: %s",
                 ipc_cmd_str[ipc.command], ipc.child_pid, strerror(errno));
            return -1;
        }
		if (ipc_kill_token(&ipc, children) == 1) {
            /* Transfered session (ie afp_socket) to old disconnected child, now kill the new one */
            LOG(log_note, logtype_afpd, "Reconnect: killing new session child[%u] after transfer",
                ipc.child_pid);
            kill(ipc.child_pid, SIGTERM);
        }        
        close(ipc.afp_socket);
        break;

	case IPC_GETSESSION:
		if (ipc_get_session(&ipc, children) != 0)
            return -1;
        break;

	default:
		LOG (log_info, logtype_afpd, "ipc_read: unknown command: %d", ipc.command);
		return -1;
    }

    return 0;
}

/* ----------------- */
int ipc_child_write(int fd, uint16_t command, int len, void *msg)
{
   char block[IPC_MAXMSGSIZE], *p;
   pid_t pid;
   uid_t uid;
   ssize_t ret;

   p = block;

   memset ( p, 0 , IPC_MAXMSGSIZE);
   if (len + IPC_HEADERLEN > IPC_MAXMSGSIZE)
       return -1;

   memcpy(p, &command, sizeof(command));
   p   += sizeof(command);

   pid = getpid();
   memcpy(p, &pid, sizeof(pid_t));
   p += sizeof(pid_t);
   
   /* FIXME 
    * using uid is wrong. It will not disconnect if the new connection
    * is with a different user. 
    * But we really don't want a remote kill command.
   */
   uid = geteuid();
   memcpy(p, &uid, sizeof(uid_t));
   p += sizeof(uid_t);

   memcpy(p, &len, 4);
   p += 4;

   memcpy(p, msg, len);

   LOG(log_debug, logtype_afpd, "ipc_child_write(%s)", ipc_cmd_str[command]);

   if ((ret = writet(fd, block, len+IPC_HEADERLEN, 0, 1)) != len + IPC_HEADERLEN) {
       return -1;
   }

   return 0;
}

