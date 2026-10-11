---
type: C Source File
title: "etc/afpd/volume.c"
description: "21 functions, includes 28 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/volume.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Defined at lines 280 to 330.

Free and total space of a volume in 32-bit and 64-bit form.

The values come from statfs, limited by the user quota when one applies, and by the Time Machine size limit of the volume. The 32-bit values cap at 0xffffffff, at 0x7fffffff before AFP 2.2 or with AFPVOL_LIMITSIZE, and at 0x01fffe00 for a ProDOS volume.

Parameters:
* `obj`: session object
* `vol`: volume, its space source flag updated
* `bfree`: receives the free bytes, capped to 32 bits
* `btotal`: receives the total bytes, capped to 32 bits
* `xbfree`: receives the free bytes
* `xbtotal`: receives the total bytes
* `bsize`: receives the block size

Returns: AFP_OK, AFPERR_PARAM when statfs failed, or AFPERR_MISC when the Time Machine usage is unknown

Calls: [get_tm_used](volume.c.md#get_tm_used), [uquota_getvolspace](quota.c.md#uquota_getvolspace), [ustatfs_getvolspace](unix.c.md#ustatfs_getvolspace)

Called by: [getvolparams](volume.c.md#getvolparams)

### vol_setdate

```c
static void vol_setdate(uint16_t id, struct adouble *adp, time_t date)
```

Defined at lines 342 to 359.

Set the creation date of a volume, unique among the configured volumes.

While another volume carries the same creation date, the date moves on by one second and the search starts over.

Parameters:
* `id`: volume ID
* `adp`: AppleDouble of the volume root
* `date`: creation date as a Unix time

Calls: [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes)

Called by: [getvolparams](volume.c.md#getvolparams)

### getvolparams

```c
static int getvolparams(const AFPObj *obj, uint16_t bitmap, struct vol *vol, struct stat *st, char *buf, size_t *buflen)
```

Defined at lines 380 to 627.

Pack the volume parameters that bitmap selects into buf.

The call first converts version 2 AppleDouble data of the volume root to the ea layout unless the volume disables that. It then opens or creates the root metadata, and a new root metadata file gets a unique creation date. The attribute word reports read-only state, file IDs, catalog search, UTF-8 names, UNIX privileges, Time Machine, extended attributes, ACLs, and case sensitivity. The name always uses the Mac charset.

Parameters:
* `obj`: session object
* `bitmap`: AFP volume bitmap, VOLPBIT_* bits
* `vol`: volume
* `st`: stat of the volume root
* `buf`: reply buffer receiving the parameters
* `buflen`: receives the bytes written

Returns: AFP_OK, AFPERR_BITMAP for an unknown bit, AFPERR_PARAM when the space lookup failed, or AFPERR_MISC

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [getvolspace](volume.c.md#getvolspace), [ucs2_to_charset](../../libatalk/unicode/charcnv.c.md#ucs2_to_charset), [vol_setdate](volume.c.md#vol_setdate)

Called by: [stat_vol](volume.c.md#stat_vol)

Uses file-scope variables: `ldap_config_valid` in [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md)

### stat_vol

```c
static int stat_vol(const AFPObj *obj, uint16_t bitmap, struct vol *vol, char *rbuf, size_t *rbuflen)
```

Defined at lines 643 to 669.

Reply with the bitmap and the volume parameters it selects.

The stat of the volume root also records the device number of the volume.

Parameters:
* `obj`: session object
* `bitmap`: AFP volume bitmap
* `vol`: volume
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_PARAM when the stat failed, or the error of getvolparams

Calls: [getvolparams](volume.c.md#getvolparams)

Called by: [afp_getvolparams](volume.c.md#afp_getvolparams), [afp_openvol](volume.c.md#afp_openvol)

### afp_getsrvrparms

```c
int afp_getsrvrparms(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 689 to 781. Declared in [etc/afpd/volume.h](volume.h.md).

FPGetSrvrParms: list the volumes that the user may see, with the server time.

The call reloads the volume configuration first. A volume appears when its root is a directory that the user may read and search, unless the volume skips that test with AFPVOL_NOSTAT. Each entry carries the password and Apple II flags and the name. The list stops at 255 volumes or 4600 bytes, the reply size at which clients disconnect.

Parameters:
* `obj`: session object
* `ibuf`: request block, unused
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or AFPERR_PARAM when the server time is unavailable

Calls: [accessmode](unix.c.md#accessmode), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [ucs2_to_charset_allocate](../../libatalk/unicode/charcnv.c.md#ucs2_to_charset_allocate)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### volume_codepage

```c
static int volume_codepage(AFPObj *obj, struct vol *volume)
```

Defined at lines 794 to 837.

Load the volume charset and the Mac charset of a volume.

The volume charset defaults to UTF8 and the Mac charset to the server setting. A volume charset that only iconv provides logs a warning. A Mac charset that is not a client charset fails.

Parameters:
* `obj`: session object
* `volume`: volume whose charsets to set

Returns: 0, or -1 when a charset failed to load

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [find_charset_functions](../../libatalk/unicode/iconv.c.md#find_charset_functions)

Called by: [afp_openvol](volume.c.md#afp_openvol)

### volume_openDB

```c
static int volume_openDB(const AFPObj *obj, struct vol *volume)
```

Defined at lines 848 to 865.

Open the CNID backend of a volume.

A volume with AFPVOL_NODEV opens it with CNID_FLAG_NODEV.

Parameters:
* `obj`: session object, unused
* `volume`: volume receiving the CNID database

Returns: 0, or -1 when cnid_open failed

Calls: [cnid_open](../../libatalk/cnid/cnid.c.md#cnid_open)

Called by: [afp_openvol](volume.c.md#afp_openvol)

### server_ipc_volumes

```c
static void server_ipc_volumes(AFPObj *obj)
```

Defined at lines 874 to 898.

Send the names of the open volumes to the master afpd.

The IPC_VOLUMES message carries the names separated by a comma and a space.

Parameters:
* `obj`: session object

Calls: [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [ipc_child_write](../../libatalk/util/server_ipc.c.md#ipc_child_write)

Called by: [afp_closevol](volume.c.md#afp_closevol), [afp_openvol](volume.c.md#afp_openvol)

### afp_openvol

```c
int afp_openvol(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 919 to 1185. Declared in [etc/afpd/volume.h](volume.h.md).

FPOpenVol: open a volume and return its parameters.

The bitmap must include VOLPBIT_VID. The volume name comes in UTF-8 or the Mac charset, and a volume password must match. An open volume only returns its parameters. Otherwise the call runs the preexec command, loads the charsets, creates the dircache entry of the root, and opens the CNID and Spotlight backends. A failure closes the volume again. The session runs as the user here.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_BITMAP, AFPERR_NOOBJ for an unknown name, AFPERR_ACCESS for a wrong password, AFPERR_PARAM, or AFPERR_MISC

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata), [afprun](afprun.c.md#afprun), [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close), [cnid_getstamp](../../libatalk/cnid/cnid.c.md#cnid_getstamp), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [dir_free](directory.c.md#dir_free), [dir_new](directory.c.md#dir_new), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [server_ipc_volumes](volume.c.md#server_ipc_volumes), [setmessage](messages.c.md#setmessage), [stat_vol](volume.c.md#stat_vol), [strcasecmp_w](../../libatalk/unicode/util_unistr.c.md#strcasecmp_w), [virtual_icon_init](virtual_icon.c.md#virtual_icon_init), [volume_codepage](volume.c.md#volume_codepage), [volume_openDB](volume.c.md#volume_opendb)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `sl_localsearch_ops` in [etc/spotlight/localsearch/sl_localsearch.c](../spotlight/localsearch/sl_localsearch.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### cnid_volume_tag

```c
uint32_t cnid_volume_tag(const struct vol *vol)
```

Defined at lines 1198 to 1213. Declared in [etc/afpd/volume.h](volume.h.md).

Identifier of the CNID table of a volume that every afpd process shares.

The v_vid field differs between processes, so the same value names different volumes in different children. Every child agrees on v_uuid. The tag is the FNV-1a hash of v_uuid, and 0 stays reserved for no tag.

Parameters:
* `vol`: volume, or NULL

Returns: the tag, never 0 for a volume with a UUID, or 0 without one

Called by: [cnid_volume_reset](volume.c.md#cnid_volume_reset), [process_cache_hints](dircache.c.md#process_cache_hints)

Mentioned in the documentation of: [reset_broadcast_forget](../../libatalk/util/server_ipc.c.md#reset_broadcast_forget)

### cnid_volume_reset

```c
void cnid_volume_reset(const struct vol *vol)
```

Defined at lines 1226 to 1255. Declared in [etc/afpd/volume.h](volume.h.md).

Announce a CNID table reset and end this session.

Only the session that reset the table sends the hint, because a responder that sent it on would loop between children. AFP has no attention for a recycled ID space, since the protocol promises that IDs stay unique, so the disconnect is the signal. The SIGTERM takes effect between requests, and the request in flight still gets its reply.

Parameters:
* `vol`: volume whose CNID table the session reset, or NULL

Calls: [cnid_volume_tag](volume.c.md#cnid_volume_tag), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint)

Called by: [get_id](file.c.md#get_id), [reenumerate_loop](file.c.md#reenumerate_loop), [sl_cnid_open_query](../spotlight/cnid/sl_cnid.c.md#sl_cnid_open_query), [sl_xapian_fill_results](../spotlight/xapian/sl_xapian.c.md#sl_xapian_fill_results), [tracker_cursor_cb](../spotlight/localsearch/sl_localsearch.c.md#tracker_cursor_cb)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### closevol

```c
void closevol(const AFPObj *obj, struct vol *vol)
```

Defined at lines 1268 to 1294. Declared in [etc/afpd/volume.h](volume.h.md).

Close a volume and release everything it holds.

The call closes the forks of the volume and retires its parent descriptor slots, which would otherwise pin directories and match after a reopen. It then purges the dircache entries, frees the root entry, closes the CNID database, and runs the postexec command.

Parameters:
* `obj`: session object
* `vol`: volume to close, or NULL

Calls: [afprun](afprun.c.md#afprun), [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close), [dir_free](directory.c.md#dir_free), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [of_closevol](ofork.c.md#of_closevol), [pfd_purge_vol](pfd_cache.c.md#pfd_purge_vol)

Called by: [afp_closevol](volume.c.md#afp_closevol), [close_all_vol](volume.c.md#close_all_vol)

### close_all_vol

```c
void close_all_vol(const AFPObj *obj)
```

Defined at lines 1301 to 1312. Declared in [etc/afpd/volume.h](volume.h.md).

Close every open volume of the session.

Parameters:
* `obj`: session object

Calls: [closevol](volume.c.md#closevol), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes)

Called by: [afp_asp_close](afp_asp.c.md#afp_asp_close), [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [afp_logout](auth.c.md#afp_logout)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_closevol

```c
int afp_closevol(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1328 to 1351. Declared in [etc/afpd/volume.h](volume.h.md).

FPCloseVol: close a volume.

The call moves to the root directory, finishes the deferred child scans of the volume, and closes it. The master afpd then gets the new list of open volumes.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, or AFPERR_PARAM for an unknown volume or a failed chdir

Calls: [closevol](volume.c.md#closevol), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [server_ipc_volumes](volume.c.md#server_ipc_volumes)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### pollvoltime

```c
int pollvoltime(AFPObj *)
```

Defined at lines 1366 to 1400. Declared in [etc/afpd/volume.h](volume.h.md).

poll if a volume is changed by other processes.

Parameters:
* `obj`: AFP connection object

Returns: 0 no attention msg sent, 1 attention msg sent, -1 error (socket closed)

Note: if attention return -1 no packet has been sent because the buffer is full, we don't care either there's no reader or there's a lot of traffic and another pollvoltime will follow

Calls: [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes)

Called by: [handle_alarm](afp_dsi.c.md#handle_alarm)

### setvoltime

```c
void setvoltime(AFPObj *, struct vol *)
```

Defined at lines 1412 to 1443. Declared in [etc/afpd/volume.h](volume.h.md).

Record a change on a volume in its modification time.

The call touches the volume root and raises v_mtime when the second advanced. Before AFP 3.2, with server notifications on, that also sends a volume-changed attention, so the Finder refreshes the free space.

Parameters:
* `obj`: session object
* `vol`: volume that changed

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_rename](filedir.c.md#afp_rename), [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams)

### afp_getvolparams

```c
int afp_getvolparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1456 to 1473. Declared in [etc/afpd/volume.h](volume.h.md).

FPGetVolParms: return the parameters of an open volume.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_PARAM for an unknown volume, or the error of stat_vol

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [stat_vol](volume.c.md#stat_vol)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_setvolparams

```c
int afp_setvolparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1490 to 1533. Declared in [etc/afpd/volume.h](volume.h.md).

FPSetVolParms: set the backup date of a volume.

The backup date is the only parameter that a client may set, and any other bitmap fails with AFPERR_BITMAP. The date lands in the AppleDouble metadata of the volume root.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_BITMAP, AFPERR_VLOCK for a read-only volume, AFPERR_ACCESS, or AFPERR_PARAM

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* `TM_USED_CACHETIME`: Time Machine cache for 60 seconds.
* Undocumented: `VOLPASSLEN`
