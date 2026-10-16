---
type: C Source File
title: "libatalk/adouble/ad_write.c"
description: "Writing and truncating the forks of an AppleDouble."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_write.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T12:16:30+11:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### adf_pwrite

```c
ssize_t adf_pwrite(struct ad_fd *ad_fd, const void *buf, size_t count, off_t offset)
```

Defined at lines 39 to 64.

Write count bytes that start at offset in an AppleDouble fork.

The write goes through pwrite and leaves the file position alone. The function checks no byte-range locks.

Parameters:
* ``: fork descriptor
* `buf`: bytes to write
* `count`: bytes to write
* `offset`: file offset

Returns: bytes written, or -1 with errno set

Called by: [ad_convert_pwrite_full](ad_open.c.md#ad_convert_pwrite_full), [ad_flush_hf](ad_flush.c.md#ad_flush_hf), [ad_flush_rf](ad_flush.c.md#ad_flush_rf), [ad_write](ad_write.c.md#ad_write)

### ad_write

```c
ssize_t ad_write(struct adouble *ad, uint32_t eid, off_t off, int end, const char *buf, size_t buflen)
```

Defined at lines 84 to 147.

Write to the data or resource fork of an AppleDouble.

A symlink data fork fails with EACCES. With a nonzero end, off counts back from the end of the fork. The ea layout stores the resource fork at offset 0 of its EA, or behind the sidecar header without HAVE_EAFD. Version 2 adds the entry offset from the header, and a resource fork write that reaches past ad_rlen extends it. The function checks no byte-range locks.

Parameters:
* `ad`: AppleDouble with the fork open
* `eid`: ADEID_DFORK or ADEID_RFORK
* `off`: offset within the fork
* `end`: 0, or nonzero to count off back from the end of the fork
* `buf`: bytes to write
* `buflen`: bytes to write

Returns: bytes written, or -1 on failure or for any other eid

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [adf_pwrite](ad_write.c.md#adf_pwrite)

Called by: [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [nad_write](../../bin/nad/nad_adouble.c.md#nad_write), [write_file](../../etc/afpd/fork.c.md#write_file)

Mentioned in the documentation of: [ad_recvfile](ad_recvfile.c.md#ad_recvfile)

### sys_ftruncate

```c
int sys_ftruncate(int fd, off_t length)
```

Defined at lines 163 to 217.

Truncate or extend an open file to length.

ftruncate does the work. When it fails on a file shorter than length, the function writes one zero byte at length minus one instead. That covers platforms whose ftruncate cannot extend a file. Without HAVE_PWRITE it also restores the file position afterwards. The caller holds any locks the region needs.

Parameters:
* `fd`: open file
* `length`: new length in bytes

Returns: 0, or -1 with the ftruncate errno, or the write errno when the extension failed

Called by: [ad_dtruncate](ad_write.c.md#ad_dtruncate), [ad_rtruncate](ad_write.c.md#ad_rtruncate)

### ad_rtruncate

```c
int ad_rtruncate(struct adouble *ad, const char *uname, const off_t size)
```

Defined at lines 232 to 254.

Truncate the resource fork of an AppleDouble to size.

The truncation point is size plus the resource fork entry offset, so the header ahead of the fork survives. ad_rlen follows. A zero size keeps the fork file, because an open fork may still reference it and ad_flush would recreate it. of_closefork deletes empty resource forks.

Parameters:
* `ad`: AppleDouble with the resource fork open
* `uname`: file name, for logging only
* `size`: new fork length in bytes

Returns: 0, or -1 with errno set

Calls: [fullpathname](../util/unix.c.md#fullpathname), [sys_ftruncate](ad_write.c.md#sys_ftruncate)

Called by: [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams)

### ad_dtruncate

```c
int ad_dtruncate(struct adouble *ad, const off_t size)
```

Defined at lines 263 to 272.

Truncate the data fork of an AppleDouble to size.

Parameters:
* `ad`: AppleDouble with the data fork open
* `size`: new fork length in bytes

Returns: 0, or -1 with errno set

Calls: [sys_ftruncate](ad_write.c.md#sys_ftruncate)

Called by: [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams)

### copy_fork

```c
int copy_fork(int eid, struct adouble *add, struct adouble *ads, uint8_t *buf, size_t buflen)
```

Defined at lines 288 to 336.

Transfer one fork from the AppleDouble ads into the AppleDouble add.

Both forks must be open. The read loop starts at the entry offset of each side and runs until the source returns end of file. A caller buffer serves only when it is larger than the 8 KiB stack buffer.

Parameters:
* `eid`: ADEID_DFORK, or any other entry for the resource fork
* `add`: destination AppleDouble
* `ads`: source AppleDouble
* `buf`: optional transfer buffer, or NULL
* `buflen`: size of buf in bytes

Returns: 0, or -1 with errno set

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [write_full](../util/unix.c.md#write_full)

Called by: [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [copyfile](../../etc/afpd/file.c.md#copyfile)
