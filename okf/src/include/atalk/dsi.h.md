---
type: C Header File
title: "include/atalk/dsi.h"
description: "DSI (Data Stream Interface) protocol definitions."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/dsi.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Declared at include/atalk/dsi.h line 167; no definition in the scanned sources.

### dsi_kill

```c
void dsi_kill(int)
```

Declared at include/atalk/dsi.h line 178; no definition in the scanned sources.

# Types

### struct DSI

Defined at line 58.
* `struct DSI * next`
* `AFPObj * AFPobj`
* `int statuslen`
* `char status`
* `char * signature`
* `struct dsi_block header`
* `struct sockaddr_storage server client`
* `struct itimerval timer`
* `int tickle`
* `int in_write`
* `int msg_request`
* `int down_request`
* `uint32_t attn_quantum`
* `uint32_t datasize`
* `uint32_t server_quantum`
* `uint16_t serverID`
* `uint16_t clientID`
* `uint8_t * commands`
* `uint8_t data`
* `size_t datalen`
* `size_t cmdlen`
* `off_t read_count`
* `off_t write_count`
* `uint32_t flags`
* `int socket`
* `int serversock`
* `size_t dsireadbuf`
* `char * buffer`
* `char * start`
* `char * eof`
* `char * end`
* `pid_t(* proto_open`: Called through by [dsi_getsession](../../libatalk/dsi/dsi_getsess.c.md#dsi_getsession).
* `void(* proto_close`: Called through by [dsi_close](../../libatalk/dsi/dsi_close.c.md#dsi_close), [dsi_disconnect](../../libatalk/dsi/dsi_stream.c.md#dsi_disconnect), [dsi_getsession](../../libatalk/dsi/dsi_getsess.c.md#dsi_getsession), [handle_transfer_session](../../etc/afpd/afp_dsi.c.md#handle_transfer_session).

### struct dsi_block

Defined at line 42.
* `uint8_t dsi_flags`
* `uint8_t dsi_command`
* `uint16_t dsi_requestID`
* `uint32_t dsi_code`
* `uint32_t dsi_doff`
* `union dsi_block dsi_data`
* `uint32_t dsi_len`
* `uint32_t dsi_reserved`

# Typedefs and enums

* `typedef struct DSI DSI`

# Macros

* Undocumented: `DSIERR_BADVERS`, `DSIERR_BUFSMALL`, `DSIERR_NOACK`, `DSIERR_NOSERV`, `DSIERR_NOSESS`, `DSIERR_OK`, `DSIERR_PARM`, `DSIERR_SERVBUSY`, `DSIERR_SESSCLOS`, `DSIERR_SIZERR`, `DSIERR_TOOMANY`, `DSIFL_MAX`, `DSIFL_REPLY`, `DSIFL_REQUEST`, `DSIFUNC_ATTN`, `DSIFUNC_CLOSE`, `DSIFUNC_CMD`, `DSIFUNC_MAX`, `DSIFUNC_OPEN`, `DSIFUNC_STAT`, `DSIFUNC_TICKLE`, `DSIFUNC_WRITE`, `DSIOPT_ATTNQUANT`, `DSIOPT_REPLCSIZE`, `DSIOPT_SERVQUANT`, `DSI_AFPOVERTCP_PORT`, `DSI_AFP_LOGGED_OUT`, `DSI_BLOCKSIZ`, `DSI_DATA`, `DSI_DATASIZ`, `DSI_DEFQUANT`, `DSI_DIE`, `DSI_DISCONNECTED`, `DSI_EXTSLEEP`, `DSI_FRAME_OVERHEAD_EXT`, `DSI_MSG_MORE`, `DSI_NOREPLY`, `DSI_NOWAIT`, `DSI_QUANTUM_GROWTH_MAX`, `DSI_RECONINPROG`, `DSI_RECONSOCKET`, `DSI_RUNNING`, `DSI_SERVQUANT_DEF`, `DSI_SERVQUANT_MAX`, `DSI_SERVQUANT_MIN`, `DSI_SLEEPING`, `DSI_WROFF_FPWRITE`, `DSI_WROFF_FPWRITEEXT`, `dsi_send`, `dsi_serverID`, `dsi_wrtreply`
