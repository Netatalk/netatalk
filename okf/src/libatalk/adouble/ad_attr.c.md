---
type: C Source File
title: "libatalk/adouble/ad_attr.c"
description: "AFP attributes, CNID, and name entries of an AppleDouble."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_attr.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-19T23:19:03+11:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `stdlib.h`, `string.h`

# Functions

### ad_getattr

```c
int ad_getattr(const struct adouble *ad, uint16_t *attr)
```

Defined at lines 42 to 82.

Read the AFP attribute word of an AppleDouble entry.

The shared and invisible attributes are opaque and live in the Finder flags, so they come from there, as bug #2802236 required. For directories ATTRBIT_EXPFLDR occupies the shared bit and stays untouched. The adouble's open-fork bits are also present in the result.

See also: https://sourceforge.net/p/netatalk/bugs/350/

Parameters:
* `ad`: AppleDouble with its metadata loaded
* `attr`: receives the attribute word in network byte order

Returns: 0

Calls: [ad_entry](ad_open.c.md#ad_entry), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [change_attributes](../../bin/nad/nad_set.c.md#change_attributes), [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [getdirparams](../../etc/afpd/directory.c.md#getdirparams), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [print_flags](../../bin/nad/nad_ls.c.md#print_flags), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

### ad_setattr

```c
int ad_setattr(const struct adouble *ad, const uint16_t attribute)
```

Defined at lines 97 to 137.

Store the AFP attribute word in an AppleDouble entry.

ad_setattr never stores the open-fork bits. For directories it drops the bits that only files carry (ATTRBIT_MULTIUSER, ATTRBIT_NOWRITE, ATTRBIT_NOCOPY), because Mac OS X 10.4 tries to set ATTRBIT_MULTIUSER on folders. It also writes the invisible bit, and for files the shared bit, into the Finder flags.

Parameters:
* `ad`: AppleDouble with its metadata loaded
* `attribute`: attribute word in network byte order

Returns: 0

Calls: [ad_entry](ad_open.c.md#ad_entry), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [change_attributes](../../bin/nad/nad_set.c.md#change_attributes), [new_ad_header](ad_open.c.md#new_ad_header), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

### ad_setid

```c
int ad_setid(struct adouble *adp, const dev_t dev, const ino_t ino, const uint32_t id, const cnid_t did, const void *stamp)
```

Defined at lines 158 to 270.

Store the CNID, the file identity, and the database stamp in the private AppleDouble entries.

PRIVDEV and PRIVINO hold 8 bytes each, so the native dev_t and ino_t widen to 64 bits in host byte order. ADVOL_NODEV zeroes the device. The CNID goes out in network order for the EA form. The parent DID only exists in version 2. When an entry cannot be addressed, ad_setid restores the lengths it changed and reports nothing written.

Parameters:
* `adp`: AppleDouble with its metadata loaded
* `dev`: device of the file
* `ino`: inode of the file
* `id`: CNID to store
* `did`: CNID of the parent directory, stored for version 2 only
* `stamp`: CNID database stamp of ADEDLEN_PRIVSYN bytes

Returns: 1 after writing the entries, so the header needs a flush, or 0 when an entry cannot be addressed

Calls: [ad_entry](ad_open.c.md#ad_entry)

Called by: [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [do_move](../../bin/nad/nad_mv.c.md#do_move), [get_id](../../etc/afpd/file.c.md#get_id), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_update_cnid](../../bin/nad/nad_adouble.c.md#nad_update_cnid), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

### ad_getid

```c
uint32_t ad_getid(struct adouble *adp, const dev_t st_dev, const ino_t st_ino, const cnid_t did, const void *stamp)
```

Defined at lines 288 to 361.

Read the CNID stored in an AppleDouble entry when it matches the file.

The stored device and inode must equal the file's, unless the volume runs with ADVOL_NODEV. The stored parent must equal did when both are non-zero. Version 2 entries hold the CNID in host order, EA entries in network order. Callers treat a return of CNID_INVALID (0) as an invalid value.

Parameters:
* `adp`: AppleDouble with its metadata loaded, or NULL
* `st_dev`: device of the file from stat
* `st_ino`: inode of the file from stat
* `did`: CNID of the parent directory, or 0 to skip that check
* `stamp`: unused

Returns: the stored CNID, or CNID_INVALID when the entry is absent or the identity differs

Calls: [ad_entry](ad_open.c.md#ad_entry)

Called by: [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [get_id](../../etc/afpd/file.c.md#get_id), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [of_hint_siblings](../../etc/afpd/ofork.c.md#of_hint_siblings)

### ad_forcegetid

```c
uint32_t ad_forcegetid(struct adouble *adp)
```

Defined at lines 371 to 393.

Read the CNID stored in an AppleDouble entry without checking the file's identity.

Parameters:
* `adp`: AppleDouble with its metadata loaded, or NULL

Returns: the stored CNID, or CNID_INVALID when adp is NULL or has no PRIVID entry

Calls: [ad_entry](ad_open.c.md#ad_entry)

Called by: [getmetadata](../../etc/afpd/file.c.md#getmetadata), [print_flags](../../bin/nad/nad_ls.c.md#print_flags)

### ad_setname

```c
int ad_setname(struct adouble *ad, const char *path)
```

Defined at lines 405 to 427.

Store a file name in the AppleDouble name entry.

ad_setname cuts the name at ADEDLEN_NAME bytes.

Parameters:
* `ad`: AppleDouble with its metadata loaded
* `path`: name to store

Returns: 1 after storing the name, 0 when the entry does not exist, -1 when it cannot be addressed

Calls: [ad_entry](ad_open.c.md#ad_entry), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

# Macros

* Undocumented: `AFPFILEIOFF_ATTR`, `FILEIOFF_ATTR`
