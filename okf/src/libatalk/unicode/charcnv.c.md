---
type: C Source File
title: "libatalk/unicode/charcnv.c"
description: "Character-set conversion routines based on samba iconv wrappers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charcnv.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/unicode](../unicode.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/byteorder.h](../../include/atalk/byteorder.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `ctype.h`, `errno.h`, `iconv.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `unistd.h`

# Functions

### charset_name

```c
static const char * charset_name(charset_t ch)
```

Defined at lines 75 to 90.

Return the iconv name of a charset.

UCS2, UTF8, and UTF8-MAC use fixed names, and the others come from the name table.

Parameters:
* `ch`: charset

Returns: the name, or NULL for an unnamed table slot

Called by: [add_charset](charcnv.c.md#add_charset), [convert_charset](charcnv.c.md#convert_charset), [convert_string](charcnv.c.md#convert_string), [convert_string_allocate](charcnv.c.md#convert_string_allocate), [get_charset_functions](charcnv.c.md#get_charset_functions), [init_iconv](charcnv.c.md#init_iconv)

Uses file-scope variables: `charset_names`

### set_charset_name

```c
int set_charset_name(charset_t ch, const char *name)
```

Defined at lines 101 to 109.

Set the name of a fixed charset slot in the name table.

The call stores a strdup of the name and does not free an earlier one.

Parameters:
* `ch`: charset below NUM_CHARSETS
* `name`: charset name

Returns: 0, or -1 for a charset outside the fixed slots

Called by: [afp_config_parse](../util/netatalk_conf.c.md#afp_config_parse), [main](../../bin/getzones/getzones.c.md#main), [main](../../bin/misc/netacnv.c.md#main), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [main](../../etc/atalkd/main.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `charset_names`

### free_charset_names

```c
void free_charset_names(void)
```

Defined at lines 114 to 122.

Free every name in the charset name table.

Called by: [afp_config_free](../util/netatalk_conf.c.md#afp_config_free)

Uses file-scope variables: `charset_names`

### get_charset_functions

```c
static struct charset_functions * get_charset_functions(charset_t ch)
```

Defined at lines 131 to 139.

Return the built-in conversion functions of a charset, which the call looks up once by name.

Parameters:
* `ch`: charset

Returns: the functions, or NULL for a charset that only iconv knows

Calls: [charset_name](charcnv.c.md#charset_name), [find_charset_functions](iconv.c.md#find_charset_functions)

Called by: [add_charset](charcnv.c.md#add_charset), [init_iconv](charcnv.c.md#init_iconv)

Uses file-scope variables: `charsets`

### lazy_initialize_conv

```c
static void lazy_initialize_conv(void)
```

Defined at lines 145 to 153.

Run init_iconv once, on the first conversion or charset lookup.

Calls: [init_iconv](charcnv.c.md#init_iconv)

Called by: [add_charset](charcnv.c.md#add_charset), [convert_charset](charcnv.c.md#convert_charset), [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal)

### add_charset

```c
charset_t add_charset(const char *name)
```

Defined at lines 165 to 215.

Find a charset by name, or register it and open its iconv descriptors to and from UCS2.

The name match ignores case. Up to MAX_CHARSETS charsets fit, and a name that iconv cannot convert fails.

Parameters:
* `name`: charset name

Returns: the charset, or `(charset_t)-1` for an error

Calls: [atalk_iconv_open](iconv.c.md#atalk_iconv_open), [charset_name](charcnv.c.md#charset_name), [get_charset_functions](charcnv.c.md#get_charset_functions), [lazy_initialize_conv](charcnv.c.md#lazy_initialize_conv)

Called by: [convert_to_mac_name](../../etc/papd/print_cups.c.md#convert_to_mac_name), [cups_autoadd_printers](../../etc/papd/print_cups.c.md#cups_autoadd_printers), [load_charset](../util/netatalk_conf.c.md#load_charset), [lp_print](../../etc/papd/lp.c.md#lp_print), [main](../../bin/getzones/getzones.c.md#main), [main](../../bin/misc/netacnv.c.md#main), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [volume_codepage](../../etc/afpd/volume.c.md#volume_codepage)

Uses file-scope variables: `charset_names`, `charsets`, `conv_handles`

### init_iconv

```c
void init_iconv(void)
```

Defined at lines 224 to 252.

Initialize iconv conversion descriptors.

This is called the first time it is needed, and also called again every time the configuration is reloaded, because the charset or codepage might have changed.

Calls: [atalk_iconv_open](iconv.c.md#atalk_iconv_open), [charset_name](charcnv.c.md#charset_name), [get_charset_functions](charcnv.c.md#get_charset_functions)

Called by: [lazy_initialize_conv](charcnv.c.md#lazy_initialize_conv)

Uses file-scope variables: `charsets`, `conv_handles`

### add_null

```c
static size_t add_null(charset_t to, char *buf, size_t bytesleft, size_t len)
```

Defined at lines 263 to 277.

Terminate a converted string with one NUL byte, or two for UCS2.

Parameters:
* `to`: target charset
* `buf`: output buffer
* `bytesleft`: free bytes after the string
* `len`: length of the string

Returns: `len`, or `(size_t)-1` with E2BIG when the NUL does not fit

Called by: [convert_string_internal](charcnv.c.md#convert_string_internal)

### convert_string_internal

```c
static size_t convert_string_internal(charset_t from, charset_t to, void const *src, size_t srclen, void *dest, size_t destlen)
```

Defined at lines 292 to 346.

Convert string from one encoding to another, making error checking etc.

Parameters:
* `from`: source character set
* `to`: destination character set
* `src`: pointer to source string (multibyte or singlebyte)
* `srclen`: length of the source string in bytes
* `dest`: pointer to destination string (multibyte or singlebyte)
* `destlen`: maximal length allowed for string

Returns: the number of bytes occupied in the destination

Calls: [add_null](charcnv.c.md#add_null), [atalk_iconv](iconv.c.md#atalk_iconv), [lazy_initialize_conv](charcnv.c.md#lazy_initialize_conv), [strlen_w](util_unistr.c.md#strlen_w)

Called by: [charset_decompose](charcnv.c.md#charset_decompose), [charset_precompose](charcnv.c.md#charset_precompose), [charset_strlower](charcnv.c.md#charset_strlower), [charset_strupper](charcnv.c.md#charset_strupper), [convert_string](charcnv.c.md#convert_string), [convert_string_allocate](charcnv.c.md#convert_string_allocate)

Uses file-scope variables: `conv_handles`

### convert_string

```c
size_t convert_string(charset_t from, charset_t to, void const *src, size_t srclen, void *dest, size_t destlen)
```

Defined at lines 367 to 410.

Convert a string between two charsets through UCS2, with Unicode composition.

A decomposed target charset gets decomposed text. Otherwise text from a decomposed source, or from a charset without built-in functions, gets precomposed. The intermediate buffers hold MAXPATHLEN UCS2 characters, and the output ends with a NUL.

Parameters:
* `from`: source charset
* `to`: target charset
* `src`: source string
* `srclen`: source length in bytes, or `(size_t)-1` for the string length
* `dest`: output buffer
* `destlen`: size of `dest`

Returns: the output length without the NUL, or `(size_t)-1` for an error

Calls: [charset_name](charcnv.c.md#charset_name), [convert_string_internal](charcnv.c.md#convert_string_internal), [decompose_w](util_unistr.c.md#decompose_w), [precompose_w](util_unistr.c.md#precompose_w)

Called by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [afp_getsrvrmesg](../../etc/afpd/messages.c.md#afp_getsrvrmesg), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [catsearch_afp](../../etc/afpd/catsearch.c.md#catsearch_afp), [configinit](../../etc/afpd/afp_config.c.md#configinit), [creatvol](../util/netatalk_conf.c.md#creatvol), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [list_eas](../vfs/ea_ad.c.md#list_eas), [mtoUTF8](../../etc/afpd/file.c.md#mtoutf8), [register_stuff](../../etc/netatalk/afp_avahi.c.md#register_stuff), [register_stuff](../../etc/netatalk/afp_mdns.c.md#register_stuff), [status_server](../../etc/afpd/status.c.md#status_server), [status_utf8servername](../../etc/afpd/status.c.md#status_utf8servername), [sys_list_eas](../vfs/ea_sys.c.md#sys_list_eas), [uam_getname](../../etc/afpd/uam.c.md#uam_getname), [ucs2_to_charset](charcnv.c.md#ucs2_to_charset)

Uses file-scope variables: `charsets`

### convert_string_allocate_internal

```c
static size_t convert_string_allocate_internal(charset_t from, charset_t to, void const *src, size_t srclen, char **dest)
```

Defined at lines 428 to 520.

Convert between character sets, allocating a new buffer for the result.

Parameters:
* `from`: source character set
* `to`: destination character set
* `src`: pointer to source string (multibyte or singlebyte)
* `srclen`: length of source buffer
* `dest`: always set at least to NULL

Note: -1 is not accepted for srclen

Returns: Size in bytes of the converted string; or -1 in case of error

Calls: [atalk_iconv](iconv.c.md#atalk_iconv), [lazy_initialize_conv](charcnv.c.md#lazy_initialize_conv)

Called by: [charset_decompose](charcnv.c.md#charset_decompose), [charset_precompose](charcnv.c.md#charset_precompose), [charset_strlower](charcnv.c.md#charset_strlower), [charset_strupper](charcnv.c.md#charset_strupper), [convert_string_allocate](charcnv.c.md#convert_string_allocate)

Uses file-scope variables: `conv_handles`

### convert_string_allocate

```c
size_t convert_string_allocate(charset_t from, charset_t to, void const *src, size_t srclen, char **dest)
```

Defined at lines 538 to 581.

Convert a string between two charsets through UCS2 into a buffer from malloc.

The composition rules match convert_string, and the buffer ends with a NUL.

Parameters:
* `from`: source charset
* `to`: target charset
* `src`: source string
* `srclen`: source length in bytes, or `(size_t)-1` for the string length
* `dest`: receives the buffer for the caller to free, or NULL

Returns: the output length without the NUL, or `(size_t)-1` for an error

Calls: [charset_name](charcnv.c.md#charset_name), [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [decompose_w](util_unistr.c.md#decompose_w), [precompose_w](util_unistr.c.md#precompose_w)

Called by: [afp_mapid](../../etc/afpd/directory.c.md#afp_mapid), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [configinit](../../etc/afpd/afp_config.c.md#configinit), [convert_to_mac_name](../../etc/papd/print_cups.c.md#convert_to_mac_name), [cups_autoadd_printers](../../etc/papd/print_cups.c.md#cups_autoadd_printers), [dir_modify](../../etc/afpd/directory.c.md#dir_modify), [dir_new](../../etc/afpd/directory.c.md#dir_new), [logincont2](../../etc/uams/uams_dhx2_pam.c.md#logincont2), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [main](../../etc/papd/main.c.md#main), [nbplkup_convert_field](../../bin/nbp/nbplkup_output.c.md#nbplkup_convert_field), [print_and_count_zones_in_reply](../../bin/getzones/getzones.c.md#print_and_count_zones_in_reply), [print_gnireply](../../bin/getzones/getzones.c.md#print_gnireply), [print_zones](../../bin/getzones/getzones.c.md#print_zones), [sl_unpack_cpx](../../etc/afpd/spotlight_marshalling.c.md#sl_unpack_cpx), [translate](../../etc/papd/lp.c.md#translate), [ucs2_to_charset_allocate](charcnv.c.md#ucs2_to_charset_allocate), [writeconf](../../etc/atalkd/config.c.md#writeconf), [zone](../../etc/atalkd/config.c.md#zone)

Uses file-scope variables: `charsets`

### charset_strupper

```c
size_t charset_strupper(charset_t ch, const char *src, size_t srclen, char *dest, size_t destlen)
```

Defined at lines 596 to 617.

Uppercase a string through UCS2 and keep its charset.

An in-place call that changes nothing returns `srclen` and leaves the buffer as it is.

Parameters:
* `ch`: charset of the string
* `src`: source string
* `srclen`: source length in bytes
* `dest`: output buffer, which may be `src`
* `destlen`: size of `dest`

Returns: the output length, or `(size_t)-1` for an error

Calls: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [strupper_w](util_unistr.c.md#strupper_w)

### charset_strlower

```c
size_t charset_strlower(charset_t ch, const char *src, size_t srclen, char *dest, size_t destlen)
```

Defined at lines 632 to 653.

Lowercase a string through UCS2 and keep its charset.

An in-place call that changes nothing returns `srclen` and leaves the buffer as it is.

Parameters:
* `ch`: charset of the string
* `src`: source string
* `srclen`: source length in bytes
* `dest`: output buffer, which may be `src`
* `destlen`: size of `dest`

Returns: the output length, or `(size_t)-1` for an error

Calls: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [strlower_w](util_unistr.c.md#strlower_w)

### ucs2_to_charset

```c
size_t ucs2_to_charset(charset_t ch, const ucs2_t *src, char *dest, size_t destlen)
```

Defined at lines 666 to 671.

Copy a string from a UCS2 src to a unix char * destination, allocating a buffer.

Parameters:
* `ch`: destination character set
* `src`: source UCS2 string
* `dest`: always set at least to NULL
* `destlen`: maximum length of destination buffer

Returns: The number of bytes occupied by the string in the destination

Calls: [convert_string](charcnv.c.md#convert_string), [strlen_w](util_unistr.c.md#strlen_w)

Called by: [cname_mtouname](../../etc/afpd/directory.c.md#cname_mtouname), [getvolparams](../../etc/afpd/volume.c.md#getvolparams)

### ucs2_to_charset_allocate

```c
size_t ucs2_to_charset_allocate(charset_t ch, char **dest, const ucs2_t *src)
```

Defined at lines 682 to 687.

Convert a NUL-terminated UCS2 string to a charset, into a buffer from malloc.

Parameters:
* `ch`: target charset
* `dest`: receives the buffer for the caller to free, or NULL
* `src`: UCS2 string

Returns: the output length, or `(size_t)-1` for an error

Calls: [convert_string_allocate](charcnv.c.md#convert_string_allocate), [strlen_w](util_unistr.c.md#strlen_w)

Called by: [afp_getsrvrparms](../../etc/afpd/volume.c.md#afp_getsrvrparms)

### charset_precompose

```c
size_t charset_precompose(charset_t ch, char *src, size_t inlen, char *dst, size_t outlen)
```

Defined at lines 699 to 727.

Precompose a string through UCS2 and keep its charset.

Parameters:
* `ch`: charset of the string
* `src`: source string
* `inlen`: source length in bytes
* `dst`: output buffer
* `outlen`: size of `dst`

Returns: the output length, or `(size_t)-1` for an error

Calls: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [precompose_w](util_unistr.c.md#precompose_w)

### charset_decompose

```c
size_t charset_decompose(charset_t ch, char *src, size_t inlen, char *dst, size_t outlen)
```

Defined at lines 739 to 767.

Decompose a string through UCS2 and keep its charset.

Parameters:
* `ch`: charset of the string
* `src`: source string
* `inlen`: source length in bytes
* `dst`: output buffer
* `outlen`: size of `dst`

Returns: the output length, or `(size_t)-1` for an error

Calls: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [decompose_w](util_unistr.c.md#decompose_w)

Called by: [add_filemeta](../../etc/afpd/spotlight.c.md#add_filemeta)

### pull_charset_flags

```c
static size_t pull_charset_flags(charset_t from_set, charset_t to_set, charset_t cap_set, const char *src, size_t srclen, char *dest, size_t destlen, uint16_t *flags)
```

Defined at lines 799 to 1024.

Convert a multibyte string to UCS2 and map the `:` and `/` characters by the flags.

The call converts the text between `:` and `/` characters with iconv and then treats each such character by the flags. CONV_UNESCAPEHEX decodes `:xx` escapes through the CAP charset, and CONV_ESCAPEHEX writes `:3a` for `:` and `:2f` for `/`. Without them a `:` becomes `/` for a Mac target, and a `/` from a Mac source becomes `:`. CONV_ESCAPEDOTS writes a leading dot as `:2e` and sets CONV_REQESCAPE.

An unconvertible sequence stops the call with EILSEQ. CONV_IGNORE then returns the converted part and sets CONV_REQMANGLE, and CONV__EILSEQ writes IGNORE_CHAR in its place. CONV_FORCE returns the converted part after any error. The scan for `:` and `/` works on bytes, so a UCS2 source does not work.

Parameters:
* `from_set`: source charset
* `to_set`: final target charset, which picks the `:` and `/` mapping
* `cap_set`: charset of the bytes in `:xx` escapes
* `src`: source string
* `srclen`: source length, or `(size_t)-1` for the string length with its NUL
* `dest`: UCS2 output
* `destlen`: size of `dest` in bytes
* `flags`: CONV_* options, which can receive CONV_REQESCAPE and CONV_REQMANGLE

Returns: the number of bytes written, or `(size_t)-1` with errno

Calls: [atalk_iconv](iconv.c.md#atalk_iconv)

Called by: [convert_charset](charcnv.c.md#convert_charset)

Uses file-scope variables: `conv_handles`

### push_charset_flags

```c
static size_t push_charset_flags(charset_t to_set, charset_t cap_set, char *src, size_t srclen, char *dest, size_t destlen, uint16_t *flags)
```

Defined at lines 1046 to 1135.

Convert a UCS2 string to a multibyte charset and escape what does not convert, by the flags.

An unconvertible character stops the call with EILSEQ. CONV_IGNORE then returns the converted part and sets CONV_REQMANGLE. CONV_ESCAPEHEX instead writes the character in the CAP charset as `:xx` escapes and sets CONV_REQESCAPE. CONV_FORCE returns the converted part after any error, and a trailing odd byte fails with EINVAL.

Parameters:
* `to_set`: target charset
* `cap_set`: charset of the bytes in `:xx` escapes
* `src`: UCS2 string
* `srclen`: source length in bytes
* `dest`: output buffer
* `destlen`: size of `dest`
* `flags`: CONV_* options, which can receive CONV_REQESCAPE and CONV_REQMANGLE

Returns: the number of bytes written, or `(size_t)-1` with errno

Calls: [atalk_iconv](iconv.c.md#atalk_iconv)

Called by: [convert_charset](charcnv.c.md#convert_charset)

Uses file-scope variables: `conv_handles`, `hexdig`

### convert_charset

```c
size_t convert_charset(charset_t from_set, charset_t to_set, charset_t cap_charset, const char *src, size_t src_len, char *dest, size_t dest_len, uint16_t *flags)
```

Defined at lines 1160 to 1243.

Convert a string between character sets, with case, composition, and escaping chosen by flags.

convert_charset pulls the source into UCS-2 and precomposes or decomposes it as the flags and the target charset require. It then folds case for CONV_TOUPPER or CONV_TOLOWER and pushes the result into the target charset.

Bug: Buffer sizing is fragile and wants malloc/free logic.

Note: dest_len must include room for the two-byte terminator.

Parameters:
* `from_set`: charset of src
* `to_set`: charset of dest
* `cap_charset`: charset used for the :XX hex escaping of unmappable characters
* `src`: source string
* `src_len`: length of src in bytes
* `dest`: destination buffer
* `dest_len`: size of dest in bytes, terminator included
* `flags`: CONV_* options on entry, conversion results on return

Returns: the length of dest in bytes, or (size_t)-1 with errno set

Calls: [charset_name](charcnv.c.md#charset_name), [decompose_w](util_unistr.c.md#decompose_w), [lazy_initialize_conv](charcnv.c.md#lazy_initialize_conv), [precompose_w](util_unistr.c.md#precompose_w), [pull_charset_flags](charcnv.c.md#pull_charset_flags), [push_charset_flags](charcnv.c.md#push_charset_flags), [strlower_w](util_unistr.c.md#strlower_w), [strupper_w](util_unistr.c.md#strupper_w)

Called by: [catsearch_afp](../../etc/afpd/catsearch.c.md#catsearch_afp), [catsearch_db](../../etc/afpd/catsearch.c.md#catsearch_db), [convert_dots_encoding](../../bin/nad/nad_util.c.md#convert_dots_encoding), [convert_utf8_to_mac](../util/pathconv.c.md#convert_utf8_to_mac), [creatvol](../util/netatalk_conf.c.md#creatvol), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [demangle_checks](../../etc/afpd/mangle.c.md#demangle_checks), [main](../../bin/misc/netacnv.c.md#main), [mangle](../../etc/afpd/mangle.c.md#mangle), [mangle_extension](../../etc/afpd/mangle.c.md#mangle_extension), [mtoupath](../../etc/afpd/desktop.c.md#mtoupath), [mtoupath](../vfs/ea_ad.c.md#mtoupath), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery), [utompath](../../etc/afpd/desktop.c.md#utompath)

Uses file-scope variables: `charsets`

# Macros

* Undocumented: `CHECK_FLAGS`, `MAX_CHARSETS`, `MAX_CONVERT_SIZE`, `hextoint`

# File-scope variables

`charset_names`, `charsets`, `conv_handles`, `hexdig`
