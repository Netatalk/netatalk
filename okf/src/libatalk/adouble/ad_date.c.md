---
type: C Source File
title: "libatalk/adouble/ad_date.c"
description: "3 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_date.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 25 to 65.

Convert an AFP date-time value between local time and UTC.

Parameters:
* `aint_p`: AFP date-time to be converted.
* `offset_dir`: Which direction we want to convert the timestamp.

Called by: [getdirparams](../../etc/afpd/directory.c.md#getdirparams), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [macbin_ad_date_from_header](../../bin/nad/macbin.c.md#macbin_ad_date_from_header), [macbin_ad_date_to_header](../../bin/nad/macbin.c.md#macbin_ad_date_to_header), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

### ad_setdate

```c
int ad_setdate(struct adouble *ad, unsigned int dateoff, uint32_t date)
```

Defined at lines 67 to 94.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [copy](../../bin/nad/nad_cp.c.md#copy), [flushfork](../../etc/afpd/fork.c.md#flushfork), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [nad_header_write](../../bin/nad/nad_adouble.c.md#nad_header_write), [new_ad_header](ad_open.c.md#new_ad_header), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [vol_setdate](../../etc/afpd/volume.c.md#vol_setdate)

### ad_getdate

```c
int ad_getdate(const struct adouble *ad, unsigned int dateoff, uint32_t *date)
```

Defined at lines 96 to 124.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [getdirparams](../../etc/afpd/directory.c.md#getdirparams), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [nad_header_read](../../bin/nad/nad_adouble.c.md#nad_header_read), [nad_header_write](../../bin/nad/nad_adouble.c.md#nad_header_write)

Mentioned in the documentation of: [ad_rebuild_from_cache](../../etc/afpd/ad_cache.c.md#ad_rebuild_from_cache)
