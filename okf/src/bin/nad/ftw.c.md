---
type: C Source File
title: "bin/nad/ftw.c"
description: "12 functions, 4 types, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/ftw.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [ftw.h](ftw.h.md)
* System headers: `alloca.h`, `assert.h`, `dirent.h`, `errno.h`, `fcntl.h`, `limits.h`, `search.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `unistd.h`

# Functions

### tdestroy_recurse

```c
static void tdestroy_recurse(node root, __free_fn_t freefct)
```

Defined at lines 201 to 214.

Called by: [mytdestroy](ftw.c.md#mytdestroy)

### mytdestroy

```c
static void mytdestroy(void *vroot, __free_fn_t freefct)
```

Defined at lines 216 to 223.

Calls: [tdestroy_recurse](ftw.c.md#tdestroy_recurse)

Called by: [ftw_startup](ftw.c.md#ftw_startup)

### mystpcpy

```c
static char * mystpcpy(char *dest, const char *src, size_t dest_size)
```

Defined at lines 225 to 240.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [ftw_startup](ftw.c.md#ftw_startup)

### xgetcwd

```c
static char * xgetcwd(void)
```

Defined at lines 242 to 282.

### object_compare

```c
static int object_compare(const void *p1, const void *p2)
```

Defined at lines 285 to 299.

Called by: [add_object](ftw.c.md#add_object), [find_object](ftw.c.md#find_object)

### add_object

```c
static int add_object(struct ftw_data *data, struct STAT *st)
```

Defined at lines 303 to 314.

Calls: [object_compare](ftw.c.md#object_compare)

Called by: [ftw_startup](ftw.c.md#ftw_startup), [process_entry](ftw.c.md#process_entry)

### find_object

```c
static int find_object(struct ftw_data *data, struct STAT *st)
```

Defined at lines 318 to 324.

Calls: [object_compare](ftw.c.md#object_compare)

Called by: [process_entry](ftw.c.md#process_entry)

### open_dir_stream

```c
static int open_dir_stream(int *dfdp, struct ftw_data *data, struct dir_data *dirp)
```

Defined at lines 328 to 469.

Called by: [ftw_dir](ftw.c.md#ftw_dir)

### process_entry

```c
static int process_entry(struct ftw_data *data, struct dir_data *dir, const char *name, size_t namlen)
```

Defined at lines 473 to 568.

Calls: [add_object](ftw.c.md#add_object), [find_object](ftw.c.md#find_object), [ftw_dir](ftw.c.md#ftw_dir)

Called by: [ftw_dir](ftw.c.md#ftw_dir)

### ftw_dir

```c
static int ftw_dir(struct ftw_data *data, struct STAT *st, struct dir_data *old_dir) internal_function
```

Defined at lines 572 to 722.

Calls: [open_dir_stream](ftw.c.md#open_dir_stream), [process_entry](ftw.c.md#process_entry), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [ftw_startup](ftw.c.md#ftw_startup), [process_entry](ftw.c.md#process_entry)

Uses file-scope variables: `upfunc`

### ftw_startup

```c
static int ftw_startup(const char *dir, int is_nftw, nftw_func_t func, dir_notification_func_t up, int descriptors, int flags)
```

Defined at lines 725 to 916.

Calls: [add_object](ftw.c.md#add_object), [ftw_dir](ftw.c.md#ftw_dir), [mystpcpy](ftw.c.md#mystpcpy), [mytdestroy](ftw.c.md#mytdestroy)

Called by: [NFTW_NAME](ftw.c.md#nftw_name)

Uses file-scope variables: `ftw_arr`, `nftw_arr`, `upfunc`

### NFTW_NAME

```c
int NFTW_NAME(const char *path, nftw_func_t func, dir_notification_func_t up, int descriptors, int flags)
```

Defined at lines 921 to 928.

Calls: [ftw_startup](ftw.c.md#ftw_startup)

# Types

### struct dir_data

Defined at line 131.
* `DIR * stream`
* `int streamfd`
* `char * content`

### struct ftw_data

Defined at line 142.
* `struct dir_data ** dirstreams`
* `size_t actdir`
* `size_t maxdir`
* `char * dirbuf`
* `size_t dirbufsize`
* `struct FTW ftw`
* `int flags`
* `const int * cvt_arr`
* `nftw_func_t func`
* `dev_t dev`
* `void * known_objects`

### struct known_object

Defined at line 137.
* `dev_t dev`
* `ino_t ino`

### struct node_t

Defined at line 194.
* `void * key`
* `struct node_t * left`
* `struct node_t * right`
* `unsigned int red`

# Typedefs and enums

* `typedef void(* __free_fn_t`
* `typedef struct node_t * node`

# Macros

* Undocumented: `FTW_NAME`, `FXSTATAT`, `LXSTAT`, `NAMLEN`, `NFTW_NAME`, `NFTW_NEW_NAME`, `NFTW_OLD_NAME`, `PATH_MAX`, `STAT`, `XSTAT`, `__chdir`, `__closedir`, `__fchdir`, `__getcwd`, `__mempcpy`, `__opendir`, `__readdir64`, `__set_errno`, `__tdestroy`, `__tfind`, `__tsearch`, `dirent64`, `internal_function`, `mempcpy`

# File-scope variables

`ftw_arr`, `nftw_arr`, `upfunc`
