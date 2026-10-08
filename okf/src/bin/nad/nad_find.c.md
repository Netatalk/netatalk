---
type: C Source File
title: "bin/nad/nad_find.c"
description: "6 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_find.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 42 to 52.

Called by: [nad_find](nad_find.c.md#nad_find)

### filesystem_find

```c
static int filesystem_find(const char *path, const char *needle, int *found)
```

Defined at lines 54 to 107.

Called by: [nad_find](nad_find.c.md#nad_find)

### bstr_list_push

```c
int bstr_list_push(struct bstrList *sl, bstring bs)
```

Defined at lines 112 to 121.

Push a bstring to the end of a list.

Called by: [nad_find](nad_find.c.md#nad_find)

### bstr_list_create_min

```c
struct bstrList * bstr_list_create_min(int min)
```

Defined at lines 126 to 140.

Create an empty list with preallocated storage for at least 'min' members.

Called by: [nad_find](nad_find.c.md#nad_find)

### bjoin_inv

```c
bstring bjoin_inv(const struct bstrList *bl, const bstring sep)
```

Defined at lines 145 to 207.

Inverse bjoin.

Called by: [nad_find](nad_find.c.md#nad_find)

### nad_find

```c
int nad_find(int argc, char **argv, AFPObj *obj)
```

Defined at lines 209 to 334. Declared in [bin/nad/nad.h](nad.h.md).

Calls: [bjoin_inv](nad_find.c.md#bjoin_inv), [bstr_list_create_min](nad_find.c.md#bstr_list_create_min), [bstr_list_push](nad_find.c.md#bstr_list_push), [closevol](nad_util.c.md#closevol), [cnid_find](../../libatalk/cnid/cnid.c.md#cnid_find), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [filesystem_find](nad_find.c.md#filesystem_find), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [usage_find](nad_find.c.md#usage_find)

Called by: [main](nad.c.md#main)
