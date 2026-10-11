---
type: C Source File
title: "etc/afpd/mangle.c"
description: "mangle, demangle (filename)"
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/mangle.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T10:33:24+11:00 }
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

Defined at lines 43 to 60.

Convert the extension of a name for the mangled form.

The extension starts at the last dot, which must not be the first byte.

Parameters:
* `vol`: volume of the name
* `uname`: name on disk
* `extension`: receives the converted extension, at least MAX_EXT_LENGTH + 2 bytes
* `charset`: target charset

Returns: the length of the extension, or 0 without one or when the conversion fails

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset)

Called by: [demangle_checks](mangle.c.md#demangle_checks), [mangle](mangle.c.md#mangle)

Mentioned in the documentation of: [mangle](mangle.c.md#mangle)

### demangle_checks

```c
static char * demangle_checks(const struct vol *vol, char *uname, char *mfilename, size_t prefix, char *ext)
```

Defined at lines 79 to 174.

Decide whether a mangled name from the client stands for a name on disk.

A name with a valid `#ID` can also be a real name, so the extensions must match first. The name on disk must have needed mangling, because it was too long or held unconvertible characters. Its converted form must then match the prefix that the client sent. Mac OS X may cut a decomposed character in the prefix, and a name that converts to nothing appears as "???".

Parameters:
* `vol`: volume of the name
* `uname`: on-disk name of the object that the ID names
* `mfilename`: mangled name from the client
* `prefix`: length of the name before MANGLE_CHAR
* `ext`: extension after the ID

Returns: uname for a match, or mfilename

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [mangle_extension](mangle.c.md#mangle_extension)

Called by: [private_demangle](mangle.c.md#private_demangle)

### private_demangle

```c
static char * private_demangle(const struct vol *vol, char *mfilename, cnid_t did, cnid_t *osx)
```

Defined at lines 195 to 273.

Find the on-disk name that a mangled name in a directory stands for.

A mangled name holds MANGLE_CHAR, the CNID in uppercase hex without a leading zero, and an extension no longer than MAX_EXT_LENGTH bytes. The CNID database starts at 17, so a smaller ID never matches. The object must sit in did. Without osx the name must equal the mangled name the server builds. With osx, demangle_checks also accepts the forms that Mac OS X sends.

Parameters:
* `vol`: volume of the name
* `mfilename`: name from the client
* `did`: CNID of the directory that holds the object
* `osx`: receives the CNID in network order, or NULL for an exact match

Returns: the name on disk, or mfilename when it is not a mangled name of an object in did

Calls: [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [demangle_checks](mangle.c.md#demangle_checks), [dirlookup](directory.c.md#dirlookup), [utompath](desktop.c.md#utompath)

Called by: [demangle](mangle.c.md#demangle), [demangle_osx](mangle.c.md#demangle_osx)

### demangle

```c
char * demangle(const struct vol *vol, char *mfilename, cnid_t did)
```

Defined at lines 285 to 288.

Resolve a mangled name in a directory, accepting only the exact form the server builds.

Parameters:
* `vol`: volume of the name
* `mfilename`: name from the client
* `did`: CNID of the directory that holds the object

Returns: the name on disk, or mfilename

Calls: [private_demangle](mangle.c.md#private_demangle)

Called by: [mtoupath](desktop.c.md#mtoupath)

### demangle_osx

```c
char * demangle_osx(const struct vol *vol, char *mfilename, cnid_t did, cnid_t *fileid)
```

Defined at lines 302 to 305.

Resolve a mangled name in a directory, also accepting the forms that Mac OS X sends.

Parameters:
* `vol`: volume of the name
* `mfilename`: name from the client
* `did`: CNID of the directory that holds the object
* `fileid`: receives the CNID, in network order, of a name in the mangled form

Returns: the name on disk, or mfilename

Calls: [private_demangle](mangle.c.md#private_demangle)

Called by: [cname_mtouname](directory.c.md#cname_mtouname)

### mangle

```c
char * mangle(const struct vol *vol, char *filename, size_t filenamelen, char *uname, cnid_t id, int flags)
```

Defined at lines 333 to 388.

Shorten or replace a filename the client cannot take, tagging it with the CNID.

A name over the client's limit keeps a prefix of its UTF-8 or Mac charset form. MANGLE_CHAR, the CNID in hexadecimal, and the extension that [mangle_extension()](mangle.c.md#mangle_extension) found follow the prefix. The result lives in a static buffer.

Bug: Early Mac OS X (10.0 to 10.4) limited names to 255 bytes. The conversion therefore runs volcharset to UTF16-MAC to UTF8-MAC truncated at 255 bytes. Later releases take 510 bytes of UTF16-MAC, which would allow volcharset to truncated UTF16-MAC to UTF8-MAC.

Note: A UTF-8 filename does not always round-trip.

Parameters:
* `vol`: volume the file belongs to
* `filename`: Mac name, too long or holding characters the client cannot map
* `filenamelen`: length of filename in bytes
* `uname`: Unix name of the file
* `id`: CNID of the file, or 0
* `flags`: bit 0 forces mangling, bit 1 selects the UTF-8 length limit

Returns: filename when it fits, NULL when id is 0, or the mangled name

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [mangle_extension](mangle.c.md#mangle_extension), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [utompath](desktop.c.md#utompath)

# Macros

* Undocumented: `hextoint`, `isuxdigit`
