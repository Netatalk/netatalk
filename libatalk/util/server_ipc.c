/*
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif 

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include <atalk/server_child.h>
#include <atalk/server_ipc.h>
#include <atalk/logger.h>

typedef struct ipc_header {
	u_int16_t command;
        pid_t	  child_pid;
        uid_t     uid;
        u_int32_t len;
	char 	  *msg;
} ipc_header;

static int pipe_fd[2];
   
void *server_ipc_create(void)
{
    if (pipe(pipe_fd)) {
        return NULL;
    }
    return &pipe_fd;
}

/* ----------------- */
int server_ipc_child(void *obj _U_)
{
    /* close input */
    close(pipe_fd[0]);
    return pipe_fd[1];
}

/* ----------------- */
int server_ipc_parent(void *obj _U_)
{
    return pipe_fd[0];
}

/* ----------------- */
static int ipc_kill_token (struct ipc_header *ipc, server_child *children)
{
    pid_t pid;

    if (ipc->len != sizeof(pid_t)) {
        return -1;
    }
    /* assume signals SA_RESTART set */
    memcpy (&pid, ipc->msg, sizeof(pid_t));

    LOG(log_info, logtype_default, "child %d user %d disconnected", pid, ipc->uid);
    server_child_kill_one(children, CHILD_DSIFORK, pid, ipc->uid);
    return 0;
}

/* ----------------- */
static int ipc_get_session (struct ipc_header *ipc, server_child *children)
{
    u_int32_t boottime;
    u_int32_t idlen;
    char     *clientid, *p;


    if (ipc->len < (sizeof(idlen) + sizeof(boottime)) ) {
	return -1;
    }
    p = ipc->msg;
    memcpy (&idlen, p, sizeof(idlen));
    idlen = ntohl (idlen);
    p += sizeof(idlen); 

    memcpy (&boottime, p, sizeof(boottime));
    p += sizeof(boottime);
    
    if (ipc->len < idlen + sizeof(idlen) + sizeof(boottime)) {
	return -1;
    }
    if (NULL == (clientid = (char*) malloc(idlen)) ) {
	return -1;
    }
    memcpy (clientid, p, idlen);
  
    server_child_kill_one_by_id (children, CHILD_DSIFORK, ipc->child_pid, ipc->uid, idlen, clientid, boottime);
    /* FIXME byte to ascii if we want to log clientid */
    LOG (log_debug, logtype_afpd, "ipc_get_session: len: %u, idlen %d, time %x", ipc->len, idlen, boottime); 
    return 0;
}

#define IPC_HEADERLEN 14
#define IPC_MAXMSGSIZE 90

/* ----------------- 
 * Ipc format
 * command
 * pid
 * uid
 * 
*/
int server_ipc_read(server_child *children)
{
    int       ret = 0;
    struct ipc_header ipc;
    char      buf[IPC_MAXMSGSIZE], *p;

    if ((ret = read(pipe_fd[0], buf, IPC_HEADERLEN)) != IPC_HEADERLEN) {
	LOG (log_info, logtype_afpd, "Reading IPC header failed (%u of %u  bytes read)", ret, IPC_HEADERLEN);
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
    if (ipc.len > (IPC_MAXMSGSIZE - IPC_HEADERLEN))
    {
	LOG (log_info, logtype_afpd, "IPC message exceeds allowed size (%u)", ipc.len);
   	return -1;
    }

    memset (buf, 0, IPC_MAXMSGSIZE);
    if ( ipc.len != 0) {
	    if ((ret = read(pipe_fd[0], buf, ipc.len)) != (int) ipc.len) {
		LOG (log_info, logtype_afpd, "Reading IPC message failed (%u of %u  bytes read)", ret, ipc.len);
		return -1;
    	}	 
    }
    ipc.msg = buf;
    
    LOG (log_debug, logtype_afpd, "ipc_read: command: %u, pid: %u, len: %u", ipc.command, ipc.child_pid, ipc.len); 

    switch (ipc.command)
    {
	case IPC_KILLTOKEN:
		return (ipc_kill_token(&ipc, children));
		break;
	case IPC_GETSESSION:
		return (ipc_get_session(&ipc, children));
		break;
	default:
		LOG (log_info, logtype_afpd, "ipc_read: unknown command: %d", ipc.command);
		return -1;
    }

}

/* ----------------- */
int server_ipc_write( u_int16_t command, int len, void *msg)
{
   char block[IPC_MAXMSGSIZE], *p;
   pid_t pid;
   uid_t uid;
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

   LOG (log_debug, logtype_afpd, "ipc_write: command: %u, pid: %u, msglen: %u", command, pid, len); 
   return write(pipe_fd[1], block, len+IPC_HEADERLEN );
}
