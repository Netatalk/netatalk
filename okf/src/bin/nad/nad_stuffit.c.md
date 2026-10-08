---
type: C Source File
title: "bin/nad/nad_stuffit.c"
description: "22 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_stuffit.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [megatron.h](megatron.h.md)
* [nad.h](nad.h.md)
* [nad_adouble.h](nad_adouble.h.md)
* System headers: `arpa/inet.h`, `dirent.h`, `errno.h`, `fcntl.h`, `limits.h`, `stdbool.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `stuffit_ffi.h`, `sys/stat.h`, `sys/types.h`, `time.h`, `unistd.h`, `utime.h`

# Functions

### usage_stuffit

```c
static void usage_stuffit(FILE *out)
```

Defined at lines 42 to 58.

Called by: [nad_stuffit](nad_stuffit.c.md#nad_stuffit)

### set_nad_volume_from_path

```c
static int set_nad_volume_from_path(AFPObj *obj, const char *path, afpvol_t *afpvol, struct nad_volume *volume)
```

Defined at lines 60 to 81.

Calls: [nad_set_volume](nad_adouble.c.md#nad_set_volume), [openvol_optional](nad_util.c.md#openvol_optional)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [unsit_archive](nad_stuffit.c.md#unsit_archive)

### entry_name_is_safe

```c
static int entry_name_is_safe(const char *name)
```

Defined at lines 83 to 114.

Called by: [unsit_archive](nad_stuffit.c.md#unsit_archive)

### make_parent_dirs

```c
static int make_parent_dirs(const char *path)
```

Defined at lines 116 to 138.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [extract_file_entry](nad_stuffit.c.md#extract_file_entry)

### make_dir_path

```c
static int make_dir_path(const char *path)
```

Defined at lines 140 to 171.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [unsit_archive](nad_stuffit.c.md#unsit_archive)

### path_basename

```c
static const char * path_basename(const char *path)
```

Defined at lines 173 to 177.

Called by: [fill_header_from_entry](nad_stuffit.c.md#fill_header_from_entry), [sit_paths](nad_stuffit.c.md#sit_paths)

### split_parent_base

```c
static int split_parent_base(const char *path, char *parent, size_t parent_len, char *base, size_t base_len)
```

Defined at lines 179 to 208.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [extract_file_entry](nad_stuffit.c.md#extract_file_entry)

### fill_header_from_entry

```c
static int fill_header_from_entry(const StuffitEntryInfo *info, struct FHeader *fh)
```

Defined at lines 210 to 245.

Calls: [path_basename](nad_stuffit.c.md#path_basename), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [extract_file_entry](nad_stuffit.c.md#extract_file_entry)

### validate_entry_fork_lengths

```c
static int validate_entry_fork_lengths(const StuffitEntryInfo *info, const StuffitBytes *data, const StuffitBytes *rsrc)
```

Defined at lines 247 to 272.

Called by: [unsit_archive](nad_stuffit.c.md#unsit_archive)

### write_fork_bytes

```c
static int write_fork_bytes(int fork, const uint8_t *ptr, size_t len)
```

Defined at lines 274 to 293.

Calls: [nad_write](nad_adouble.c.md#nad_write)

Called by: [extract_file_entry](nad_stuffit.c.md#extract_file_entry)

### extract_file_entry

```c
static int extract_file_entry(const StuffitEntryInfo *info, const StuffitBytes *data, const StuffitBytes *rsrc)
```

Defined at lines 295 to 374.

Calls: [fill_header_from_entry](nad_stuffit.c.md#fill_header_from_entry), [make_parent_dirs](nad_stuffit.c.md#make_parent_dirs), [nad_close](nad_adouble.c.md#nad_close), [nad_open](nad_adouble.c.md#nad_open), [split_parent_base](nad_stuffit.c.md#split_parent_base), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [write_fork_bytes](nad_stuffit.c.md#write_fork_bytes)

Called by: [unsit_archive](nad_stuffit.c.md#unsit_archive)

### unsit_archive

```c
static int unsit_archive(const char *path, AFPObj *obj)
```

Defined at lines 376 to 443.

Calls: [closevol](nad_util.c.md#closevol), [entry_name_is_safe](nad_stuffit.c.md#entry_name_is_safe), [extract_file_entry](nad_stuffit.c.md#extract_file_entry), [make_dir_path](nad_stuffit.c.md#make_dir_path), [set_nad_volume_from_path](nad_stuffit.c.md#set_nad_volume_from_path), [validate_entry_fork_lengths](nad_stuffit.c.md#validate_entry_fork_lengths)

Called by: [nad_stuffit](nad_stuffit.c.md#nad_stuffit)

### read_fork_bytes

```c
static int read_fork_bytes(int fork, size_t len, uint8_t **out)
```

Defined at lines 445 to 483.

Calls: [nad_read](nad_adouble.c.md#nad_read)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer)

Uses file-scope variables: `forkname` in [bin/nad/megatron.c](megatron.c.md)

### add_file_to_writer

```c
static int add_file_to_writer(StuffitWriter *writer, const char *path, const char *entry_name, AFPObj *obj)
```

Defined at lines 485 to 539.

Calls: [closevol](nad_util.c.md#closevol), [nad_close](nad_adouble.c.md#nad_close), [nad_open](nad_adouble.c.md#nad_open), [read_fork_bytes](nad_stuffit.c.md#read_fork_bytes), [set_nad_volume_from_path](nad_stuffit.c.md#set_nad_volume_from_path)

Called by: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer)

### add_folder_to_writer

```c
static int add_folder_to_writer(StuffitWriter *writer, const char *entry_name)
```

Defined at lines 541 to 554.

Called by: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer)

### join_path

```c
static int join_path(char *out, size_t out_len, const char *dir, const char *name)
```

Defined at lines 556 to 565.

Called by: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer)

### is_ad_metadata_entry

```c
static int is_ad_metadata_entry(const char *name)
```

Defined at lines 567 to 573.

Called by: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer)

### add_path_to_writer

```c
static int add_path_to_writer(StuffitWriter *writer, const char *path, const char *entry_name, AFPObj *obj)
```

Defined at lines 575 to 628.

Calls: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [add_folder_to_writer](nad_stuffit.c.md#add_folder_to_writer), [is_ad_metadata_entry](nad_stuffit.c.md#is_ad_metadata_entry), [join_path](nad_stuffit.c.md#join_path)

Called by: [sit_paths](nad_stuffit.c.md#sit_paths)

### default_sit_output

```c
static char * default_sit_output(const char *path)
```

Defined at lines 630 to 642.

Called by: [sit_paths](nad_stuffit.c.md#sit_paths)

### write_owned_bytes

```c
static int write_owned_bytes(const char *path, StuffitOwnedBytes *bytes)
```

Defined at lines 644 to 665.

Called by: [sit_paths](nad_stuffit.c.md#sit_paths)

### sit_paths

```c
static int sit_paths(int argc, char **argv, AFPObj *obj, const char *output, uint8_t method)
```

Defined at lines 667 to 724.

Calls: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer), [default_sit_output](nad_stuffit.c.md#default_sit_output), [path_basename](nad_stuffit.c.md#path_basename), [write_owned_bytes](nad_stuffit.c.md#write_owned_bytes)

Called by: [nad_stuffit](nad_stuffit.c.md#nad_stuffit)

### nad_stuffit

```c
int nad_stuffit(int argc, char **argv, AFPObj *obj)
```

Defined at lines 726 to 821.

Calls: [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [sit_paths](nad_stuffit.c.md#sit_paths), [unsit_archive](nad_stuffit.c.md#unsit_archive), [usage_stuffit](nad_stuffit.c.md#usage_stuffit)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `nad_log_verbose` in [bin/nad/nad_util.c](nad_util.c.md)

# Macros

* Undocumented: `SIT_DEFAULT_METHOD`
