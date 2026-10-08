---
type: C Source File
title: "bin/pap/pap.c"
description: "6 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/pap/pap.c"
tags: ["bin/pap"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/pap](../pap.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/pap.h](../../include/atalk/pap.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `fcntl.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### copy_status

```c
static int copy_status(const struct iovec *response, char *status_buf, size_t status_buf_size, size_t *status_len)
```

Defined at lines 39 to 60.

Called by: [main](pap.c.md#main), [send_file](pap.c.md#send_file)

Uses file-scope variables: `data`

### usage

```c
static void usage(char *path)
```

Defined at lines 62 to 87.

Called by: [main](pap.c.md#main)

### paprc

```c
static char * paprc(void)
```

Defined at lines 90 to 118.

Called by: [main](pap.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 180 to 531.

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [copy_status](pap.c.md#copy_status), [nbp_lookup](../../libatalk/nbp/nbp_lkup.c.md#nbp_lookup), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [paprc](pap.c.md#paprc), [send_file](pap.c.md#send_file), [updatestatus](pap.c.md#updatestatus), [usage](pap.c.md#usage)

Uses file-scope variables: `cbuf`, `connid`, `debug`, `nn`, `noeof`, `oquantum`, `quantum`, `rniov`, `sat`, `satp`, `status`, `waitforprinter`

### send_file

```c
static int send_file(int fd, ATP atp, int lastfile, int is_imagewriter)
```

Defined at lines 537 to 977.

Calls: [atp_rreq](../../libatalk/atp/atp_rreq.c.md#atp_rreq), [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_rsel](../../libatalk/atp/atp_rsel.c.md#atp_rsel), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [atp_sresp](../../libatalk/atp/atp_sresp.c.md#atp_sresp), [copy_status](pap.c.md#copy_status), [updatestatus](pap.c.md#updatestatus)

Called by: [main](pap.c.md#main)

Uses file-scope variables: `cbuf`, `connid`, `data`, `debug`, `nn`, `noeof`, `oquantum`, `port`, `quantum`, `rfiov`, `rniov`, `sat`, `satp`, `seq`, `sfiov`, `sniov`, `waitforprinter`

### updatestatus

```c
static void updatestatus(char *s, int len)
```

Defined at lines 979 to 1009.

Called by: [main](pap.c.md#main), [send_file](pap.c.md#send_file)

Uses file-scope variables: `status`

# Macros

* Undocumented: `IMAGEWRITER`, `IMAGEWRITER_LQ`, `PAP_STATUS_BUFFER_SIZE`, `PAP_STATUS_HEADER_SIZE`, `_PATH_PAPRC`

# File-scope variables

`cbuf`, `connid`, `data`, `debug`, `fbuf`, `nbuf`, `nn`, `noeof`, `oquantum`, `port`, `printer`, `quantum`, `rfiov`, `rniov`, `sat`, `satp`, `seq`, `sfiov`, `sniov`, `status`, `waitforprinter`
