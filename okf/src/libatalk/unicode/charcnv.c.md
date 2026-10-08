---
type: C Source File
title: "libatalk/unicode/charcnv.c"
description: "21 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charcnv.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 69 to 84.

Return the name of a charset to give to iconv().

Called by: [add_charset](charcnv.c.md#add_charset), [convert_charset](charcnv.c.md#convert_charset), [convert_string](charcnv.c.md#convert_string), [convert_string_allocate](charcnv.c.md#convert_string_allocate), [get_charset_functions](charcnv.c.md#get_charset_functions), [init_iconv](charcnv.c.md#init_iconv)

Uses file-scope variables: `charset_names`

### set_charset_name

```c
int set_charset_name(charset_t ch, const char *name)
```

Defined at lines 86 to 94.

Called by: [afp_config_parse](../util/netatalk_conf.c.md#afp_config_parse), [main](../../bin/getzones/getzones.c.md#main), [main](../../bin/misc/netacnv.c.md#main), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [main](../../etc/atalkd/main.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `charset_names`

### free_charset_names

```c
void free_charset_names(void)
```

Defined at lines 96 to 104.

Called by: [afp_config_free](../util/netatalk_conf.c.md#afp_config_free)

Uses file-scope variables: `charset_names`

### get_charset_functions

```c
static struct charset_functions * get_charset_functions(charset_t ch)
```

Defined at lines 106 to 114.

Calls: [charset_name](charcnv.c.md#charset_name), [find_charset_functions](iconv.c.md#find_charset_functions)

Called by: [add_charset](charcnv.c.md#add_charset), [init_iconv](charcnv.c.md#init_iconv)

Uses file-scope variables: `charsets`

### lazy_initialize_conv

```c
static void lazy_initialize_conv(void)
```

Defined at lines 117 to 125.

Calls: [init_iconv](charcnv.c.md#init_iconv)

Called by: [add_charset](charcnv.c.md#add_charset), [convert_charset](charcnv.c.md#convert_charset), [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal)

### add_charset

```c
charset_t add_charset(const char *name)
```

Defined at lines 127 to 177.

Calls: [atalk_iconv_open](iconv.c.md#atalk_iconv_open), [charset_name](charcnv.c.md#charset_name), [get_charset_functions](charcnv.c.md#get_charset_functions), [lazy_initialize_conv](charcnv.c.md#lazy_initialize_conv)

Called by: [convert_to_mac_name](../../etc/papd/print_cups.c.md#convert_to_mac_name), [cups_autoadd_printers](../../etc/papd/print_cups.c.md#cups_autoadd_printers), [load_charset](../util/netatalk_conf.c.md#load_charset), [lp_print](../../etc/papd/lp.c.md#lp_print), [main](../../bin/getzones/getzones.c.md#main), [main](../../bin/misc/netacnv.c.md#main), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [volume_codepage](../../etc/afpd/volume.c.md#volume_codepage)

Uses file-scope variables: `charset_names`, `charsets`, `conv_handles`

### init_iconv

```c
void init_iconv(void)
```

Defined at lines 186 to 214.

Initialize iconv conversion descriptors.

This is called the first time it is needed, and also called again every time the configuration is reloaded, because the charset or codepage might have changed.

Calls: [atalk_iconv_open](iconv.c.md#atalk_iconv_open), [charset_name](charcnv.c.md#charset_name), [get_charset_functions](charcnv.c.md#get_charset_functions)

Called by: [lazy_initialize_conv](charcnv.c.md#lazy_initialize_conv)

Uses file-scope variables: `charsets`, `conv_handles`

### add_null

```c
static size_t add_null(charset_t to, char *buf, size_t bytesleft, size_t len)
```

Defined at lines 216 to 230.

Called by: [convert_string_internal](charcnv.c.md#convert_string_internal)

### convert_string_internal

```c
static size_t convert_string_internal(charset_t from, charset_t to, void const *src, size_t srclen, void *dest, size_t destlen)
```

Defined at lines 245 to 299.

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

Defined at lines 302 to 345.

Calls: [charset_name](charcnv.c.md#charset_name), [convert_string_internal](charcnv.c.md#convert_string_internal), [decompose_w](util_unistr.c.md#decompose_w), [precompose_w](util_unistr.c.md#precompose_w)

Called by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [afp_getsrvrmesg](../../etc/afpd/messages.c.md#afp_getsrvrmesg), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [catsearch_afp](../../etc/afpd/catsearch.c.md#catsearch_afp), [configinit](../../etc/afpd/afp_config.c.md#configinit), [creatvol](../util/netatalk_conf.c.md#creatvol), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [list_eas](../vfs/ea_ad.c.md#list_eas), [mtoUTF8](../../etc/afpd/file.c.md#mtoutf8), [register_stuff](../../etc/netatalk/afp_avahi.c.md#register_stuff), [register_stuff](../../etc/netatalk/afp_mdns.c.md#register_stuff), [status_server](../../etc/afpd/status.c.md#status_server), [status_utf8servername](../../etc/afpd/status.c.md#status_utf8servername), [sys_list_eas](../vfs/ea_sys.c.md#sys_list_eas), [uam_getname](../../etc/afpd/uam.c.md#uam_getname), [ucs2_to_charset](charcnv.c.md#ucs2_to_charset)

Uses file-scope variables: `charsets`

### convert_string_allocate_internal

```c
static size_t convert_string_allocate_internal(charset_t from, charset_t to, void const *src, size_t srclen, char **dest)
```

Defined at lines 363 to 455.

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

Defined at lines 458 to 501.

Calls: [charset_name](charcnv.c.md#charset_name), [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [decompose_w](util_unistr.c.md#decompose_w), [precompose_w](util_unistr.c.md#precompose_w)

Called by: [afp_mapid](../../etc/afpd/directory.c.md#afp_mapid), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [configinit](../../etc/afpd/afp_config.c.md#configinit), [convert_to_mac_name](../../etc/papd/print_cups.c.md#convert_to_mac_name), [cups_autoadd_printers](../../etc/papd/print_cups.c.md#cups_autoadd_printers), [dir_modify](../../etc/afpd/directory.c.md#dir_modify), [dir_new](../../etc/afpd/directory.c.md#dir_new), [logincont2](../../etc/uams/uams_dhx2_pam.c.md#logincont2), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [main](../../etc/papd/main.c.md#main), [nbplkup_convert_field](../../bin/nbp/nbplkup_output.c.md#nbplkup_convert_field), [print_and_count_zones_in_reply](../../bin/getzones/getzones.c.md#print_and_count_zones_in_reply), [print_gnireply](../../bin/getzones/getzones.c.md#print_gnireply), [print_zones](../../bin/getzones/getzones.c.md#print_zones), [sl_unpack_cpx](../../etc/afpd/spotlight_marshalling.c.md#sl_unpack_cpx), [translate](../../etc/papd/lp.c.md#translate), [ucs2_to_charset_allocate](charcnv.c.md#ucs2_to_charset_allocate), [writeconf](../../etc/atalkd/config.c.md#writeconf), [zone](../../etc/atalkd/config.c.md#zone)

Uses file-scope variables: `charsets`

### charset_strupper

```c
size_t charset_strupper(charset_t ch, const char *src, size_t srclen, char *dest, size_t destlen)
```

Defined at lines 503 to 524.

Calls: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [strupper_w](util_unistr.c.md#strupper_w)

### charset_strlower

```c
size_t charset_strlower(charset_t ch, const char *src, size_t srclen, char *dest, size_t destlen)
```

Defined at lines 526 to 547.

Calls: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [strlower_w](util_unistr.c.md#strlower_w)

### ucs2_to_charset

```c
size_t ucs2_to_charset(charset_t ch, const ucs2_t *src, char *dest, size_t destlen)
```

Defined at lines 560 to 565.

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

Defined at lines 568 to 573.

Calls: [convert_string_allocate](charcnv.c.md#convert_string_allocate), [strlen_w](util_unistr.c.md#strlen_w)

Called by: [afp_getsrvrparms](../../etc/afpd/volume.c.md#afp_getsrvrparms)

### charset_precompose

```c
size_t charset_precompose(charset_t ch, char *src, size_t inlen, char *dst, size_t outlen)
```

Defined at lines 575 to 603.

Calls: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [precompose_w](util_unistr.c.md#precompose_w)

### charset_decompose

```c
size_t charset_decompose(charset_t ch, char *src, size_t inlen, char *dst, size_t outlen)
```

Defined at lines 605 to 633.

Calls: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [decompose_w](util_unistr.c.md#decompose_w)

Called by: [add_filemeta](../../etc/afpd/spotlight.c.md#add_filemeta)

### pull_charset_flags

```c
static size_t pull_charset_flags(charset_t from_set, charset_t to_set, charset_t cap_set, const char *src, size_t srclen, char *dest, size_t destlen, uint16_t *flags)
```

Defined at lines 649 to 874.

Convert from MB to UCS2 charset.

Flags:

* CONV_UNESCAPEHEX: ':XX' will be converted to an UCS2 character
* CONV_IGNORE: return the first convertable characters.
* CONV_FORCE: force convertion

Bug: This will *not* work if the destination charset is not multibyte, i.e. UCS2->UCS2 will fail The (un)escape scheme is not compatible to the old cap style escape. This is bad, we need it for e.g. HFS cdroms.

Calls: [atalk_iconv](iconv.c.md#atalk_iconv)

Called by: [convert_charset](charcnv.c.md#convert_charset)

Uses file-scope variables: `conv_handles`

### push_charset_flags

```c
static size_t push_charset_flags(charset_t to_set, charset_t cap_set, char *src, size_t srclen, char *dest, size_t destlen, uint16_t *flags)
```

Defined at lines 893 to 982.

Convert from UCS2 to MB charset.

Flags:

* CONV_ESCAPEDOTS: escape leading dots
* CONV_ESCAPEHEX: unconvertable characters and '/' will be escaped to :XX
* CONV_IGNORE: return the first convertable characters.
* CONV__EILSEQ: unconvertable characters will be replaced with '_'
* CONV_FORCE: force convertion Bug: CONV_IGNORE and CONV_ESCAPEHEX can't work together. Should we check this ? This will *not* work if the destination charset is not multibyte, i.e. UCS2->UCS2 will fail The escape scheme is not compatible to the old cap style escape. This is bad, we need it for e.g. HFS cdroms.

Calls: [atalk_iconv](iconv.c.md#atalk_iconv)

Called by: [convert_charset](charcnv.c.md#convert_charset)

Uses file-scope variables: `conv_handles`, `hexdig`

### convert_charset

```c
size_t convert_charset(charset_t from_set, charset_t to_set, charset_t cap_charset, const char *src, size_t src_len, char *dest, size_t dest_len, uint16_t *flags)
```

Defined at lines 988 to 1071.

Bug: the size is a mess we really need a malloc/free logic

Note: dest_len must include space for the two-byte terminator

Calls: [charset_name](charcnv.c.md#charset_name), [decompose_w](util_unistr.c.md#decompose_w), [lazy_initialize_conv](charcnv.c.md#lazy_initialize_conv), [precompose_w](util_unistr.c.md#precompose_w), [pull_charset_flags](charcnv.c.md#pull_charset_flags), [push_charset_flags](charcnv.c.md#push_charset_flags), [strlower_w](util_unistr.c.md#strlower_w), [strupper_w](util_unistr.c.md#strupper_w)

Called by: [catsearch_afp](../../etc/afpd/catsearch.c.md#catsearch_afp), [catsearch_db](../../etc/afpd/catsearch.c.md#catsearch_db), [convert_dots_encoding](../../bin/nad/nad_util.c.md#convert_dots_encoding), [convert_utf8_to_mac](../util/pathconv.c.md#convert_utf8_to_mac), [creatvol](../util/netatalk_conf.c.md#creatvol), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [demangle_checks](../../etc/afpd/mangle.c.md#demangle_checks), [main](../../bin/misc/netacnv.c.md#main), [mangle](../../etc/afpd/mangle.c.md#mangle), [mangle_extension](../../etc/afpd/mangle.c.md#mangle_extension), [mtoupath](../../etc/afpd/desktop.c.md#mtoupath), [mtoupath](../vfs/ea_ad.c.md#mtoupath), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery), [utompath](../../etc/afpd/desktop.c.md#utompath)

Uses file-scope variables: `charsets`

# Macros

* Undocumented: `CHECK_FLAGS`, `MAX_CHARSETS`, `MAX_CONVERT_SIZE`, `hextoint`

# File-scope variables

`charset_names`, `charsets`, `conv_handles`, `hexdig`
