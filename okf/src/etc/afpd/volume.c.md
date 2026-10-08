---
type: C Source File
title: "etc/afpd/volume.c"
description: "21 functions, includes 28 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/volume.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [acls.h](acls.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/ldapconfig.h](../../include/atalk/ldapconfig.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [etc/spotlight/sl_backends.h](../spotlight/sl_backends.h.md)
* [file.h](file.h.md)
* [fork.h](fork.h.md)
* [hash.h](hash.h.md)
* [mangle.h](mangle.h.md)
* [pfd_cache.h](pfd_cache.h.md)
* [unix.h](unix.h.md)
* [virtual_icon.h](virtual_icon.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `bstrlib.h`, `ctype.h`, `errno.h`, `grp.h`, `iniparser.h`, `inttypes.h`, `netinet/in.h`, `pwd.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `time.h`, `utime.h`

# Functions

### get_tm_bandsize

```c
static long long int get_tm_bandsize(const char *path)
```

Defined at lines 88 to 126.

Read band-size info from Info.plist XML file of an TM sparsebundle.

Parameters:
* `path`: path to Info.plist file

Returns: band-size in bytes, -1 on error

Called by: [get_tm_used](volume.c.md#get_tm_used)

### get_tm_bands

```c
static long long int get_tm_bands(const char *path)
```

Defined at lines 134 to 160.

Return number on entries in a directory.

Parameters:
* `path`: path to dir

Returns: number of entries, -1 on error

Called by: [get_tm_used](volume.c.md#get_tm_used)

### get_tm_used

```c
static int get_tm_used(struct vol *vol)
```

Defined at lines 184 to 260.

Calculate used size of a TimeMachine volume.

This assumes that the volume is used only for TimeMachine.

1. readdir(path of volume)
1. for every element that matches regex "\(.*\)\.sparsebundle$" :
1. parse "\1.sparsebundle/Info.plist" and read the band-size XML key integer value
1. readdir "\1.sparsebundle/bands/" counting files
1. calculate used size as: (file_count - 1) * band-size

The result of the calculation is returned in "volume->v_tm_used". "volume->v_appended" gets reset to 0. "volume->v_tm_cachetime" is updated with the current time from [time(NULL)](../../sys/netatalk/at_control.c.md).

"volume->v_tm_used" is cached for TM_USED_CACHETIME seconds and updated by "volume->v_appended". The latter is increased by X every time the client appends X bytes to a file (in [fork.c](fork.c.md)).

Parameters:
* `vol`: (rw) volume to calculate

Returns: 0 on success, -1 on error

Calls: [get_tm_bands](volume.c.md#get_tm_bands), [get_tm_bandsize](volume.c.md#get_tm_bandsize)

Called by: [getvolspace](volume.c.md#getvolspace)

### getvolspace

```c
static int getvolspace(const AFPObj *obj, struct vol *vol, uint32_t *bfree, uint32_t *btotal, VolSpace *xbfree, VolSpace *xbtotal, uint32_t *bsize)
```

Defined at lines 262 to 312.

Calls: [get_tm_used](volume.c.md#get_tm_used), [uquota_getvolspace](quota.c.md#uquota_getvolspace), [ustatfs_getvolspace](unix.c.md#ustatfs_getvolspace)

Called by: [getvolparams](volume.c.md#getvolparams)

### vol_setdate

```c
static void vol_setdate(uint16_t id, struct adouble *adp, time_t date)
```

Defined at lines 318 to 335.

set volume creation date

Note: avoid duplicate, well at least it tries

Calls: [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes)

Called by: [getvolparams](volume.c.md#getvolparams)

### getvolparams

```c
static int getvolparams(const AFPObj *obj, uint16_t bitmap, struct vol *vol, struct stat *st, char *buf, size_t *buflen)
```

Defined at lines 338 to 585.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [getvolspace](volume.c.md#getvolspace), [ucs2_to_charset](../../libatalk/unicode/charcnv.c.md#ucs2_to_charset), [vol_setdate](volume.c.md#vol_setdate)

Called by: [stat_vol](volume.c.md#stat_vol)

Uses file-scope variables: `ldap_config_valid` in [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md)

### stat_vol

```c
static int stat_vol(const AFPObj *obj, uint16_t bitmap, struct vol *vol, char *rbuf, size_t *rbuflen)
```

Defined at lines 588 to 614.

Calls: [getvolparams](volume.c.md#getvolparams)

Called by: [afp_getvolparams](volume.c.md#afp_getvolparams), [afp_openvol](volume.c.md#afp_openvol)

### afp_getsrvrparms

```c
int afp_getsrvrparms(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 617 to 709. Declared in [etc/afpd/volume.h](volume.h.md).

Calls: [accessmode](unix.c.md#accessmode), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [ucs2_to_charset_allocate](../../libatalk/unicode/charcnv.c.md#ucs2_to_charset_allocate)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### volume_codepage

```c
static int volume_codepage(AFPObj *obj, struct vol *volume)
```

Defined at lines 712 to 755.

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [find_charset_functions](../../libatalk/unicode/iconv.c.md#find_charset_functions)

Called by: [afp_openvol](volume.c.md#afp_openvol)

### volume_openDB

```c
static int volume_openDB(const AFPObj *obj, struct vol *volume)
```

Defined at lines 758 to 775.

Calls: [cnid_open](../../libatalk/cnid/cnid.c.md#cnid_open)

Called by: [afp_openvol](volume.c.md#afp_openvol)

### server_ipc_volumes

```c
static void server_ipc_volumes(AFPObj *obj)
```

Defined at lines 780 to 804.

Send list of open volumes to afpd master via IPC

Calls: [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [ipc_child_write](../../libatalk/util/server_ipc.c.md#ipc_child_write)

Called by: [afp_closevol](volume.c.md#afp_closevol), [afp_openvol](volume.c.md#afp_openvol)

### afp_openvol

```c
int afp_openvol(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 809 to 1075. Declared in [etc/afpd/volume.h](volume.h.md).

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata), [afprun](afprun.c.md#afprun), [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close), [cnid_getstamp](../../libatalk/cnid/cnid.c.md#cnid_getstamp), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [dir_free](directory.c.md#dir_free), [dir_new](directory.c.md#dir_new), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [server_ipc_volumes](volume.c.md#server_ipc_volumes), [setmessage](messages.c.md#setmessage), [stat_vol](volume.c.md#stat_vol), [strcasecmp_w](../../libatalk/unicode/util_unistr.c.md#strcasecmp_w), [virtual_icon_init](virtual_icon.c.md#virtual_icon_init), [volume_codepage](volume.c.md#volume_codepage), [volume_openDB](volume.c.md#volume_opendb)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

Mentioned in the documentation of: [volume_free](../../libatalk/util/netatalk_conf.c.md#volume_free)

### cnid_volume_tag

```c
uint32_t cnid_volume_tag(const struct vol *vol)
```

Defined at lines 1085 to 1100. Declared in [etc/afpd/volume.h](volume.h.md).

Stable cross-process identifier for a volume's CNID table.

v_vid is per-process, so the same vid names different volumes in different children. v_uuid is what every child agrees on.

Returns: FNV-1a of v_uuid, or 0 if the volume has no UUID

Called by: [cnid_volume_reset](volume.c.md#cnid_volume_reset), [process_cache_hints](dircache.c.md#process_cache_hints)

Mentioned in the documentation of: [reset_already_broadcast](../../libatalk/util/server_ipc.c.md#reset_already_broadcast), [reset_broadcast_forget](../../libatalk/util/server_ipc.c.md#reset_broadcast_forget)

### cnid_volume_reset

```c
void cnid_volume_reset(const struct vol *vol)
```

Defined at lines 1110 to 1139. Declared in [etc/afpd/volume.h](volume.h.md).

Announce a CNID table reset and end this session.

Only the session that reset the table sends the hint; a responder that re-broadcast would loop between children. No AFP attention exists for "the ID space was recycled" — the protocol guarantees IDs are never reused — so the disconnect is the signal.

Calls: [cnid_volume_tag](volume.c.md#cnid_volume_tag), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint)

Called by: [get_id](file.c.md#get_id), [reenumerate_loop](file.c.md#reenumerate_loop), [sl_cnid_open_query](../spotlight/cnid/sl_cnid.c.md#sl_cnid_open_query), [sl_xapian_fill_results](../spotlight/xapian/sl_xapian.c.md#sl_xapian_fill_results), [tracker_cursor_cb](../spotlight/localsearch/sl_localsearch.c.md#tracker_cursor_cb)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### closevol

```c
void closevol(const AFPObj *obj, struct vol *vol)
```

Defined at lines 1141 to 1167. Declared in [etc/afpd/volume.h](volume.h.md).

Calls: [afprun](afprun.c.md#afprun), [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close), [dir_free](directory.c.md#dir_free), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [of_closevol](ofork.c.md#of_closevol), [pfd_purge_vol](pfd_cache.c.md#pfd_purge_vol)

Called by: [afp_closevol](volume.c.md#afp_closevol), [close_all_vol](volume.c.md#close_all_vol)

### close_all_vol

```c
void close_all_vol(const AFPObj *obj)
```

Defined at lines 1170 to 1181. Declared in [etc/afpd/volume.h](volume.h.md).

Calls: [closevol](volume.c.md#closevol), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes)

Called by: [afp_asp_close](afp_asp.c.md#afp_asp_close), [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [afp_logout](auth.c.md#afp_logout)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_closevol

```c
int afp_closevol(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1184 to 1207. Declared in [etc/afpd/volume.h](volume.h.md).

Calls: [closevol](volume.c.md#closevol), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [server_ipc_volumes](volume.c.md#server_ipc_volumes)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

Mentioned in the documentation of: [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol)

### pollvoltime

```c
int pollvoltime(AFPObj *)
```

Defined at lines 1222 to 1256. Declared in [etc/afpd/volume.h](volume.h.md).

poll if a volume is changed by other processes.

Parameters:
* `obj`: AFP connection object

Returns: 0 no attention msg sent, 1 attention msg sent, -1 error (socket closed)

Note: if attention return -1 no packet has been sent because the buffer is full, we don't care either there's no reader or there's a lot of traffic and another pollvoltime will follow

Calls: [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes)

Called by: [handle_alarm](afp_dsi.c.md#handle_alarm)

Calls through [`AFPObj::attention`](../../include/atalk/globals.h.md#struct-afpobj): no table assigns this field

### setvoltime

```c
void setvoltime(AFPObj *, struct vol *)
```

Defined at lines 1259 to 1290. Declared in [etc/afpd/volume.h](volume.h.md).

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_rename](filedir.c.md#afp_rename), [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams)

Calls through [`AFPObj::attention`](../../include/atalk/globals.h.md#struct-afpobj): no table assigns this field

### afp_getvolparams

```c
int afp_getvolparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1293 to 1310. Declared in [etc/afpd/volume.h](volume.h.md).

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [stat_vol](volume.c.md#stat_vol)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_setvolparams

```c
int afp_setvolparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1313 to 1356. Declared in [etc/afpd/volume.h](volume.h.md).

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* Undocumented: `TM_USED_CACHETIME`, `VOLPASSLEN`
