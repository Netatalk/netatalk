---
type: C Source File
title: "libatalk/dsi/dsi_write.c"
description: "3 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_write.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `fcntl.h`, `stdio.h`, `string.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Functions

### dsi_writeinit

```c
size_t dsi_writeinit(DSI *, char **)
```

Defined at lines 31 to 64. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Begin a [DSI](../../include/atalk/dsi.h.md#struct-dsi) write transfer: set datasize and hand back, by reference, any payload already in the read-ahead buffer. *bufp is valid only until the next dsi_* call; consume it immediately. A NULL bufp discards the buffered payload, for callers that only drain the transfer.

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [write_fork](../../etc/afpd/fork.c.md#write_fork)

### dsi_write

```c
size_t dsi_write(DSI *, void *, const size_t)
```

Defined at lines 70 to 85. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

fill up buf and then return. this should be called repeatedly until all the data has been read. i block alarm processing during the transfer to avoid sending unnecessary tickles.

Calls: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [write_fork](../../etc/afpd/fork.c.md#write_fork)

### dsi_writeflush

```c
void dsi_writeflush(DSI *)
```

Defined at lines 88 to 102. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

flush any unread buffers.

Calls: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [write_fork](../../etc/afpd/fork.c.md#write_fork)
