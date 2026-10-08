---
type: C Source File
title: "bin/nad/nad_set.c"
description: "7 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_set.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [nad.h](nad.h.md)
* System headers: `ctype.h`, `dirent.h`, `errno.h`, `fcntl.h`, `grp.h`, `limits.h`, `pwd.h`, `stdarg.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### usage_set

```c
static void usage_set(void)
```

Defined at lines 62 to 93.

Called by: [nad_set](nad_set.c.md#nad_set)

### change_type

```c
static void change_type(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_type)
```

Defined at lines 95 to 103.

Called by: [nad_set](nad_set.c.md#nad_set)

### change_creator

```c
static void change_creator(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_creator)
```

Defined at lines 105 to 113.

Called by: [nad_set](nad_set.c.md#nad_set)

### change_label

```c
static void change_label(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_label)
```

Defined at lines 115 to 144.

Called by: [nad_set](nad_set.c.md#nad_set)

Uses file-scope variables: `labels`

### change_attributes

```c
static void change_attributes(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_attributes)
```

Defined at lines 146 to 205.

Calls: [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_setattr](../../libatalk/adouble/ad_attr.c.md#ad_setattr)

Called by: [nad_set](nad_set.c.md#nad_set)

Uses file-scope variables: `new_attributes`

### change_flags

```c
static void change_flags(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_flags)
```

Defined at lines 207 to 312.

Called by: [nad_set](nad_set.c.md#nad_set)

Uses file-scope variables: `new_flags`

### nad_set

```c
int nad_set(int argc, char **argv, AFPObj *obj)
```

Defined at lines 314 to 401. Declared in [bin/nad/nad.h](nad.h.md).

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [change_attributes](nad_set.c.md#change_attributes), [change_creator](nad_set.c.md#change_creator), [change_flags](nad_set.c.md#change_flags), [change_label](nad_set.c.md#change_label), [change_type](nad_set.c.md#change_type), [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [openvol_optional](nad_util.c.md#openvol_optional), [usage_set](nad_set.c.md#usage_set)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `new_attributes`, `new_creator`, `new_flags`, `new_label`, `new_type`

# Macros

* Undocumented: `ADv2_DIRNAME`, `DIR_DOT_OR_DOTDOT`

# File-scope variables

`labels`, `new_attributes`, `new_creator`, `new_flags`, `new_label`, `new_type`
