---
type: C Source File
title: "libatalk/util/pathconv.c"
description: "1 function, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/pathconv.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)

# Functions

### convert_utf8_to_mac

```c
char * convert_utf8_to_mac(const struct vol *vol, const char *upath)
```

Defined at lines 25 to 63.

Convert a UTF-8 filename to a mangled Mac filename.

Parameters:
* `vol`: volume structure
* `upath`: input path in UTF-8

Returns: pointer to static buffer containing mangled Mac filename

See also: utompath() in [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)

Calls: [convert_charset](../unicode/charcnv.c.md#convert_charset), [strnlen](../compat/misc.c.md#strnlen)

Called by: [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [copy](../../bin/nad/nad_cp.c.md#copy), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid)
