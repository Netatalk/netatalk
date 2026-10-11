---
type: C Source File
title: "bin/nad/nad_set.c"
description: "7 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_set.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Defined at lines 66 to 97.

Print the usage of nad set and the meaning of its flag letters to stdout.

Called by: [nad_set](nad_set.c.md#nad_set)

### change_type

```c
static void change_type(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_type)
```

Defined at lines 110 to 118.

Store a new file type in the Finder info of the metadata.

The call takes the first 4 bytes of `p_new_type`.

Parameters:
* `path`: entry path, unused
* `vol`: volume of the entry, unused
* `st`: stat result of the entry, unused
* `ad`: open metadata
* `p_new_type`: type code of 4 characters

Calls: [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry)

Called by: [nad_set](nad_set.c.md#nad_set)

### change_creator

```c
static void change_creator(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_creator)
```

Defined at lines 131 to 139.

Store a new creator in the Finder info of the metadata.

The call takes the first 4 bytes of `p_new_creator`.

Parameters:
* `path`: entry path, unused
* `vol`: volume of the entry, unused
* `st`: stat result of the entry, unused
* `ad`: open metadata
* `p_new_creator`: creator code of 4 characters

Calls: [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry)

Called by: [nad_set](nad_set.c.md#nad_set)

### change_label

```c
static void change_label(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_label)
```

Defined at lines 153 to 182.

Set the color label in the Finder flags of the metadata.

The label is one of `none`, `grey`, `green`, `violet`, `blue`, `yellow`, `red`, and `orange`, in any case, and another name changes nothing.

Parameters:
* `path`: entry path, unused
* `vol`: volume of the entry, unused
* `st`: stat result of the entry, unused
* `ad`: open metadata
* `p_new_label`: label name

Calls: [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry)

Called by: [nad_set](nad_set.c.md#nad_set)

### change_attributes

```c
static void change_attributes(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_attributes)
```

Defined at lines 197 to 256.

Set or clear AFP attributes from a string of letters.

An uppercase letter sets an attribute, and a lowercase one clears it. Y, P, R, and L stand for system, needs backup, no rename, and no delete. Regular files also take W for no write and O for the ATTRBIT_NOCOPY bit.

Parameters:
* `path`: entry path, unused
* `vol`: volume of the entry, unused
* `st`: stat result of the entry
* `ad`: open metadata
* `p_new_attributes`: attribute letters

Calls: [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_setattr](../../libatalk/adouble/ad_attr.c.md#ad_setattr)

Called by: [nad_set](nad_set.c.md#nad_set)

Uses file-scope variables: `new_attributes`

### change_flags

```c
static void change_flags(char *path, afpvol_t *vol, const struct stat *st, struct adouble *ad, char *p_new_flags)
```

Defined at lines 272 to 377.

Set or clear Finder flags from a string of letters.

An uppercase letter sets a flag, and a lowercase one clears it. D, E, I, C, S, B, V, and A stand for on desktop, hidden extension, inited, custom icon, name locked, bundle, invisible, and alias. Regular files also take M for shared, N for no INIT resources, and T for stationery.

Parameters:
* `path`: entry path, unused
* `vol`: volume of the entry, unused
* `st`: stat result of the entry
* `ad`: open metadata
* `p_new_flags`: flag letters

Calls: [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry)

Called by: [nad_set](nad_set.c.md#nad_set)

Uses file-scope variables: `new_flags`

### nad_set

```c
int nad_set(int argc, char **argv, AFPObj *obj)
```

Defined at lines 392 to 479. Declared in [bin/nad/nad.h](nad.h.md).

Run the nad set command, which changes the Mac metadata of one entry.

The -t and -c options set the type and the creator, -l the color label, -f the Finder flags, and -a the AFP attributes. The call creates missing metadata. Metadata that does not open leaves the entry unchanged, and the call still returns 0. A missing path or a failed stat exits with 1.

Parameters:
* `argc`: argument count, from the set command on
* `argv`: argument vector, from the set command on
* `obj`: AFP configuration with the volumes

Returns: 0, 1 when the volume does not open, or -1 for an unknown option

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [change_attributes](nad_set.c.md#change_attributes), [change_creator](nad_set.c.md#change_creator), [change_flags](nad_set.c.md#change_flags), [change_label](nad_set.c.md#change_label), [change_type](nad_set.c.md#change_type), [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [openvol_optional](nad_util.c.md#openvol_optional), [usage_set](nad_set.c.md#usage_set)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `new_attributes`, `new_creator`, `new_flags`, `new_label`, `new_type`

# Macros

* Undocumented: `ADv2_DIRNAME`, `DIR_DOT_OR_DOTDOT`

# File-scope variables

`labels`, `new_attributes`, `new_creator`, `new_flags`, `new_label`, `new_type`
