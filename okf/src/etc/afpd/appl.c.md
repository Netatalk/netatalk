---
type: C Source File
title: "etc/afpd/appl.c"
description: "7 functions, includes 10 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/appl.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 34 to 37.

Called by: [copyapplfile](appl.c.md#copyapplfile)

### applopen

```c
static int applopen(struct vol *vol, uint8_t creator[4], int flags, int mode)
```

Defined at lines 39 to 86.

Calls: [ad_mkdir](../../libatalk/adouble/ad_open.c.md#ad_mkdir), [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [dtfile](desktop.c.md#dtfile)

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_getappl](appl.c.md#afp_getappl), [afp_rmvappl](appl.c.md#afp_rmvappl)

Uses file-scope variables: `sa`

### copyapplfile

```c
static int copyapplfile(int sfd, int dfd, char *mpath, u_short mplen)
```

Defined at lines 91 to 164.

copy appls to new file, deleting any matching (old) appl entries

Calls: [pathcmp](appl.c.md#pathcmp)

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_rmvappl](appl.c.md#afp_rmvappl)

### makemacpath

```c
static char * makemacpath(const struct vol *vol, char *mpath, int mpathlen, struct dir *dir, char *path)
```

Defined at lines 180 to 213.

build mac. path (backwards) by traversing the directory tree

The old way: dir and path refer to an app, path is a mac format pathname. [makemacpath()](appl.c.md#makemacpath) builds something that looks like a cname, but uses upaths instead of mac format paths.

The new way: dir and path refer to an app, path is a mac format pathname. [makemacpath()](appl.c.md#makemacpath) builds a cname. (zero is a path separator and it's not \0 terminated).

See also: [afp_getappl()](appl.c.md#afp_getappl) for the backward compatiblity code.

Calls: [dirlookup](directory.c.md#dirlookup)

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_rmvappl](appl.c.md#afp_rmvappl)

### afp_addappl

```c
int afp_addappl(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 216 to 355.

Calls: [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [applopen](appl.c.md#applopen), [cname](directory.c.md#cname), [copyapplfile](appl.c.md#copyapplfile), [dirlookup](directory.c.md#dirlookup), [dtfile](desktop.c.md#dtfile), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [makemacpath](appl.c.md#makemacpath), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `sa`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_rmvappl

```c
int afp_rmvappl(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 357 to 464.

Calls: [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [applopen](appl.c.md#applopen), [cname](directory.c.md#cname), [copyapplfile](appl.c.md#copyapplfile), [dirlookup](directory.c.md#dirlookup), [dtfile](desktop.c.md#dtfile), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [makemacpath](appl.c.md#makemacpath), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `sa`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_getappl

```c
int afp_getappl(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 466 to 620.

Calls: [applopen](appl.c.md#applopen), [cname](directory.c.md#cname), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [path_isadir](../../include/atalk/directory.h.md#path_isadir)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `sa`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

Mentioned in the documentation of: [makemacpath](appl.c.md#makemacpath)

# File-scope variables

`sa`
