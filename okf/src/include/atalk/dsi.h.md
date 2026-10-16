---
type: C Header File
title: "include/atalk/dsi.h"
description: "Data Stream Interface packet types, session state and I/O operations."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/dsi.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](afp.h.md)
* [atalk/globals.h](globals.h.md)
* [atalk/server_child.h](server_child.h.md)
* System headers: `arpa/inet.h`, `netinet/in.h`, `signal.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Included by

* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/afp_config.h](../../etc/afpd/afp_config.h.md)
* [etc/afpd/afp_dsi.c](../../etc/afpd/afp_dsi.c.md)
* [etc/afpd/afpstats.c](../../etc/afpd/afpstats.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/fork.c](../../etc/afpd/fork.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/messages.c](../../etc/afpd/messages.c.md)
* [etc/afpd/spotlight_marshalling.c](../../etc/afpd/spotlight_marshalling.c.md)
* [etc/afpd/status.c](../../etc/afpd/status.c.md)
* [etc/afpd/status.h](../../etc/afpd/status.h.md)
* [etc/afpd/uam.c](../../etc/afpd/uam.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/netatalk/afp_avahi.c](../../etc/netatalk/afp_avahi.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [etc/papd/uam.c](../../etc/papd/uam.c.md)
* [libatalk/adouble/ad_conv.c](../../libatalk/adouble/ad_conv.c.md)
* [libatalk/adouble/ad_recvfile.c](../../libatalk/adouble/ad_recvfile.c.md)
* [libatalk/asp/asp_getsess.c](../../libatalk/asp/asp_getsess.c.md)
* [libatalk/dsi/dsi_attn.c](../../libatalk/dsi/dsi_attn.c.md)
* [libatalk/dsi/dsi_close.c](../../libatalk/dsi/dsi_close.c.md)
* [libatalk/dsi/dsi_cmdreply.c](../../libatalk/dsi/dsi_cmdreply.c.md)
* [libatalk/dsi/dsi_getsess.c](../../libatalk/dsi/dsi_getsess.c.md)
* [libatalk/dsi/dsi_getstat.c](../../libatalk/dsi/dsi_getstat.c.md)
* [libatalk/dsi/dsi_init.c](../../libatalk/dsi/dsi_init.c.md)
* [libatalk/dsi/dsi_opensess.c](../../libatalk/dsi/dsi_opensess.c.md)
* [libatalk/dsi/dsi_read.c](../../libatalk/dsi/dsi_read.c.md)
* [libatalk/dsi/dsi_stream.c](../../libatalk/dsi/dsi_stream.c.md)
* [libatalk/dsi/dsi_tcp.c](../../libatalk/dsi/dsi_tcp.c.md)
* [libatalk/dsi/dsi_tickle.c](../../libatalk/dsi/dsi_tickle.c.md)
* [libatalk/dsi/dsi_write.c](../../libatalk/dsi/dsi_write.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)
* [libatalk/util/server_ipc.c](../../libatalk/util/server_ipc.c.md)

# Functions

### dsi_setstatus

```c
void dsi_setstatus(DSI *, char *, const size_t)
```

Declared at include/atalk/dsi.h line 154; no definition in the scanned sources.

### dsi_kill

```c
void dsi_kill(int)
```

Declared at include/atalk/dsi.h line 165; no definition in the scanned sources.

# Types

### struct DSI

Defined at line 42.
* `struct DSI * next`: multiple listening addresses
* `AFPObj * AFPobj`
* `int statuslen`
* `char status`
* `char * signature`
* `struct dsi_block header`
* `struct sockaddr_storage server client`
* `struct itimerval timer`
* `int tickle`: tickle count
* `int in_write`: a reply under way or left unfinished
* `int msg_request`: pending message to the client
* `int down_request`: pending SIGUSR1 down in 5 mn
* `uint32_t attn_quantum`
* `uint32_t datasize`
* `uint32_t server_quantum`
* `uint16_t serverID`
* `uint16_t clientID`
* `uint8_t * commands`: [DSI](dsi.h.md#struct-dsi) recieve buffer.
* `uint8_t data`: [DSI](dsi.h.md#struct-dsi) reply buffer.
* `size_t datalen`
* `size_t cmdlen`
* `off_t read_count`
* `off_t write_count`
* `uint32_t flags`: [DSI](dsi.h.md#struct-dsi) flags like DSI_SLEEPING, DSI_DISCONNECTED.
* `int socket`: AFP session socket.
* `int serversock`: listening socket
* `int splice_pipe`: recvfile pipe, opened on first use
* `int splice_size`: recvfile round size, 0 once declined
* `volatile sig_atomic_t * stop_wait`: points at the flag that ends a wait for input once set, or NULL
* `size_t dsireadbuf`: size of the [DSI](dsi.h.md#struct-dsi) readahead buffer used in [dsi_peek()](../../libatalk/dsi/dsi_stream.c.md#dsi_peek)
* `char * buffer`: buffer start
* `char * start`: current buffer head
* `char * eof`: end of currently used buffer
* `char * end`
* `pid_t(* proto_open`: Called through by [dsi_getsession](../../libatalk/dsi/dsi_getsess.c.md#dsi_getsession).
* `void(* proto_close`: Called through by [dsi_close](../../libatalk/dsi/dsi_close.c.md#dsi_close), [dsi_disconnect](../../libatalk/dsi/dsi_stream.c.md#dsi_disconnect), [dsi_getsession](../../libatalk/dsi/dsi_getsess.c.md#dsi_getsession), [handle_transfer_session](../../etc/afpd/afp_dsi.c.md#handle_transfer_session).

### struct dsi_block

Defined at line 26.
* `uint8_t dsi_flags`: packet type: request or reply
* `uint8_t dsi_command`: command
* `uint16_t dsi_requestID`: request ID
* `uint32_t dsi_code`: error code
* `uint32_t dsi_doff`: write data offset within the payload
* `union dsi_block dsi_data`
* `uint32_t dsi_len`: payload length, excluding the [DSI](dsi.h.md#struct-dsi) header
* `uint32_t dsi_reserved`: reserved field

# Typedefs and enums

* `typedef struct DSI DSI`

# Macros

* `DSIFUNC_ATTN`: DSIAttention.
* `DSIFUNC_CLOSE`: DSICloseSession.
* `DSIFUNC_CMD`: DSICommand.
* `DSIFUNC_MAX`: largest command
* `DSIFUNC_OPEN`: DSIOpenSession.
* `DSIFUNC_STAT`: DSIGetStatus.
* `DSIFUNC_TICKLE`: DSITickle.
* `DSIFUNC_WRITE`: DSIWrite.
* `DSIOPT_ATTNQUANT`: attention quantum
* `DSIOPT_REPLCSIZE`: AFP replaycache size supported by the server (that's us).
* `DSIOPT_SERVQUANT`: server request quantum
* `DSI_AFPOVERTCP_PORT`: default port number
* `DSI_AFP_LOGGED_OUT`: client called afp_logout, quit on next EOF from socket
* `DSI_DATA`: we have received a [DSI](dsi.h.md#struct-dsi) command
* `DSI_DEFQUANT`: default attention quantum size
* `DSI_DIE`: SIGUSR1, going down in 5 minutes.
* `DSI_DISCONNECTED`: we're in diconnected state after a socket error
* `DSI_EXTSLEEP`: we're sleeping after FPZzzzz
* `DSI_NOREPLY`: in dsi_write we generate our own replies
* `DSI_QUANTUM_GROWTH_MAX`: max growth of the session quantum during MSS alignment (256 KiB)
* `DSI_RECONINPROG`: used in the new session in reconnect
* `DSI_RECONSOCKET`: we have a new socket from primary reconnect
* `DSI_RUNNING`: we have received a AFP command
* `DSI_SERVQUANT_DEF`: default server quantum (1 MB)
* `DSI_SERVQUANT_MAX`: maximum server quantum (256 MiB)
* `DSI_SERVQUANT_MIN`: minimum server quantum
* `DSI_SLEEPING`: we're sleeping after FPZzzzz
* Undocumented: `DSIERR_BADVERS`, `DSIERR_BUFSMALL`, `DSIERR_NOACK`, `DSIERR_NOSERV`, `DSIERR_NOSESS`, `DSIERR_OK`, `DSIERR_PARM`, `DSIERR_SERVBUSY`, `DSIERR_SESSCLOS`, `DSIERR_SIZERR`, `DSIERR_TOOMANY`, `DSIFL_MAX`, `DSIFL_REPLY`, `DSIFL_REQUEST`, `DSI_BLOCKSIZ`, `DSI_DATASIZ`, `DSI_FRAME_OVERHEAD_EXT`, `DSI_NOWAIT`, `DSI_WROFF_FPWRITE`, `DSI_WROFF_FPWRITEEXT`, `dsi_send`, `dsi_serverID`, `dsi_wrtreply`
