---
type: C Source File
title: "bin/nad/megatron.c"
description: "25 functions, 1 type, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/megatron.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [hqx.h](hqx.h.md)
* [macbin.h](macbin.h.md)
* [megatron.h](megatron.h.md)
* [nad.h](nad.h.md)
* [nad_adouble.h](nad_adouble.h.md)
* [netatalk/endian.h](../../sys/netatalk/endian.h.md)
* System headers: `errno.h`, `fcntl.h`, `pwd.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### usage_archive

```c
static void usage_archive(FILE *out)
```

Defined at lines 58 to 77.

Print the usage of the nad archive commands bin, hex, unbin, and unhex.

Parameters:
* `out`: stream for the text

Called by: [nad_archive](megatron.c.md#nad_archive)

### parse_archive_mode

```c
static int parse_archive_mode(const char *name)
```

Defined at lines 85 to 94.

Map an archive command name to its conversion mode.

Parameters:
* `name`: `bin`, `hex`, `unbin`, or `unhex`

Returns: NAD2BIN, NAD2HEX, BIN2NAD, or HEX2NAD, or -1 for another name

Uses file-scope variables: `archive_modes`

### mode_writes_metadata

```c
static int mode_writes_metadata(int module)
```

Defined at lines 102 to 105.

Tell whether a mode decodes an archive into a file with Mac metadata.

Parameters:
* `module`: conversion mode

Returns: nonzero for BIN2NAD and HEX2NAD

Called by: [mode_creates_volume_file](megatron.c.md#mode_creates_volume_file), [nad_archive_convert](megatron.c.md#nad_archive_convert), [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### mode_reads_metadata

```c
static int mode_reads_metadata(int module)
```

Defined at lines 114 to 117.

Tell whether a mode encodes a file and its Mac metadata into an archive.

Parameters:
* `module`: conversion mode

Returns: nonzero for NAD2BIN and NAD2HEX

Called by: [mode_creates_volume_file](megatron.c.md#mode_creates_volume_file), [nad_archive](megatron.c.md#nad_archive), [nad_archive_convert](megatron.c.md#nad_archive_convert)

### mode_creates_volume_file

```c
static int mode_creates_volume_file(int module, int flags)
```

Defined at lines 129 to 140.

Tell whether a conversion creates a file in the current directory.

Decoding always creates one. Encoding creates the archive file unless stdout writes the archive to stdout, and header creates nothing.

Parameters:
* `module`: conversion mode
* `flags`: OPTION_* flags

Returns: nonzero when the conversion creates a file

Calls: [mode_reads_metadata](megatron.c.md#mode_reads_metadata), [mode_writes_metadata](megatron.c.md#mode_writes_metadata)

Called by: [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### from_open

```c
static int from_open(int un, char *file, struct FHeader *fh, int flags)
```

Defined at lines 154 to 170.

Open the input of a conversion and read its header.

Decoding reads a BinHex or a MacBinary archive, and encoding reads a file and its metadata through nad_open.

Parameters:
* `un`: conversion mode
* `file`: input path, or `-` for stdin
* `fh`: receives the file header
* `flags`: OPTION_* flags

Returns: 0, or -1 for an error

Calls: [bin_open](macbin.c.md#bin_open), [hqx_open](hqx.c.md#hqx_open), [nad_open](nad_adouble.c.md#nad_open)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### from_read

```c
static ssize_t from_read(int un, int fork, char *buf, size_t len)
```

Defined at lines 182 to 198.

Read the next bytes of a fork from the conversion input.

Parameters:
* `un`: conversion mode
* `fork`: DATA or RESOURCE
* `buf`: destination buffer
* `len`: size of `buf`

Returns: the number of bytes read, 0 at the end of the fork, or -1 for an error

Calls: [bin_read](macbin.c.md#bin_read), [hqx_read](hqx.c.md#hqx_read), [nad_read](nad_adouble.c.md#nad_read)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### from_close

```c
static int from_close(int un)
```

Defined at lines 206 to 222.

Close the input of a conversion.

Parameters:
* `un`: conversion mode

Returns: 0, or -1 for an error

Calls: [bin_close](macbin.c.md#bin_close), [hqx_close](hqx.c.md#hqx_close), [nad_close](nad_adouble.c.md#nad_close)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### to_open

```c
static int to_open(int to, char *file, struct FHeader *fh, int flags)
```

Defined at lines 237 to 253.

Create the output of a conversion from a file header.

Decoding creates a file with its metadata through nad_open, and encoding creates a MacBinary or a BinHex archive. The output takes its name from the header and must not exist yet.

Parameters:
* `to`: conversion mode
* `file`: input path, unused by the writers
* `fh`: file header of the input
* `flags`: OPTION_* flags

Returns: 0, or -1 for an error

Calls: [bin_open](macbin.c.md#bin_open), [hqx_open](hqx.c.md#hqx_open), [nad_open](nad_adouble.c.md#nad_open)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### to_write

```c
static ssize_t to_write(int to, int fork, size_t bufc)
```

Defined at lines 263 to 279.

Write the next bytes of a fork from the fork buffer to the output.

Parameters:
* `to`: conversion mode
* `fork`: DATA or RESOURCE
* `bufc`: number of bytes in the fork buffer

Returns: the number of bytes written, or -1 for an error

Calls: [bin_write](macbin.c.md#bin_write), [hqx_write](hqx.c.md#hqx_write), [nad_write](nad_adouble.c.md#nad_write)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

Uses file-scope variables: `forkbuf`

### to_close

```c
static int to_close(int to, int keepflag)
```

Defined at lines 288 to 304.

Close the output of a conversion and keep or delete it.

Parameters:
* `to`: conversion mode
* `keepflag`: KEEP or TRASH

Returns: 0, or -1 for an error

Calls: [bin_close](macbin.c.md#bin_close), [hqx_close](hqx.c.md#hqx_close), [nad_close](nad_adouble.c.md#nad_close)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### created_file_path

```c
static const char * created_file_path(int module)
```

Defined at lines 313 to 325.

Return the path of the archive that an encoding created.

Parameters:
* `module`: conversion mode

Returns: the archive path for NAD2BIN and NAD2HEX, or NULL for the decoding modes

Calls: [bin_path](macbin.c.md#bin_path), [hqx_path](hqx.c.md#hqx_path)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### update_created_file_cnid

```c
static int update_created_file_cnid(const struct nad_volume *volume, const char *path)
```

Defined at lines 340 to 412.

Give a new archive file in a volume its CNID and store the CNID in its metadata.

Stdout, the stand-in volume of force, and a volume without CNIDs need nothing. The call resolves the CNID of the path in the database. It then writes the CNID, the parent ID, and the database stamp into the AppleDouble metadata of the file.

Parameters:
* `volume`: volume state of the conversion
* `path`: archive path, `-` for stdout, or NULL

Returns: 0, or -1 with errno for an error

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### print_header_date

```c
static void print_header_date(const char *label, uint32_t ad_date, int available)
```

Defined at lines 425 to 451.

Print one labeled header date in local time.

A date that the format does not carry prints as `(not available)`. The AD_DATE_START date prints as `(not set)`, and a date that does not convert as `(invalid)`.

Parameters:
* `label`: label of the line
* `ad_date`: AppleDouble date in network byte order
* `available`: nonzero when the header carries the date

Called by: [print_header](megatron.c.md#print_header)

### print_header

```c
static void print_header(const struct FHeader *fh)
```

Defined at lines 461 to 481.

Print the file header of the input for header.

The output lists the name, the comment, the creator, the type, both fork lengths, and the creation, modification, and backup dates.

Parameters:
* `fh`: file header

Calls: [print_header_date](megatron.c.md#print_header_date)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### nad_archive_convert

```c
static int nad_archive_convert(char *path, int module, const char *newname, int flags, const struct nad_volume *volume)
```

Defined at lines 500 to 583.

Convert one file between an archive and a file with Mac metadata.

A directory gets a message and passes. Under header the call only prints the input header. Otherwise it creates the output, moves each fork through a buffer of 8 KiB, and checks the length of each fork against the header. A failed fork deletes the partial output. An encoding without stdout then gives the new archive its CNID.

Parameters:
* `path`: input path, or `-` for stdin
* `module`: conversion mode
* `newname`: name for the output header, or an empty string to keep the input name
* `flags`: OPTION_* flags
* `volume`: volume state of the conversion

Returns: 0, or -1 for an error

Calls: [created_file_path](megatron.c.md#created_file_path), [from_close](megatron.c.md#from_close), [from_open](megatron.c.md#from_open), [from_read](megatron.c.md#from_read), [mode_reads_metadata](megatron.c.md#mode_reads_metadata), [mode_writes_metadata](megatron.c.md#mode_writes_metadata), [nad_set_volume](nad_adouble.c.md#nad_set_volume), [print_header](megatron.c.md#print_header), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [to_close](megatron.c.md#to_close), [to_open](megatron.c.md#to_open), [to_write](megatron.c.md#to_write), [update_created_file_cnid](megatron.c.md#update_created_file_cnid)

Called by: [nad_archive](megatron.c.md#nad_archive)

Uses file-scope variables: `forkbuf`

### validate_volume_metadata

```c
static int validate_volume_metadata(const char *volpath, const struct vol *vol)
```

Defined at lines 594 to 603.

Refuse a volume whose AppleDouble format nad does not support.

Parameters:
* `volpath`: path for the message
* `vol`: volume to check

Returns: 0 for adouble:v2 and adouble:ea, or -1

Called by: [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### set_fallback_volume

```c
static void set_fallback_volume(struct nad_volume *volume, int flags)
```

Defined at lines 616 to 637.

Set up the stand-in volume that force uses outside an AFP volume.

The stand-in volume is the current directory with adouble:ea metadata, or with adouble:v2 under adouble, and it has no CNIDs.

Parameters:
* `volume`: receives the stand-in volume
* `flags`: OPTION_* flags

Calls: [ad_path](../../libatalk/adouble/ad_open.c.md#ad_path), [ad_path_ea](../../libatalk/adouble/ad_open.c.md#ad_path_ea), [ad_path_osx](../../libatalk/adouble/ad_open.c.md#ad_path_osx)

Called by: [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

Uses file-scope variables: `fallback_volume`

### resolve_metadata_volume

```c
static int resolve_metadata_volume(AFPObj *obj, int module, const char *path, int flags, struct nad_volume *volume)
```

Defined at lines 659 to 755.

Find the AFP volume that holds the metadata of a conversion and open its CNID database.

Decoding and stdin input use the volume of the current directory, and encoding uses the volume of the input file. Outside a volume the call fails, unless force picks the stand-in volume.

When the conversion creates a file, the call also opens the CNID database of the current directory and reads its stamp. Under force a current directory outside a volume skips the CNIDs. A CNID backend that this build lacks fails.

Parameters:
* `obj`: AFP configuration with the volumes
* `module`: conversion mode
* `path`: input path, or `-` for stdin
* `flags`: OPTION_* flags
* `volume`: receives the volume state

Returns: 0, or -1 for an error

Calls: [close_metadata_volume](megatron.c.md#close_metadata_volume), [cnid_getstamp](../../libatalk/cnid/cnid.c.md#cnid_getstamp), [cnid_open](../../libatalk/cnid/cnid.c.md#cnid_open), [cnid_scheme_registered](../../libatalk/cnid/cnid.c.md#cnid_scheme_registered), [getvolbypath](../../libatalk/util/netatalk_conf.c.md#getvolbypath), [mode_creates_volume_file](megatron.c.md#mode_creates_volume_file), [mode_writes_metadata](megatron.c.md#mode_writes_metadata), [nad_not_inside_volume](nad_util.c.md#nad_not_inside_volume), [set_fallback_volume](megatron.c.md#set_fallback_volume), [validate_volume_metadata](megatron.c.md#validate_volume_metadata)

Called by: [nad_archive](megatron.c.md#nad_archive)

Uses file-scope variables: `forceflag` in [bin/nad/nad_util.c](nad_util.c.md)

### close_metadata_volume

```c
static void close_metadata_volume(struct nad_volume *volume)
```

Defined at lines 763 to 771.

Close the CNID database that resolve_metadata_volume opened and clear the volume state.

Parameters:
* `volume`: volume state

Calls: [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close)

Called by: [nad_archive](megatron.c.md#nad_archive), [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### arg_is_option_with_value

```c
static int arg_is_option_with_value(const char *arg)
```

Defined at lines 779 to 782.

Tell whether an archive argument is an option that takes a value.

Parameters:
* `arg`: command-line argument

Returns: nonzero for filename

Called by: [nad_archive](megatron.c.md#nad_archive)

### arg_is_flag_option

```c
static int arg_is_flag_option(const char *arg)
```

Defined at lines 790 to 796.

Tell whether an archive argument is an option without a value.

Parameters:
* `arg`: command-line argument

Returns: nonzero for header, stdout, adouble, and verbose

Called by: [nad_archive](megatron.c.md#nad_archive)

### set_newname

```c
static int set_newname(char *newname, size_t newname_len, const char *name)
```

Defined at lines 806 to 814.

Store the filename name for the next conversion.

Parameters:
* `newname`: buffer for the name
* `newname_len`: size of `newname`
* `name`: name from the command line

Returns: 0, or -1 for a name that does not fit

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [nad_archive](megatron.c.md#nad_archive)

### option_after_file

```c
static int option_after_file(const char *arg)
```

Defined at lines 822 to 827.

Report an option that follows a file argument.

Parameters:
* `arg`: option that came after a file

Returns: 1, the exit status for the error

Called by: [nad_archive](megatron.c.md#nad_archive)

### nad_archive

```c
int nad_archive(int argc, char **argv, AFPObj *obj)
```

Defined at lines 843 to 995.

Run the nad archive commands bin, hex, unbin, and unhex.

The -h and help options print the usage and return 0. The first pass checks the arguments, and every option has to come before the first file. The second pass converts each file with a fresh volume state. A filename name applies to the first file, or to stdin without a file. Without a file the decoding commands read stdin, and the encoding commands fail.

Parameters:
* `argc`: argument count, from the command name on
* `argv`: argument vector, from the command name on
* `obj`: AFP configuration with the volumes

Returns: 0, or 1 when a file fails or the arguments do not parse

Calls: [arg_is_flag_option](megatron.c.md#arg_is_flag_option), [arg_is_option_with_value](megatron.c.md#arg_is_option_with_value), [close_metadata_volume](megatron.c.md#close_metadata_volume), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [mode_reads_metadata](megatron.c.md#mode_reads_metadata), [nad_archive_convert](megatron.c.md#nad_archive_convert), [option_after_file](megatron.c.md#option_after_file), [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume), [set_newname](megatron.c.md#set_newname), [usage_archive](megatron.c.md#usage_archive)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `nad_log_verbose` in [bin/nad/nad_util.c](nad_util.c.md)

# Types

### struct archive_mode

Defined at line 41.
* `const char * name`

# File-scope variables

`archive_modes`, `fallback_volume`, `forkbuf`, `forkname`
