---
type: C Source File
title: "etc/afpd/appl.c"
description: "The APPL desktop database: FPAddAPPL, FPRemoveAPPL, and FPGetAPPL."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/appl.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-27T23:13:49+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [desktop.h](desktop.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [volume.h](volume.h.md)
* System headers: `bstrlib.h`, `ctype.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### pathcmp

```c
static int pathcmp(char *p, int plen, char *q, int qlen)
```

Defined at lines 48 to 51.

Compare two APPL paths of explicit lengths.

Parameters:
* `p`: first path
* `plen`: length of p
* `q`: second path
* `qlen`: length of q

Returns: 0 when both paths hold the same bytes, 1 otherwise

Called by: [copyapplfile](appl.c.md#copyapplfile)

### applopen

```c
static int applopen(struct vol *vol, uint8_t creator[4], int flags, int mode)
```

Defined at lines 69 to 116.

Open the APPL file of a creator in the desktop database.

One descriptor serves the APPL file of the last creator and volume. A read-only open of the same file reuses it and keeps its read position, so FPGetAPPL can walk the entries in order. Any other open closes it first. With O_CREAT, a missing file also gets its two parent directories in the desktop database.

Parameters:
* `vol`: volume of the desktop database
* `creator`: creator code of the application
* `flags`: open flags
* `mode`: creation mode, masked with the parent directory permissions

Returns: AFP_OK, or AFPERR_PARAM when open fails

Calls: [ad_mkdir](../../libatalk/adouble/ad_open.c.md#ad_mkdir), [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [dtfile](desktop.c.md#dtfile)

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_getappl](appl.c.md#afp_getappl), [afp_rmvappl](appl.c.md#afp_rmvappl)

Uses file-scope variables: `sa`

### copyapplfile

```c
static int copyapplfile(int sfd, int dfd, char *mpath, u_short mplen)
```

Defined at lines 132 to 205.

Write the APPL entries of one file to another, except the entries of one path.

Each entry holds a 4-byte APPL tag, a 2-byte path length, and the path. The call drops every entry whose path equals mpath. A short read or a path length past the buffer ends the call with -1.

Parameters:
* `sfd`: APPL file, read from its current position
* `dfd`: file that receives the kept entries
* `mpath`: path to drop
* `mplen`: length of mpath

Returns: 0 at the end of sfd, or -1 on a read, length, or write error

Calls: [pathcmp](appl.c.md#pathcmp)

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_rmvappl](appl.c.md#afp_rmvappl)

### makemacpath

```c
static char * makemacpath(const struct vol *vol, char *mpath, int mpathlen, struct dir *dir, char *path)
```

Defined at lines 226 to 258.

Build the volume path of a file backwards at the end of a buffer.

The result holds the long names of the directories below the volume root and of the file. NUL bytes separate the names and no NUL ends them, the form that cname reads. The first 6 bytes of mpath stay free for the APPL tag and the entry length.

Parameters:
* `vol`: volume of the file
* `mpath`: buffer that receives the path at its end
* `mpathlen`: size of mpath
* `dir`: directory of the file
* `path`: long name of the file

Returns: the start of the path in mpath, or NULL when it does not fit or a parent lookup fails

See also: [afp_getappl()](appl.c.md#afp_getappl) for the decoding of older [entries](../../libatalk/util/server_ipc.c.md).

Calls: [dirlookup](directory.c.md#dirlookup)

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_rmvappl](appl.c.md#afp_rmvappl)

### afp_addappl

```c
int afp_addappl(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 276 to 415.

FPAddAPPL: add an application to the APPL file of its creator.

The request carries the volume, the directory, the creator, the APPL tag, and the path. The new entry goes first, so index 1 of FPGetAPPL names the application added last. The call drops an older entry for the same path. It writes the entries to a temporary file and renames that over the APPL file.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_BADTYPE for a directory, AFPERR_PARAM, or the error of the directory or path lookup

Calls: [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [applopen](appl.c.md#applopen), [cname](directory.c.md#cname), [copyapplfile](appl.c.md#copyapplfile), [dirlookup](directory.c.md#dirlookup), [dtfile](desktop.c.md#dtfile), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [makemacpath](appl.c.md#makemacpath), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `sa`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_rmvappl

```c
int afp_rmvappl(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 433 to 540.

FPRemoveAPPL: remove an application from the APPL file of its creator.

The request carries the volume, the directory, the creator, and the path. The call writes every other entry to a temporary file and renames that over the APPL file. A path without an entry still gets AFP_OK.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_NOOBJ without an APPL file, AFPERR_BADTYPE for a directory, AFPERR_PARAM, or the error of the directory or path lookup

Calls: [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [applopen](appl.c.md#applopen), [cname](directory.c.md#cname), [copyapplfile](appl.c.md#copyapplfile), [dirlookup](directory.c.md#dirlookup), [dtfile](desktop.c.md#dtfile), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [makemacpath](appl.c.md#makemacpath), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `sa`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_getappl

```c
int afp_getappl(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 563 to 717.

FPGetAPPL: return the file parameters of an application from the APPL file of its creator.

The request carries the volume, the creator, a 1-based index, and the file bitmap. Index 0 counts as 1. The open APPL file keeps its position between calls, so a walk with rising indexes reads each entry once.

The call decodes the colon hex escapes of older entries and resolves the path from the volume root. The reply holds the bitmap, the APPL tag, and the file parameters.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_NOITEM when no entry or file matches, AFPERR_BITMAP, or AFPERR_PARAM

Calls: [applopen](appl.c.md#applopen), [cname](directory.c.md#cname), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [path_isadir](../../include/atalk/directory.h.md#path_isadir)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `sa`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

Mentioned in the documentation of: [makemacpath](appl.c.md#makemacpath)

# File-scope variables

`sa`
