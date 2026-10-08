---
type: C Source File
title: "etc/afpd/mangle.c"
description: "mangle, demangle (filename)"
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/mangle.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)
* [desktop.h](desktop.h.md)
* [mangle.h](mangle.h.md)
* System headers: `ctype.h`, `stdio.h`

# Functions

### mangle_extension

```c
static size_t mangle_extension(const struct vol *vol, const char *uname, char *extension, charset_t charset)
```

Defined at lines 30 to 47.

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset)

Called by: [demangle_checks](mangle.c.md#demangle_checks), [mangle](mangle.c.md#mangle)

### demangle_checks

```c
static char * demangle_checks(const struct vol *vol, char *uname, char *mfilename, size_t prefix, char *ext)
```

Defined at lines 49 to 144.

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [mangle_extension](mangle.c.md#mangle_extension)

Called by: [private_demangle](mangle.c.md#private_demangle)

### private_demangle

```c
static char * private_demangle(const struct vol *vol, char *mfilename, cnid_t did, cnid_t *osx)
```

Defined at lines 149 to 227.

Calls: [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [demangle_checks](mangle.c.md#demangle_checks), [dirlookup](directory.c.md#dirlookup), [utompath](desktop.c.md#utompath)

Called by: [demangle](mangle.c.md#demangle), [demangle_osx](mangle.c.md#demangle_osx)

### demangle

```c
char * demangle(const struct vol *vol, char *mfilename, cnid_t did)
```

Defined at lines 232 to 235.

Calls: [private_demangle](mangle.c.md#private_demangle)

Called by: [mtoupath](desktop.c.md#mtoupath)

### demangle_osx

```c
char * demangle_osx(const struct vol *vol, char *mfilename, cnid_t did, cnid_t *fileid)
```

Defined at lines 241 to 244.

Calls: [private_demangle](mangle.c.md#private_demangle)

Called by: [cname_mtouname](directory.c.md#cname_mtouname)

### mangle

```c
char * mangle(const struct vol *vol, char *filename, size_t filenamelen, char *uname, cnid_t id, int flags)
```

Defined at lines 265 to 320.

Bug: Early Mac OS X (10.0-10.4.?) had the limitation up to 255 Byte. Current implementation is: volcharset -> UTF16-MAC -> truncated 255 UTF8-MAC Recent Mac OS X (10.4.?-) don't have this limitation. Desirable implementation is: volcharset -> truncated 510 UTF16-MAC -> UTF8-MAC

Note: with utf8 filename not always round trip ```
filename   mac filename too long or first chars if unmatchable chars.
uname      unix filename
id         file/folder ID or 0
```

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [mangle_extension](mangle.c.md#mangle_extension), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [utompath](desktop.c.md#utompath)

# Macros

* Undocumented: `hextoint`, `isuxdigit`
