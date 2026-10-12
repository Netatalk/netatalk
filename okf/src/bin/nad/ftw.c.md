---
type: C Source File
title: "bin/nad/ftw.c"
description: "File tree walker from the GNU C Library for the nad commands."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/ftw.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 216 to 229.

Free a tsearch subtree and pass each item to a free function.

The call frees both subtrees first, then the item and the node. It assumes that a tsearch node of the C library starts with the item pointer, the left link, and the right link.

Parameters:
* `root`: root of the subtree, not NULL
* `freefct`: function that frees an item

Called by: [mytdestroy](ftw.c.md#mytdestroy)

### mytdestroy

```c
static void mytdestroy(void *vroot, __free_fn_t freefct)
```

Defined at lines 239 to 246.

Free a whole tsearch tree, as the GNU tdestroy does.

An empty tree needs nothing.

Parameters:
* `vroot`: root of the tree, or NULL
* `freefct`: function that frees an item

Calls: [tdestroy_recurse](ftw.c.md#tdestroy_recurse)

Called by: [ftw_startup](ftw.c.md#ftw_startup)

### mystpcpy

```c
static char * mystpcpy(char *dest, const char *src, size_t dest_size)
```

Defined at lines 262 to 277.

Store a string in a sized buffer and return the end of the stored string, like stpcpy.

When the string fits, the call returns a pointer to its NUL. A string that does not fit ends cut at the buffer size, and the call then returns `dest + dest_size - 2`, one byte before the NUL. A buffer of size 0 stays untouched.

Parameters:
* `dest`: destination buffer
* `src`: string to store
* `dest_size`: size of `dest`

Returns: the end of the stored string in `dest`

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [ftw_startup](ftw.c.md#ftw_startup)

### xgetcwd

```c
static char * xgetcwd(void)
```

Defined at lines 289 to 329.

Return the current directory in a buffer from malloc.

The buffer starts at PATH_MAX plus 2 bytes and grows by 512 bytes while getcwd fails with ERANGE. The `__getcwd` macro of the walk maps to this call.

Returns: the path for the caller to free, or NULL with the errno of the failure

### object_compare

```c
static int object_compare(const void *p1, const void *p2)
```

Defined at lines 342 to 356.

Order two known objects by inode and then by device, for tsearch.

The walk only needs equality. The call compares field by field, so the padding of the structure plays no part.

Parameters:
* `p1`: first [known_object](ftw.c.md#struct-known_object)
* `p2`: second [known_object](ftw.c.md#struct-known_object)

Returns: a negative value, 0, or a positive value

Called by: [add_object](ftw.c.md#add_object), [find_object](ftw.c.md#find_object)

### add_object

```c
static int add_object(struct ftw_data *data, struct STAT *st)
```

Defined at lines 368 to 379.

Remember the device and the inode of a directory, so the walk enters it once.

Parameters:
* `data`: walk state with the tree of known objects
* `st`: stat result of the directory

Returns: 0, or -1 when memory runs out

Calls: [object_compare](ftw.c.md#object_compare)

Called by: [ftw_startup](ftw.c.md#ftw_startup), [process_entry](ftw.c.md#process_entry)

### find_object

```c
static int find_object(struct ftw_data *data, struct STAT *st)
```

Defined at lines 390 to 396.

Tell whether the walk entered a directory before.

Parameters:
* `data`: walk state with the tree of known objects
* `st`: stat result of the directory

Returns: nonzero for a known directory, or 0

Calls: [object_compare](ftw.c.md#object_compare)

Called by: [process_entry](ftw.c.md#process_entry)

### open_dir_stream

```c
static int open_dir_stream(int *dfdp, struct ftw_data *data, struct dir_data *dirp)
```

Defined at lines 418 to 559.

Open the directory that the walk buffer names and give its stream the next slot.

The walk keeps up to maxdir streams open in a ring. When the next slot holds a stream, the call reads the remaining names of that directory into a list of NUL-terminated names and closes it. The list keeps up to 1 MiB of names and cuts each name at 255 bytes, and further names drop out.

The new stream opens relative to the parent stream while that one is open. Otherwise it opens by base name under FTW_CHDIR, or by full path.

Parameters:
* `dfdp`: descriptor of the parent stream, or NULL for the start directory
* `data`: walk state
* `dirp`: receives the new stream

Returns: 0, or -1 with errno for a failure

Called by: [ftw_dir](ftw.c.md#ftw_dir)

### process_entry

```c
static int process_entry(struct ftw_data *data, struct dir_data *dir, const char *name, size_t namlen)
```

Defined at lines 583 to 678.

Visit one directory entry and report it to the callback or descend into it.

The call skips `.` and `..` and appends the name to the walk buffer. It classifies the entry with stat, or with lstat under FTW_PHYS. Without FTW_PHYS a dangling symbolic link gives FTW_SLN. A stat failure with EACCES or ENOENT gives FTW_NS, and other stat failures end the walk.

Under FTW_MOUNT the call skips entries on another file system than the start. Without FTW_PHYS the call enters each directory once, which stops symbolic link cycles. Under FTW_ACTIONRETVAL a FTW_SKIP_SUBTREE result becomes 0.

Parameters:
* `data`: walk state
* `dir`: directory that holds the entry
* `name`: entry name
* `namlen`: length of `name`

Returns: 0 to go on, -1 for an error, or the nonzero value of the callback

Calls: [add_object](ftw.c.md#add_object), [find_object](ftw.c.md#find_object), [ftw_dir](ftw.c.md#ftw_dir)

Called by: [ftw_dir](ftw.c.md#ftw_dir)

### ftw_dir

```c
static int ftw_dir(struct ftw_data *data, struct STAT *st, struct dir_data *old_dir)
```

Defined at lines 700 to 850.

Walk one directory, report it, visit its entries, and return to its parent.

A directory that does not open for EACCES goes to the callback as FTW_DNR, and other open errors end the walk. The callback sees the directory with FTW_D before its entries, or with FTW_DP after them under FTW_DEPTH. Under FTW_CHDIR the call changes into the directory and back to the parent afterwards.

After the entries the call runs the up function of nftw, which nad uses to step its directory IDs back to the parent.

Parameters:
* `data`: walk state
* `st`: stat result of the directory
* `old_dir`: parent directory, or NULL for the start directory

Returns: 0 to go on, -1 for an error, or the nonzero value of the callback

Calls: [open_dir_stream](ftw.c.md#open_dir_stream), [process_entry](ftw.c.md#process_entry), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [ftw_startup](ftw.c.md#ftw_startup), [process_entry](ftw.c.md#process_entry)

Uses file-scope variables: `upfunc`

### ftw_startup

```c
static int ftw_startup(const char *dir, int is_nftw, nftw_func_t func, dir_notification_func_t up, int descriptors, int flags)
```

Defined at lines 873 to 1064.

Set up a walk from a start path, run it, and restore the current directory.

An empty path fails with ENOENT. The walk keeps up to `descriptors` directory streams open, and at least one. The walk buffer drops the trailing slashes of the path. Under FTW_CHDIR the call changes into the directory that holds the start and returns to the current directory at the end. A start directory goes to ftw_dir, and any other start goes straight to the callback.

Parameters:
* `dir`: start path, not empty
* `is_nftw`: nonzero for the flag values of nftw, 0 for those of ftw
* `func`: callback for each object
* `up`: function to run after each directory, or NULL
* `descriptors`: limit of open directory streams
* `flags`: FTW_* flags of nftw

Returns: 0, -1 for an error, or the nonzero value of the callback

Calls: [add_object](ftw.c.md#add_object), [ftw_dir](ftw.c.md#ftw_dir), [mystpcpy](ftw.c.md#mystpcpy), [mytdestroy](ftw.c.md#mytdestroy)

Called by: [nftw](ftw.c.md#nftw)

Uses file-scope variables: `ftw_arr`, `nftw_arr`, `upfunc`

### nftw

```c
int nftw(const char *path, nftw_func_t func, dir_notification_func_t up, int descriptors, int flags)
```

Defined at lines 1082 to 1089.

Walk a file tree like the POSIX nftw, with a function to run after each directory.

The call runs ftw_startup with the nftw flag values. The `up` function runs after the entries of each directory, before its FTW_DP report.

Parameters:
* `path`: start path
* `func`: callback for each object
* `up`: function to run after each directory, or NULL
* `descriptors`: limit of open directory streams
* `flags`: FTW_* flags

Returns: 0, -1 for an error, or the nonzero value of the callback

Calls: [ftw_startup](ftw.c.md#ftw_startup)

Called by: [nad_cp](nad_cp.c.md#nad_cp), [nad_rm](nad_rm.c.md#nad_rm)

# Types

### struct dir_data

Defined at line 136.
* `DIR * stream`
* `int streamfd`
* `char * content`

### struct ftw_data

Defined at line 147.
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

Defined at line 142.
* `dev_t dev`
* `ino_t ino`

### struct node_t

Defined at line 199.
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
