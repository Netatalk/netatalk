---
type: C Source File
title: "libatalk/adouble/ad_write.c"
description: "7 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_write.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### adf_pwrite

```c
ssize_t adf_pwrite(struct ad_fd *ad_fd, const void *buf, size_t count, off_t offset)
```

Defined at lines 25 to 50.

Called by: [ad_convert_pwrite_full](ad_open.c.md#ad_convert_pwrite_full), [ad_flush_hf](ad_flush.c.md#ad_flush_hf), [ad_flush_rf](ad_flush.c.md#ad_flush_rf), [ad_write](ad_write.c.md#ad_write)

### ad_write

```c
ssize_t ad_write(struct adouble *ad, uint32_t eid, off_t off, int end, const char *buf, size_t buflen)
```

Defined at lines 53 to 116.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [adf_pwrite](ad_write.c.md#adf_pwrite)

Called by: [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [nad_write](../../bin/nad/nad_adouble.c.md#nad_write), [write_file](../../etc/afpd/fork.c.md#write_file)

### sys_ftruncate

```c
int sys_ftruncate(int fd, off_t length)
```

Defined at lines 122 to 176.

Called by: [ad_dtruncate](ad_write.c.md#ad_dtruncate), [ad_rtruncate](ad_write.c.md#ad_rtruncate)

### ad_rtruncate

```c
int ad_rtruncate(struct adouble *ad, const char *uname, const off_t size)
```

Defined at lines 179 to 201.

Calls: [fullpathname](../util/unix.c.md#fullpathname), [sys_ftruncate](ad_write.c.md#sys_ftruncate)

Called by: [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams)

### ad_dtruncate

```c
int ad_dtruncate(struct adouble *ad, const off_t size)
```

Defined at lines 203 to 212.

Calls: [sys_ftruncate](ad_write.c.md#sys_ftruncate)

Called by: [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams)

### copy_all

```c
static int copy_all(const int dfd, const void *buf, size_t buflen)
```

Defined at lines 215 to 235.

Called by: [copy_fork](ad_write.c.md#copy_fork)

### copy_fork

```c
int copy_fork(int eid, struct adouble *add, struct adouble *ads, uint8_t *buf, size_t buflen)
```

Defined at lines 240 to 320.

copy only the fork data stream

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [copy_all](ad_write.c.md#copy_all), [sys_sendfile](ad_sendfile.c.md#sys_sendfile)

Called by: [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [copyfile](../../etc/afpd/file.c.md#copyfile)
