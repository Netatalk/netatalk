---
type: C Source File
title: "bin/nad/nad_ls.c"
description: "The nad ls command: list entries with their Mac metadata."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_ls.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 71 to 74.

Order two directory entries by name with strcmp, for scandir.

Parameters:
* `a`: first entry
* `b`: second entry

Returns: a negative value, 0, or a positive value

Called by: [ad_ls_r](nad_ls.c.md#ad_ls_r)

### check_netatalk_dirs

```c
static const char * check_netatalk_dirs(const char *name)
```

Defined at lines 101 to 110.

Tell whether a name is one of the netatalk folders .AppleDouble, .AppleDB, and .AppleDesktop.

Parameters:
* `name`: directory entry name

Returns: the matching table entry, or NULL

Called by: [ad_ls_r](nad_ls.c.md#ad_ls_r)

### usage_ls

```c
static void usage_ls(void)
```

Defined at lines 117 to 150.

Print the usage of nad ls and the meaning of its flag letters to stdout.

Called by: [nad_ls](nad_ls.c.md#nad_ls)

### print_numlinks

```c
static void print_numlinks(const struct stat *statp)
```

Defined at lines 157 to 160.

Print the link count of an entry, five characters wide.

Parameters:
* `statp`: stat result of the entry

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_owner

```c
static void print_owner(const struct stat *statp)
```

Defined at lines 167 to 176.

Print the owner name of an entry, or the UID when the name is unknown.

Parameters:
* `statp`: stat result of the entry

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_group

```c
static void print_group(const struct stat *statp)
```

Defined at lines 183 to 192.

Print the group name of an entry, or the GID when the name is unknown.

Parameters:
* `statp`: stat result of the entry

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_size

```c
static void print_size(const struct stat *statp)
```

Defined at lines 201 to 213.

Print the size of an entry, or the major and minor numbers of a device.

The device numbers come from bits 15 to 8 and bits 7 to 0 of st_rdev.

Parameters:
* `statp`: stat result of the entry

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_date

```c
static void print_date(const struct stat *statp)
```

Defined at lines 223 to 245.

Print the modification time of an entry as ls shows it.

Times within the last half year show the hour and the minute, and older or future times show the year.

Parameters:
* `statp`: stat result of the entry

Called by: [ad_print](nad_ls.c.md#ad_print)

### print_flags

```c
static void print_flags(char *path, afpvol_t *vol, const struct stat *st)
```

Defined at lines 261 to 480.

Print the Mac metadata columns of an entry for nad ls -l.

The columns hold the Finder flags, the AFP attributes, the color label, the type, the creator, and the CNID from the AppleDouble metadata. A lowercase letter marks a set flag, and an uppercase one a flag that a directory should not carry. Without metadata the columns hold dashes. Outside a volume the CNID column shows `!NO-CNID`, and a missing CNID shows `!ADVOL_CACHE`.

Parameters:
* `path`: entry path
* `vol`: volume of the entry
* `st`: stat result of the entry

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_forcegetid](../../libatalk/adouble/ad_attr.c.md#ad_forcegetid), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata)

Called by: [ad_print](nad_ls.c.md#ad_print)

Uses file-scope variables: `labels`

### print_mode

```c
static void print_mode(const struct stat *st)
```

Defined at lines 491 to 555.

Print the type letter and the permission bits of an entry as ls shows them.

Parameters:
* `st`: stat result of the entry

Called by: [ad_print](nad_ls.c.md#ad_print)

### get_terminal_width

```c
static int get_terminal_width(void)
```

Defined at lines 567 to 585.

Return the width of the terminal for column output.

The width comes from the window size of stdout, then from COLUMNS, and the default is 80.

Returns: the width in characters

Called by: [print_columns](nad_ls.c.md#print_columns)

### print_name_sanitized

```c
static unsigned int print_name_sanitized(const char *name)
```

Defined at lines 594 to 609.

Print a file name with a question mark for each character that does not print.

Parameters:
* `name`: file name

Returns: the number of characters printed

Called by: [ad_print](nad_ls.c.md#ad_print), [print_columns](nad_ls.c.md#print_columns)

### print_columns

```c
static void print_columns(char **names, unsigned int count)
```

Defined at lines 620 to 668.

Print names in columns that fit the terminal width, filling each column first.

Each column is two characters wider than the longest name.

Parameters:
* `names`: names to print
* `count`: number of names

Calls: [get_terminal_width](nad_ls.c.md#get_terminal_width), [print_name_sanitized](nad_ls.c.md#print_name_sanitized), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [ad_ls_r](nad_ls.c.md#ad_ls_r)

### ad_print

```c
static int ad_print(char *path, const struct stat *st, afpvol_t *vol)
```

Defined at lines 681 to 703.

Print one entry, as a plain name or as a long line under -l.

The long line starts with the Unix columns under -u, followed by the Mac metadata columns and the name.

Parameters:
* `path`: entry path
* `st`: stat result of the entry
* `vol`: volume of the entry

Returns: 0

Calls: [print_date](nad_ls.c.md#print_date), [print_flags](nad_ls.c.md#print_flags), [print_group](nad_ls.c.md#print_group), [print_mode](nad_ls.c.md#print_mode), [print_name_sanitized](nad_ls.c.md#print_name_sanitized), [print_numlinks](nad_ls.c.md#print_numlinks), [print_owner](nad_ls.c.md#print_owner), [print_size](nad_ls.c.md#print_size)

Called by: [ad_ls_r](nad_ls.c.md#ad_ls_r)

Uses file-scope variables: `ls_l`, `ls_u`

### ad_ls_r

```c
static int ad_ls_r(char *path, afpvol_t *vol)
```

Defined at lines 718 to 922.

List a file, or the entries of a directory, and descend under -R.

For a regular file, or under -d, the call prints the entry itself. It lists a directory from inside, sorted by name, in columns or long lines. Dot files need -a, and the netatalk folders of a volume never show. Under -R the call descends into each subdirectory with a header line, and at the end it returns to the starting directory.

Parameters:
* `path`: entry path
* `vol`: volume of the entry

Returns: 0, or -1 for an error or a signal

Calls: [ad_print](nad_ls.c.md#ad_print), [check_netatalk_dirs](nad_ls.c.md#check_netatalk_dirs), [compare_names](nad_ls.c.md#compare_names), [print_columns](nad_ls.c.md#print_columns), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Called by: [nad_ls](nad_ls.c.md#nad_ls)

Uses file-scope variables: `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `first`, `ls_R`, `ls_a`, `ls_d`, `ls_l`, `recursion`

### nad_ls

```c
int nad_ls(int argc, char **argv, AFPObj *obj)
```

Defined at lines 937 to 1049. Declared in [bin/nad/nad.h](nad.h.md).

Run the nad ls command.

The options are -a, -d, -l, -R, and -u. Without a path the call lists the current directory. Otherwise it lists the file arguments first and then each directory with a header. The call skips paths that do not stat without a message, and listing errors leave the exit status at 0.

Parameters:
* `argc`: argument count, from the ls command on
* `argv`: argument vector, from the ls command on
* `obj`: AFP configuration with the volumes

Returns: 0, 1 when a volume does not open, or -1 for an unknown option

Calls: [ad_ls_r](nad_ls.c.md#ad_ls_r), [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [usage_ls](nad_ls.c.md#usage_ls)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `first`, `ls_R`, `ls_a`, `ls_d`, `ls_l`, `ls_u`, `recursion`

# Macros

* Undocumented: `ADv2_DIRNAME`, `DIR_DOT_OR_DOTDOT`, `MODE`, `TYPE`

# File-scope variables

`first`, `labels`, `ls_R`, `ls_a`, `ls_d`, `ls_l`, `ls_u`, `netatalk_dirs`, `recursion`
