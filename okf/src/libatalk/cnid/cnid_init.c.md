---
type: C Source File
title: "libatalk/cnid/cnid_init.c"
description: "initialization stuff for CNID backends."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/cnid_init.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/cnid](../cnid.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/list.h](../../include/atalk/list.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `stdlib.h`

# Functions

### cnid_init

```c
void cnid_init(void)
```

Defined at lines 41 to 47. Declared in [include/atalk/cnid.h](../../include/atalk/cnid.h.md).

Calls: [cnid_register](cnid.c.md#cnid_register)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../etc/afpd/main.c.md#main), [nad_archive](../../bin/nad/megatron.c.md#nad_archive), [nad_cp](../../bin/nad/nad_cp.c.md#nad_cp), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [nad_ls](../../bin/nad/nad_ls.c.md#nad_ls), [nad_mkdir](../../bin/nad/nad_mkdir.c.md#nad_mkdir), [nad_mv](../../bin/nad/nad_mv.c.md#nad_mv), [nad_rm](../../bin/nad/nad_rm.c.md#nad_rm), [nad_rmdir](../../bin/nad/nad_rmdir.c.md#nad_rmdir), [nad_set](../../bin/nad/nad_set.c.md#nad_set), [nad_stuffit](../../bin/nad/nad_stuffit.c.md#nad_stuffit)

Uses file-scope variables: `cnid_sqlite_module` in [libatalk/cnid/sqlite/cnid_sqlite.c](sqlite/cnid_sqlite.c.md)
