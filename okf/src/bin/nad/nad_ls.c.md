---
type: C Source File
title: "bin/nad/nad_ls.c"
description: "16 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_ls.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [nad.h](nad.h.md)
* System headers: `ctype.h`, `dirent.h`, `errno.h`, `fcntl.h`, `grp.h`, `limits.h`, `pwd.h`, `stdarg.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/types.h`, `termios.h`, `time.h`, `unistd.h`

# Functions

### compare_names

```c
static int compare_names(const struct dirent **a, const struct dirent **b)
```

Defined at lines 59 to 62.

Called by: [ad_ls_r](nad_ls.c.md#ad_ls_r)

### check_netatalk_dirs

```c
static const char * check_netatalk_dirs(const char *name)
```

Defined at lines 86 to 95.

Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop".

Returns: pointer to name or NULL.

Called by: [ad_ls_r](nad_ls.c.md#ad_ls_r)

Uses file-scope variables: `netatalk_dirs`

### usage_ls

```c
static void usage_ls(void)
```

Defined at lines 98 to 131.

Called by: [nad_ls](nad_ls.c.md#nad_ls)

### print_numlinks

```c
static void print_numlinks(const struct stat *statp)
```

Defined at lines 133 to 136.

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_owner

```c
static void print_owner(const struct stat *statp)
```

Defined at lines 138 to 147.

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_group

```c
static void print_group(const struct stat *statp)
```

Defined at lines 149 to 158.

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_size

```c
static void print_size(const struct stat *statp)
```

Defined at lines 160 to 172.

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_date

```c
static void print_date(const struct stat *statp)
```

Defined at lines 174 to 196.

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_flags

```c
static void print_flags(char *path, afpvol_t *vol, const struct stat *st)
```

Defined at lines 198 to 417.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_forcegetid](../../libatalk/adouble/ad_attr.c.md#ad_forcegetid), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata)

Called by: [ad_print](nad_ls.c.md#ad_print)

Uses file-scope variables: `labels`

### print_mode

```c
static void print_mode(const struct stat *st)
```

Defined at lines 422 to 486.

Called by: [ad_print](nad_ls.c.md#ad_print)

### get_terminal_width

```c
static int get_terminal_width(void)
```

Defined at lines 490 to 508.

Called by: [print_columns](nad_ls.c.md#print_columns)

### print_name_sanitized

```c
static unsigned int print_name_sanitized(const char *name)
```

Defined at lines 514 to 529.

Print a filename with non-printable characters replaced by '?'.

Returns: the number of characters printed

Called by: [ad_print](nad_ls.c.md#ad_print), [print_columns](nad_ls.c.md#print_columns)

### print_columns

```c
static void print_columns(char **names, unsigned int count)
```

Defined at lines 531 to 579.

Calls: [get_terminal_width](nad_ls.c.md#get_terminal_width), [print_name_sanitized](nad_ls.c.md#print_name_sanitized), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [ad_ls_r](nad_ls.c.md#ad_ls_r)

### ad_print

```c
static int ad_print(char *path, const struct stat *st, afpvol_t *vol)
```

Defined at lines 581 to 603.

Calls: [print_date](nad_ls.c.md#print_date), [print_flags](nad_ls.c.md#print_flags), [print_group](nad_ls.c.md#print_group), [print_mode](nad_ls.c.md#print_mode), [print_name_sanitized](nad_ls.c.md#print_name_sanitized), [print_numlinks](nad_ls.c.md#print_numlinks), [print_owner](nad_ls.c.md#print_owner), [print_size](nad_ls.c.md#print_size)

Called by: [ad_ls_r](nad_ls.c.md#ad_ls_r)

Uses file-scope variables: `ls_l`, `ls_u`

### ad_ls_r

```c
static int ad_ls_r(char *path, afpvol_t *vol)
```

Defined at lines 605 to 809.

Calls: [ad_print](nad_ls.c.md#ad_print), [check_netatalk_dirs](nad_ls.c.md#check_netatalk_dirs), [compare_names](nad_ls.c.md#compare_names), [print_columns](nad_ls.c.md#print_columns), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Called by: [nad_ls](nad_ls.c.md#nad_ls)

Uses file-scope variables: `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `first`, `ls_R`, `ls_a`, `ls_d`, `ls_l`, `recursion`

### nad_ls

```c
int nad_ls(int argc, char **argv, AFPObj *obj)
```

Defined at lines 811 to 923. Declared in [bin/nad/nad.h](nad.h.md).

Calls: [ad_ls_r](nad_ls.c.md#ad_ls_r), [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [usage_ls](nad_ls.c.md#usage_ls)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `first`, `ls_R`, `ls_a`, `ls_d`, `ls_l`, `ls_u`, `recursion`

# Macros

* Undocumented: `ADv2_DIRNAME`, `DIR_DOT_OR_DOTDOT`, `MODE`, `TYPE`

# File-scope variables

`first`, `labels`, `ls_R`, `ls_a`, `ls_d`, `ls_l`, `ls_u`, `netatalk_dirs`, `recursion`
