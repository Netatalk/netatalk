---
type: C Source File
title: "etc/afpd/desktop.c"
description: "Manage the AppleDesktop folder and its contents."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/desktop.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [fork.h](fork.h.md)
* [mangle.h](mangle.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `bstrlib.h`, `ctype.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/stat.h`, `sys/uio.h`

# Functions

### setdeskmode

```c
int setdeskmode(const struct vol *vol, const mode_t mode)
```

Defined at lines 57 to 164.

Calls: [dir_rx_set](../../libatalk/vfs/unix.c.md#dir_rx_set), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [ochmod](../../libatalk/util/unix.c.md#ochmod)

Called by: [setdirparams](directory.c.md#setdirparams)

### setdeskowner

```c
int setdeskowner(const struct vol *vol, uid_t uid, gid_t gid)
```

Defined at lines 166 to 240.

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname)

Called by: [setdirparams](directory.c.md#setdirparams)

### create_appledesktop_folder

```c
static void create_appledesktop_folder(const struct vol *vol)
```

Defined at lines 242 to 273.

Calls: [become_root](../../libatalk/util/unix.c.md#become_root), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Called by: [afp_opendt](desktop.c.md#afp_opendt)

### afp_opendt

```c
int afp_opendt(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 275 to 292.

Calls: [create_appledesktop_folder](desktop.c.md#create_appledesktop_folder), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_closedt

```c
int afp_closedt(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 294 to 307.

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### icon_dtfile

```c
static char * icon_dtfile(struct vol *vol, uint8_t creator[4])
```

Defined at lines 311 to 314.

Calls: [dtfile](desktop.c.md#dtfile)

Called by: [afp_addicon](desktop.c.md#afp_addicon), [afp_geticon](desktop.c.md#afp_geticon), [afp_geticoninfo](desktop.c.md#afp_geticoninfo), [iconopen](desktop.c.md#iconopen)

### iconopen

```c
static int iconopen(struct vol *vol, uint8_t creator[4], int flags, int mode)
```

Defined at lines 316 to 363.

Calls: [ad_mkdir](../../libatalk/adouble/ad_open.c.md#ad_mkdir), [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [icon_dtfile](desktop.c.md#icon_dtfile)

Called by: [afp_addicon](desktop.c.md#afp_addicon), [afp_geticon](desktop.c.md#afp_geticon), [afp_geticoninfo](desktop.c.md#afp_geticoninfo)

Uses file-scope variables: `si`

### afp_addicon

```c
int afp_addicon(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 365 to 569.

Calls: [asp_wrtcont](../../libatalk/asp/asp_write.c.md#asp_wrtcont), [dsi_write](../../libatalk/dsi/dsi_write.c.md#dsi_write), [dsi_writeflush](../../libatalk/dsi/dsi_write.c.md#dsi_writeflush), [dsi_writeinit](../../libatalk/dsi/dsi_write.c.md#dsi_writeinit), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [icon_dtfile](desktop.c.md#icon_dtfile), [iconopen](desktop.c.md#iconopen)

Uses file-scope variables: `si`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_geticoninfo

```c
int afp_geticoninfo(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 579 to 654.

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [icon_dtfile](desktop.c.md#icon_dtfile), [iconopen](desktop.c.md#iconopen)

Uses file-scope variables: `si`, `ucreator`, `usize`, `utag`, `utype`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_geticon

```c
int afp_geticon(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 657 to 807.

Calls: [dsi_read](../../libatalk/dsi/dsi_read.c.md#dsi_read), [dsi_readdone](../../libatalk/dsi/dsi_read.c.md#dsi_readdone), [dsi_readinit](../../libatalk/dsi/dsi_read.c.md#dsi_readinit), [dsi_stream_read_file](../../libatalk/dsi/dsi_stream.c.md#dsi_stream_read_file), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [icon_dtfile](desktop.c.md#icon_dtfile), [iconopen](desktop.c.md#iconopen)

Calls through [`AFPObj::exit`](../../include/atalk/globals.h.md#struct-afpobj): no table assigns this field

Uses file-scope variables: `si`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### dtfile

```c
char * dtfile(const struct vol *vol, uint8_t creator[], char *ext)
```

Defined at lines 811 to 852.

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_rmvappl](appl.c.md#afp_rmvappl), [applopen](appl.c.md#applopen), [icon_dtfile](desktop.c.md#icon_dtfile)

Uses file-scope variables: `hexdig`

### mtoupath

```c
char * mtoupath(const struct vol *vol, char *mpath, cnid_t did, int utf8)
```

Defined at lines 859 to 895.

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [demangle](mangle.c.md#demangle), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_setforkparams](fork.c.md#afp_setforkparams), [catsearch_afp](catsearch.c.md#catsearch_afp), [cname](directory.c.md#cname), [cname_mtouname](directory.c.md#cname_mtouname), [ctoupath](filedir.c.md#ctoupath), [getforkparams](fork.c.md#getforkparams), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork), [set_name](file.c.md#set_name)

### utompath

```c
char * utompath(const struct vol *vol, char *upath, cnid_t id, int utf8)
```

Defined at lines 900 to 935. Declared in [bin/nad/nad.h](../../bin/nad/nad.h.md).

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [mangle](mangle.c.md#mangle)

Called by: [afp_resolveid](file.c.md#afp_resolveid), [catsearch_db](catsearch.c.md#catsearch_db), [check_dirent](enumerate.c.md#check_dirent), [cname_mtouname](directory.c.md#cname_mtouname), [crit_check](catsearch.c.md#crit_check), [dir_add](directory.c.md#dir_add), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata), [nad_header_read](../../bin/nad/nad_adouble.c.md#nad_header_read), [private_demangle](mangle.c.md#private_demangle), [set_name](file.c.md#set_name)

### ad_addcomment

```c
static int ad_addcomment(const AFPObj *obj, struct vol *vol, struct path *path, char *ibuf)
```

Defined at lines 938 to 1016.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [check_access](directory.c.md#check_access), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_findname](ofork.c.md#of_findname), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_addcomment](desktop.c.md#afp_addcomment)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_addcomment

```c
int afp_addcomment(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1019 to 1052.

Calls: [ad_addcomment](desktop.c.md#ad_addcomment), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### ad_getcomment

```c
static int ad_getcomment(struct vol *vol, struct path *path, char *rbuf, size_t *rbuflen)
```

Defined at lines 1055 to 1119.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata), [ad_rlen_meta_absent](../../include/atalk/directory.h.md#ad_rlen_meta_absent), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [of_findname](ofork.c.md#of_findname), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_getcomment](desktop.c.md#afp_getcomment)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_getcomment

```c
int afp_getcomment(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1122 to 1151.

Calls: [ad_getcomment](desktop.c.md#ad_getcomment), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### ad_rmvcomment

```c
static int ad_rmvcomment(const AFPObj *obj, struct vol *vol, struct path *path)
```

Defined at lines 1154 to 1225.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [check_access](directory.c.md#check_access), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_findname](ofork.c.md#of_findname), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_rmvcomment](desktop.c.md#afp_rmvcomment)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_rmvcomment

```c
int afp_rmvcomment(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1228 to 1257.

Calls: [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* Undocumented: `EXEC_MODE`, `min`

# File-scope variables

`hexdig`, `si`, `ucreator`, `usize`, `utag`, `utype`
