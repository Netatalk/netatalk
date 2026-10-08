---
type: C Source File
title: "libatalk/dsi/dsi_read.c"
description: "3 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_read.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `signal.h`, `stdio.h`, `string.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Functions

### dsi_readinit

```c
ssize_t dsi_readinit(DSI *, void *, const size_t, const size_t, const int)
```

Defined at lines 29 to 49. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

streaming i/o for afp_read.

this is all from the perspective of the client. it basically does the reverse of dsi_write. on first entry, it will send off the header plus whatever is in its command buffer. it returns the amount of stuff still to be read (constrained by the buffer size).

Calls: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

Called by: [afp_geticon](../../etc/afpd/desktop.c.md#afp_geticon), [read_fork](../../etc/afpd/fork.c.md#read_fork), [rfork_cache_serve_from_buf](../../etc/afpd/fork.c.md#rfork_cache_serve_from_buf)

### dsi_readdone

```c
void dsi_readdone(DSI *)
```

Defined at lines 51 to 54. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Called by: [afp_geticon](../../etc/afpd/desktop.c.md#afp_geticon), [read_fork](../../etc/afpd/fork.c.md#read_fork), [rfork_cache_serve_from_buf](../../etc/afpd/fork.c.md#rfork_cache_serve_from_buf)

### dsi_read

```c
ssize_t dsi_read(DSI *, void *, const size_t)
```

Defined at lines 57 to 68. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

send off the data

Calls: [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

Called by: [afp_geticon](../../etc/afpd/desktop.c.md#afp_geticon), [read_fork](../../etc/afpd/fork.c.md#read_fork), [rfork_cache_serve_from_buf](../../etc/afpd/fork.c.md#rfork_cache_serve_from_buf)
