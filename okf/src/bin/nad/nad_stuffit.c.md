---
type: C Source File
title: "bin/nad/nad_stuffit.c"
description: "22 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_stuffit.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 47 to 63.

Print the usage of the nad sit and unsit commands.

Parameters:
* `out`: stream for the text

Called by: [nad_stuffit](nad_stuffit.c.md#nad_stuffit)

### set_nad_volume_from_path

```c
static int set_nad_volume_from_path(AFPObj *obj, const char *path, afpvol_t *afpvol, struct nad_volume *volume)
```

Defined at lines 77 to 98.

Open the volume of a path and select it for the nad_adouble calls.

Outside a volume the state marks the volume as a stand-in. A volume without an open CNID database skips the CNIDs.

Parameters:
* `obj`: AFP configuration with the volumes
* `path`: path inside the volume
* `afpvol`: receives the opened volume
* `volume`: receives the volume state for nad_set_volume

Returns: 0, or -1 when the volume does not open

Calls: [nad_set_volume](nad_adouble.c.md#nad_set_volume), [openvol_optional](nad_util.c.md#openvol_optional)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [unsit_archive](nad_stuffit.c.md#unsit_archive)

### entry_name_is_safe

```c
static int entry_name_is_safe(const char *name)
```

Defined at lines 109 to 140.

Tell whether an archive entry name stays inside the extraction directory.

The name has to be relative and free of empty, `.`, and `..` components.

Parameters:
* `name`: entry name from the archive

Returns: 1 for a safe name, or 0

Called by: [unsit_archive](nad_stuffit.c.md#unsit_archive)

### make_parent_dirs

```c
static int make_parent_dirs(const char *path)
```

Defined at lines 150 to 172.

Create the missing parent directories of a relative path.

Each new directory gets mode 0777 less the umask, and existing ones pass.

Parameters:
* `path`: relative file path

Returns: 0, or -1 for a path that is too long or a failed mkdir

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [extract_file_entry](nad_stuffit.c.md#extract_file_entry)

### make_dir_path

```c
static int make_dir_path(const char *path)
```

Defined at lines 182 to 213.

Create a directory path with every component.

Each new directory gets mode 0777 less the umask, and existing ones pass.

Parameters:
* `path`: directory path

Returns: 0, or -1 for a path that is too long or a failed mkdir

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [unsit_archive](nad_stuffit.c.md#unsit_archive)

### path_basename

```c
static const char * path_basename(const char *path)
```

Defined at lines 223 to 227.

Return the part of a path after its last slash.

A path that ends in a slash gives an empty name.

Parameters:
* `path`: path

Returns: the base name inside `path`

Called by: [fill_header_from_entry](nad_stuffit.c.md#fill_header_from_entry), [sit_paths](nad_stuffit.c.md#sit_paths)

### split_parent_base

```c
static int split_parent_base(const char *path, char *parent, size_t parent_len, char *base, size_t base_len)
```

Defined at lines 241 to 270.

Split a path into its parent directory and its base name.

A path without a slash, or with only a leading one, gives `.` as parent.

Parameters:
* `path`: path to split
* `parent`: receives the parent directory
* `parent_len`: size of `parent`
* `base`: receives the base name
* `base_len`: size of `base`

Returns: 0, or -1 for a part that does not fit

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [extract_file_entry](nad_stuffit.c.md#extract_file_entry)

### fill_header_from_entry

```c
static int fill_header_from_entry(const StuffitEntryInfo *info, struct FHeader *fh)
```

Defined at lines 284 to 319.

Build a file header from a StuffIt entry.

The header takes the base name, both fork lengths, the type, the creator, and the Finder flags of the entry. The Mac creation and modification dates turn into AppleDouble dates without a time zone conversion, and a zero date becomes the current time.

Parameters:
* `info`: StuffIt entry
* `fh`: file header to fill

Returns: 0, or -1 for a name that is too long

Calls: [path_basename](nad_stuffit.c.md#path_basename), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [extract_file_entry](nad_stuffit.c.md#extract_file_entry)

### validate_entry_fork_lengths

```c
static int validate_entry_fork_lengths(const StuffitEntryInfo *info, const StuffitBytes *data, const StuffitBytes *rsrc)
```

Defined at lines 330 to 355.

Check the decoded forks of a StuffIt entry against its lengths and the 32-bit limit.

Parameters:
* `info`: StuffIt entry
* `data`: decoded data fork
* `rsrc`: decoded resource fork

Returns: 0, or -1 for a mismatch or a fork of 4 GiB or more

Called by: [unsit_archive](nad_stuffit.c.md#unsit_archive)

### write_fork_bytes

```c
static int write_fork_bytes(int fork, const uint8_t *ptr, size_t len)
```

Defined at lines 365 to 384.

Write a whole fork with nad_write, at most 8 KiB per call.

Parameters:
* `fork`: DATA or RESOURCE
* `ptr`: fork bytes
* `len`: number of bytes

Returns: 0, or -1 for a failed write

Calls: [nad_write](nad_adouble.c.md#nad_write)

Called by: [extract_file_entry](nad_stuffit.c.md#extract_file_entry)

### extract_file_entry

```c
static int extract_file_entry(const StuffitEntryInfo *info, const StuffitBytes *data, const StuffitBytes *rsrc)
```

Defined at lines 400 to 479.

Extract one StuffIt file entry with its forks and Mac metadata.

The call creates the parent directories, changes into the parent, and creates the file through nad_open, which refuses an existing file. It writes both forks and sets the modification time of the file. An error deletes the partial file, and the call always returns to the starting directory.

Parameters:
* `info`: StuffIt entry
* `data`: decoded data fork
* `rsrc`: decoded resource fork

Returns: 0, or -1 for an error

Calls: [fill_header_from_entry](nad_stuffit.c.md#fill_header_from_entry), [make_parent_dirs](nad_stuffit.c.md#make_parent_dirs), [nad_close](nad_adouble.c.md#nad_close), [nad_open](nad_adouble.c.md#nad_open), [split_parent_base](nad_stuffit.c.md#split_parent_base), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [write_fork_bytes](nad_stuffit.c.md#write_fork_bytes)

Called by: [unsit_archive](nad_stuffit.c.md#unsit_archive)

### unsit_archive

```c
static int unsit_archive(const char *path, AFPObj *obj)
```

Defined at lines 492 to 559.

Extract every entry of a StuffIt archive into the current directory.

The metadata goes to the volume of the current directory. An unsafe entry name, a fork length mismatch, or any other error stops the extraction at that entry.

Parameters:
* `path`: archive path
* `obj`: AFP configuration with the volumes

Returns: 0, or -1 for an error

Calls: [closevol](nad_util.c.md#closevol), [entry_name_is_safe](nad_stuffit.c.md#entry_name_is_safe), [extract_file_entry](nad_stuffit.c.md#extract_file_entry), [make_dir_path](nad_stuffit.c.md#make_dir_path), [set_nad_volume_from_path](nad_stuffit.c.md#set_nad_volume_from_path), [validate_entry_fork_lengths](nad_stuffit.c.md#validate_entry_fork_lengths)

Called by: [nad_stuffit](nad_stuffit.c.md#nad_stuffit)

### read_fork_bytes

```c
static int read_fork_bytes(int fork, size_t len, uint8_t **out)
```

Defined at lines 571 to 609.

Read a whole fork of the open file into a buffer from malloc.

An empty fork gives NULL, and a short read fails.

Parameters:
* `fork`: DATA or RESOURCE
* `len`: fork length
* `out`: receives the buffer, which the caller frees

Returns: 0, or -1 for an error

Calls: [nad_read](nad_adouble.c.md#nad_read)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer)

Uses file-scope variables: `forkname` in [bin/nad/megatron.c](megatron.c.md)

### add_file_to_writer

```c
static int add_file_to_writer(StuffitWriter *writer, const char *path, const char *entry_name, AFPObj *obj)
```

Defined at lines 624 to 678.

Add a file with its forks and Mac metadata to a StuffIt writer.

The call selects the volume of the path, opens the file through nad_open, and reads both forks into memory. The entry takes the dates, the type, the creator, and the Finder flags of the file.

Parameters:
* `writer`: StuffIt writer
* `path`: file path
* `entry_name`: name of the entry in the archive
* `obj`: AFP configuration with the volumes

Returns: 0, or -1 for an error

Calls: [closevol](nad_util.c.md#closevol), [nad_close](nad_adouble.c.md#nad_close), [nad_open](nad_adouble.c.md#nad_open), [read_fork_bytes](nad_stuffit.c.md#read_fork_bytes), [set_nad_volume_from_path](nad_stuffit.c.md#set_nad_volume_from_path)

Called by: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer)

### add_folder_to_writer

```c
static int add_folder_to_writer(StuffitWriter *writer, const char *entry_name)
```

Defined at lines 687 to 700.

Add a folder entry to a StuffIt writer.

Parameters:
* `writer`: StuffIt writer
* `entry_name`: name of the folder in the archive

Returns: 0, or -1 for an error

Called by: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer)

### join_path

```c
static int join_path(char *out, size_t out_len, const char *dir, const char *name)
```

Defined at lines 711 to 720.

Join a directory and a name with a slash.

Parameters:
* `out`: buffer for the result
* `out_len`: size of `out`
* `dir`: directory
* `name`: name

Returns: 0, or -1 for a result that does not fit

Called by: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer)

### is_ad_metadata_entry

```c
static int is_ad_metadata_entry(const char *name)
```

Defined at lines 731 to 737.

Tell whether a directory entry holds netatalk or AppleDouble metadata.

The metadata names are `._` files and the .AppleDouble, .AppleDB, and .AppleDesktop folders.

Parameters:
* `name`: entry name

Returns: nonzero for a metadata name

Called by: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer)

### add_path_to_writer

```c
static int add_path_to_writer(StuffitWriter *writer, const char *path, const char *entry_name, AFPObj *obj)
```

Defined at lines 752 to 805.

Add a file or a directory tree to a StuffIt writer.

A directory adds a folder entry and then each child, apart from the metadata names. Any type other than a regular file or a directory fails, symbolic links included.

Parameters:
* `writer`: StuffIt writer
* `path`: path to add
* `entry_name`: name of the entry in the archive
* `obj`: AFP configuration with the volumes

Returns: 0, or -1 for an error

Calls: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [add_folder_to_writer](nad_stuffit.c.md#add_folder_to_writer), [is_ad_metadata_entry](nad_stuffit.c.md#is_ad_metadata_entry), [join_path](nad_stuffit.c.md#join_path)

Called by: [sit_paths](nad_stuffit.c.md#sit_paths)

### default_sit_output

```c
static char * default_sit_output(const char *path)
```

Defined at lines 814 to 826.

Return the default archive path, the input path with `.sit` appended.

Parameters:
* `path`: input path

Returns: the archive path for the caller to free, or NULL when memory runs out

Called by: [sit_paths](nad_stuffit.c.md#sit_paths)

### write_owned_bytes

```c
static int write_owned_bytes(const char *path, StuffitOwnedBytes *bytes)
```

Defined at lines 836 to 857.

Write the bytes of a finished archive to a file and replace any existing file.

Parameters:
* `path`: output path
* `bytes`: archive bytes

Returns: 0, or -1 for an error

Called by: [sit_paths](nad_stuffit.c.md#sit_paths)

### sit_paths

```c
static int sit_paths(int argc, char **argv, AFPObj *obj, const char *output, uint8_t method)
```

Defined at lines 872 to 929.

Build a StuffIt archive from files and directory trees and write it.

Each argument becomes an entry under its base name. Without -o the call needs a single input and writes `input.sit`.

Parameters:
* `argc`: number of inputs
* `argv`: input paths
* `obj`: AFP configuration with the volumes
* `output`: archive path from -o, or NULL
* `method`: StuffIt compression method

Returns: 0, or -1 for an error

Calls: [add_path_to_writer](nad_stuffit.c.md#add_path_to_writer), [default_sit_output](nad_stuffit.c.md#default_sit_output), [path_basename](nad_stuffit.c.md#path_basename), [write_owned_bytes](nad_stuffit.c.md#write_owned_bytes)

Called by: [nad_stuffit](nad_stuffit.c.md#nad_stuffit)

### nad_stuffit

```c
int nad_stuffit(int argc, char **argv, AFPObj *obj)
```

Defined at lines 944 to 1039.

Run the nad sit and unsit commands.

The -h and help options print the usage, and verbose works for both commands. The sit command also takes -o or output for the archive path and method for the compression method, 13 by default. The unsit command extracts each archive into the current directory.

Parameters:
* `argc`: argument count, from the command name on
* `argv`: argument vector, from the command name on
* `obj`: AFP configuration with the volumes

Returns: 0, or 1 for an error

Calls: [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [sit_paths](nad_stuffit.c.md#sit_paths), [unsit_archive](nad_stuffit.c.md#unsit_archive), [usage_stuffit](nad_stuffit.c.md#usage_stuffit)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `nad_log_verbose` in [bin/nad/nad_util.c](nad_util.c.md)

# Macros

* Undocumented: `SIT_DEFAULT_METHOD`
