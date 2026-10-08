---
type: C Source File
title: "libatalk/adouble/ad_attr.c"
description: "6 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_attr.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 30 to 70.

Note: the "shared" and "invisible" attributes are opaque and stored and retrieved from the FinderFlags. This fixes Bug #2802236:

See also: https://sourceforge.net/p/netatalk/bugs/350/

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [change_attributes](../../bin/nad/nad_set.c.md#change_attributes), [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [getdirparams](../../etc/afpd/directory.c.md#getdirparams), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [print_flags](../../bin/nad/nad_ls.c.md#print_flags), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

Mentioned in the documentation of: [ad_rebuild_from_cache](../../etc/afpd/ad_cache.c.md#ad_rebuild_from_cache)

### ad_setattr

```c
int ad_setattr(const struct adouble *ad, const uint16_t attribute)
```

Defined at lines 73 to 113.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [change_attributes](../../bin/nad/nad_set.c.md#change_attributes), [new_ad_header](ad_open.c.md#new_ad_header), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

### ad_setid

```c
int ad_setid(struct adouble *adp, const dev_t dev, const ino_t ino, const uint32_t id, const cnid_t did, const void *stamp)
```

Defined at lines 120 to 232.

save file/folder ID in AppleDoubleV2 netatalk private parameters

Returns: 1 if resource fork has been modified

Returns: -1 on error.

Called by: [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [do_move](../../bin/nad/nad_mv.c.md#do_move), [get_id](../../etc/afpd/file.c.md#get_id), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_update_cnid](../../bin/nad/nad_adouble.c.md#nad_update_cnid), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

### ad_getid

```c
uint32_t ad_getid(struct adouble *adp, const dev_t st_dev, const ino_t st_ino, const cnid_t did, const void *stamp)
```

Defined at lines 238 to 311.

Retrieve stored file / folder.

Note: Callers should treat a return of CNID_INVALID (0) as an invalid value.

Called by: [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [get_id](../../etc/afpd/file.c.md#get_id), [getmetadata](../../etc/afpd/file.c.md#getmetadata)

### ad_forcegetid

```c
uint32_t ad_forcegetid(struct adouble *adp)
```

Defined at lines 314 to 336.

Called by: [getmetadata](../../etc/afpd/file.c.md#getmetadata), [print_flags](../../bin/nad/nad_ls.c.md#print_flags)

### ad_setname

```c
int ad_setname(struct adouble *ad, const char *path)
```

Defined at lines 341 to 363.

set resource fork filename attribute.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

# Macros

* Undocumented: `AFPFILEIOFF_ATTR`, `FILEIOFF_ATTR`
