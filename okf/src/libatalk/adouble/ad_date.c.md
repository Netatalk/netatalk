---
type: C Source File
title: "libatalk/adouble/ad_date.c"
description: "3 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_date.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `string.h`, `sys/time.h`, `time.h`

# Functions

### set_utc_offset

```c
int set_utc_offset(uint32_t *aint_p, ad_time_offset_dir_t offset_dir)
```

Defined at lines 31 to 71.

Convert an AFP date-time value between local time and UTC.

AD_DATE_START, the earliest AFP date, passes through unchanged. The conversion to UTC can be an hour off during the transition to or from daylight saving time.

Parameters:
* `aint_p`: AFP date-time to convert, replaced by the result
* `offset_dir`: TO_LOCALTIME or TO_UTC

Returns: 0 on success, -1 when the time conversion fails

Called by: [getdirparams](../../etc/afpd/directory.c.md#getdirparams), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [macbin_ad_date_from_header](../../bin/nad/macbin.c.md#macbin_ad_date_from_header), [macbin_ad_date_to_header](../../bin/nad/macbin.c.md#macbin_ad_date_to_header), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

### ad_setdate

```c
int ad_setdate(struct adouble *ad, unsigned int dateoff, uint32_t date)
```

Defined at lines 84 to 111.

Store a date in the AppleDouble file dates entry.

Parameters:
* `ad`: AppleDouble with its metadata loaded
* `dateoff`: AD_DATE_CREATE, AD_DATE_MODIFY, AD_DATE_BACKUP, or AD_DATE_ACCESS, plus AD_DATE_UNIX when date is a Unix time
* `date`: date to store, a Unix time when dateoff carries AD_DATE_UNIX

Returns: 0, or -1 when the entry is missing or dateoff is out of range

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [copy](../../bin/nad/nad_cp.c.md#copy), [flushfork](../../etc/afpd/fork.c.md#flushfork), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [nad_header_write](../../bin/nad/nad_adouble.c.md#nad_header_write), [new_ad_header](ad_open.c.md#new_ad_header), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [vol_setdate](../../etc/afpd/volume.c.md#vol_setdate)

### ad_getdate

```c
int ad_getdate(const struct adouble *ad, unsigned int dateoff, uint32_t *date)
```

Defined at lines 122 to 150.

Read a date from the AppleDouble file dates entry.

Parameters:
* `ad`: AppleDouble with its metadata loaded
* `dateoff`: AD_DATE_CREATE, AD_DATE_MODIFY, AD_DATE_BACKUP, or AD_DATE_ACCESS, plus AD_DATE_UNIX to receive a Unix time
* `date`: receives the date

Returns: 0, or -1 when the entry is missing or dateoff is out of range

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [getdirparams](../../etc/afpd/directory.c.md#getdirparams), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [nad_header_read](../../bin/nad/nad_adouble.c.md#nad_header_read), [nad_header_write](../../bin/nad/nad_adouble.c.md#nad_header_write)
