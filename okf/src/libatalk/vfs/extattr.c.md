---
type: C Source File
title: "libatalk/vfs/extattr.c"
description: "14 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/extattr.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/vfs](../vfs.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/types.h`

# Functions

### prefix

```c
static const char * prefix(const char *uname)
```

Defined at lines 84 to 92.

Calls: [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [sys_fgetxattr](extattr.c.md#sys_fgetxattr), [sys_fremovexattr](extattr.c.md#sys_fremovexattr), [sys_fsetxattr](extattr.c.md#sys_fsetxattr), [sys_getxattr](extattr.c.md#sys_getxattr), [sys_lgetxattr](extattr.c.md#sys_lgetxattr), [sys_lremovexattr](extattr.c.md#sys_lremovexattr), [sys_lsetxattr](extattr.c.md#sys_lsetxattr), [sys_removexattr](extattr.c.md#sys_removexattr), [sys_setxattr](extattr.c.md#sys_setxattr)

Uses file-scope variables: `attr_name`

### sys_getxattrfd

```c
int sys_getxattrfd(int fd, const char *uname, int oflag,...)
```

Defined at lines 94 to 118. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Called by: [ad_open_rf_ea](../adouble/ad_open.c.md#ad_open_rf_ea)

### sys_getxattr

```c
ssize_t sys_getxattr(const char *path, const char *name, void *value, size_t size)
```

Defined at lines 120 to 185. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent), [sys_get_easize](ea_sys.c.md#sys_get_easize)

### sys_fgetxattr

```c
ssize_t sys_fgetxattr(int filedes, const char *name, void *value, size_t size)
```

Defined at lines 187 to 248. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [ad_reso_size](../adouble/ad_open.c.md#ad_reso_size), [copy_ea](unix.c.md#copy_ea), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent), [sys_get_easize](ea_sys.c.md#sys_get_easize)

### sys_lgetxattr

```c
ssize_t sys_lgetxattr(const char *path, const char *name, void *value, size_t size)
```

Defined at lines 250 to 305. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_reso_size](../adouble/ad_open.c.md#ad_reso_size), [do_check_ea_support](../util/netatalk_conf.c.md#do_check_ea_support), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent), [sys_get_easize](ea_sys.c.md#sys_get_easize)

### sys_listxattr

```c
ssize_t sys_listxattr(const char *path, char *list, size_t size)
```

Defined at lines 435 to 466. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Called by: [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile), [sys_list_eas](ea_sys.c.md#sys_list_eas)

### sys_flistxattr

```c
ssize_t sys_flistxattr(int filedes, const char *path, char *list, size_t size)
```

Defined at lines 468 to 500. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Called by: [sys_list_eas](ea_sys.c.md#sys_list_eas)

### sys_llistxattr

```c
ssize_t sys_llistxattr(const char *path, char *list, size_t size)
```

Defined at lines 502 to 533. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Called by: [sys_list_eas](ea_sys.c.md#sys_list_eas)

### sys_removexattr

```c
int sys_removexattr(const char *path, const char *name)
```

Defined at lines 535 to 563. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [sys_remove_ea](ea_sys.c.md#sys_remove_ea)

### sys_fremovexattr

```c
int sys_fremovexattr(int filedes, const char *path, const char *name)
```

Defined at lines 565 to 593. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [sys_remove_ea](ea_sys.c.md#sys_remove_ea)

### sys_lremovexattr

```c
int sys_lremovexattr(const char *path, const char *name)
```

Defined at lines 595 to 621. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [sys_remove_ea](ea_sys.c.md#sys_remove_ea)

### sys_setxattr

```c
int sys_setxattr(const char *path, const char *name, const void *value, size_t size, int flags)
```

Defined at lines 623 to 686. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_conv_v22ea_hf](../adouble/ad_conv.c.md#ad_conv_v22ea_hf), [do_check_ea_support](../util/netatalk_conf.c.md#do_check_ea_support), [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile), [sys_set_ea](ea_sys.c.md#sys_set_ea)

### sys_fsetxattr

```c
int sys_fsetxattr(int filedes, const char *name, const void *value, size_t size, int flags)
```

Defined at lines 688 to 754. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_flush_hf](../adouble/ad_flush.c.md#ad_flush_hf), [copy_ea](unix.c.md#copy_ea), [sys_set_ea](ea_sys.c.md#sys_set_ea)

### sys_lsetxattr

```c
int sys_lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags)
```

Defined at lines 756 to 818. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Calls: [prefix](extattr.c.md#prefix)

Called by: [sys_set_ea](ea_sys.c.md#sys_set_ea)

# File-scope variables

`attr_name`
