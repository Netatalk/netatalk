---
type: C Source File
title: "bin/nad/nad_find.c"
description: "6 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_find.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [nad.h](nad.h.md)
* System headers: `bstrlib.h`, `dirent.h`, `errno.h`, `limits.h`, `stdarg.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### usage_find

```c
static void usage_find(void)
```

Defined at lines 45 to 55.

Print the usage of nad find to stdout.

Called by: [nad_find](nad_find.c.md#nad_find)

### filesystem_find

```c
static int filesystem_find(const char *path, const char *needle, int *found)
```

Defined at lines 70 to 123.

Search a directory tree outside a volume for names that contain a string.

The call prints each path whose last component contains `needle`, and it descends into directories without following symbolic links. The match is case-sensitive.

Parameters:
* `path`: path to search from
* `needle`: string to look for
* `found`: receives 1 after a match

Returns: 0, or -1 for an error, which ends the search

Called by: [nad_find](nad_find.c.md#nad_find)

### bstr_list_push

```c
int bstr_list_push(struct bstrList *sl, bstring bs)
```

Defined at lines 132 to 141.

Append a bstring to a list and grow the list when it is full.

Parameters:
* `sl`: list
* `bs`: string, which the list takes over

Returns: BSTR_OK, or BSTR_ERR when the list does not grow

Called by: [nad_find](nad_find.c.md#nad_find)

### bstr_list_create_min

```c
struct bstrList * bstr_list_create_min(int min)
```

Defined at lines 149 to 163.

Create an empty list with room for at least `min` entries.

Parameters:
* `min`: number of entries to allocate

Returns: the list, or NULL when memory runs out

Called by: [nad_find](nad_find.c.md#nad_find)

### bjoin_inv

```c
bstring bjoin_inv(const struct bstrList *bl, const bstring sep)
```

Defined at lines 177 to 239.

Join the strings of a list in reverse order, with an optional separator between them.

The nad find command collects the path components from the entry up to the volume root. The reverse order then gives the path from the root.

Parameters:
* `bl`: list of strings
* `sep`: separator, or NULL for none

Returns: the joined string, or NULL for an invalid list or when memory runs out

Called by: [nad_find](nad_find.c.md#nad_find)

### nad_find

```c
int nad_find(int argc, char **argv, AFPObj *obj)
```

Defined at lines 254 to 379. Declared in [bin/nad/nad.h](nad.h.md).

Run the nad find command, which searches a volume for entries by name.

The -v option names the volume, and the current directory is the default. In a volume the call lowercases the name, asks the CNID database for matches, and prints the full path of each. Outside a volume it searches the directory tree for names that contain the string.

Parameters:
* `argc`: argument count, starting one word before find
* `argv`: argument vector, starting one word before find
* `obj`: AFP configuration with the volumes

Returns: 0 when a match turns up, or 1, while usage errors exit with 1

Calls: [bjoin_inv](nad_find.c.md#bjoin_inv), [bstr_list_create_min](nad_find.c.md#bstr_list_create_min), [bstr_list_push](nad_find.c.md#bstr_list_push), [closevol](nad_util.c.md#closevol), [cnid_find](../../libatalk/cnid/cnid.c.md#cnid_find), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [filesystem_find](nad_find.c.md#filesystem_find), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [usage_find](nad_find.c.md#usage_find)

Called by: [main](nad.c.md#main)
