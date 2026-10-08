---
type: C Source File
title: "etc/afpd/catsearch.c"
description: "FPCatSearch implementation."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/catsearch.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [filedir.h](filedir.h.md)
* [fork.h](fork.h.md)
* [volume.h](volume.h.md)
* System headers: `ctype.h`, `errno.h`, `netinet/in.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/stat.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### catsearch_spec_addlen

```c
static int catsearch_spec_addlen(size_t *len, size_t add)
```

Defined at lines 133 to 141.

Called by: [catsearch_spec_layout](catsearch.c.md#catsearch_spec_layout)

### catsearch_spec_layout

```c
static int catsearch_spec_layout(uint16_t fbitmap, uint16_t dbitmap, uint32_t rbitmap, struct catsearch_spec_layout *layout)
```

Defined at lines 143 to 231.

Calls: [catsearch_spec_addlen](catsearch.c.md#catsearch_spec_addlen)

### catsearch_validate_spec

```c
static int catsearch_validate_spec(const unsigned char *spec, size_t spec_len, const struct catsearch_spec_layout *layout)
```

Defined at lines 233 to 281.

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp)

### clearstack

```c
static void clearstack(void)
```

Defined at lines 284 to 291.

Clears directory stack.

Called by: [addstack](catsearch.c.md#addstack), [catsearch](catsearch.c.md#catsearch)

Uses file-scope variables: `dsidx`, `save_cidx`

### addstack

```c
static int addstack(char *uname, struct dir *dir, int pidx)
```

Defined at lines 294 to 318.

Puts new item onto directory stack.

Calls: [clearstack](catsearch.c.md#clearstack)

Called by: [catsearch](catsearch.c.md#catsearch)

Uses file-scope variables: `dsidx`, `dssize`, `dstack`

### reducestack

```c
static int reducestack(void)
```

Defined at lines 324 to 346.

Removes checked items from top of directory stack.

Returns: index of the first unchecked elements or -1.

Called by: [catsearch](catsearch.c.md#catsearch)

Uses file-scope variables: `dsidx`, `dstack`, `save_cidx`

### adl_lkup

```c
static struct adouble * adl_lkup(struct vol *vol, struct path *path, struct adouble *adp)
```

Defined at lines 352 to 395.

Looks up for an opened adouble structure, opens resource fork of selected file.

Calls: [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [of_findname](ofork.c.md#of_findname), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [crit_check](catsearch.c.md#crit_check), [unpack_finderinfo](catsearch.c.md#unpack_finderinfo)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### unpack_buffer

```c
static struct finderinfo * unpack_buffer(struct finderinfo *finfo, char *buffer)
```

Defined at lines 398 to 409.

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp), [unpack_finderinfo](catsearch.c.md#unpack_finderinfo)

### unpack_finderinfo

```c
static struct finderinfo * unpack_finderinfo(struct vol *vol, struct path *path, struct adouble **adp, struct finderinfo *finfo, int islnk)
```

Defined at lines 413 to 421.

Calls: [adl_lkup](catsearch.c.md#adl_lkup), [get_finderinfo](file.c.md#get_finderinfo), [unpack_buffer](catsearch.c.md#unpack_buffer)

Called by: [crit_check](catsearch.c.md#crit_check)

### crit_check

```c
static int crit_check(struct vol *vol, struct path *path)
```

Defined at lines 434 to 648.

Criteria checker.

Returns: a 2-bit value. * bit 0 means if checked file meets given criteria.
* bit 1 means if it is a directory and we should descent into it. * uname - UNIX name
* fname - our fname (translated to UNIX)
* cidx - index in directory stack

Calls: [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [adl_lkup](catsearch.c.md#adl_lkup), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [get_id](file.c.md#get_id), [strcasecmp_w](../../libatalk/unicode/util_unistr.c.md#strcasecmp_w), [strcasestr_w](../../libatalk/unicode/util_unistr.c.md#strcasestr_w), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [unpack_finderinfo](catsearch.c.md#unpack_finderinfo), [utompath](desktop.c.md#utompath)

Called by: [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db)

Uses file-scope variables: `c2`

### rslt_add

```c
static int rslt_add(const AFPObj *obj, struct vol *vol, struct path *path, char **buf, int ext)
```

Defined at lines 651 to 700.

Calls: [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams)

Called by: [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db)

### catsearch

```c
static int catsearch(const AFPObj *obj, struct vol *vol, struct dir *dir, int rmatches, uint32_t *pos, char *rbuf, uint32_t *nrecs, int *rsize, int ext)
```

Defined at lines 721 to 961.

This function performs a filesystem search.

Uses globals c1, c2, the search criteria

Parameters:
* `obj`: AFP connection object
* `vol`: volume we are searching on ...
* `dir`: directory we are starting from ...
* `rmatches`: maximum number of matches we can return
* `pos`: position we've stopped recently
* `rbuf`: output buffer
* `nrecs`: number of matches
* `rsize`: length of data written to output buffer
* `ext`: extended search flag

Calls: [addstack](catsearch.c.md#addstack), [check_dirent](enumerate.c.md#check_dirent), [clearstack](catsearch.c.md#clearstack), [crit_check](catsearch.c.md#crit_check), [dir_add](directory.c.md#dir_add), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [movecwd](directory.c.md#movecwd), [of_stat](ofork.c.md#of_stat), [reducestack](catsearch.c.md#reducestack), [rslt_add](catsearch.c.md#rslt_add), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp)

Uses file-scope variables: `dstack`, `save_cidx`

### catsearch_db

```c
static int catsearch_db(const AFPObj *obj, struct vol *vol, const char *uname, int rmatches, uint32_t *pos, char *rbuf, uint32_t *nrecs, int *rsize, int ext)
```

Defined at lines 979 to 1141.

This function performs a CNID db search.

Uses globals c1, c2, the search criteria Always searches the whole volume, not a subtree

Parameters:
* `obj`: AFP connection object
* `vol`: volume we are searching on ...
* `uname`: UNIX name of object to search
* `rmatches`: maximum number of matches we can return
* `pos`: position we've stopped recently
* `rbuf`: output buffer
* `nrecs`: number of matches
* `rsize`: length of data written to output buffer
* `ext`: extended search flag

Calls: [cnid_find](../../libatalk/cnid/cnid.c.md#cnid_find), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [crit_check](catsearch.c.md#crit_check), [dirlookup](directory.c.md#dirlookup), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [movecwd](directory.c.md#movecwd), [of_stat](ofork.c.md#of_stat), [rslt_add](catsearch.c.md#rslt_add), [utompath](desktop.c.md#utompath)

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp)

### catsearch_afp

```c
static int catsearch_afp(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int ext)
```

Defined at lines 1144 to 1433.

Calls: [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db), [catsearch_validate_spec](catsearch.c.md#catsearch_validate_spec), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [mtoupath](desktop.c.md#mtoupath), [unpack_buffer](catsearch.c.md#unpack_buffer)

Called by: [afp_catsearch](catsearch.c.md#afp_catsearch), [afp_catsearch_ext](catsearch.c.md#afp_catsearch_ext)

Uses file-scope variables: `c2`

### afp_catsearch

```c
int afp_catsearch(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1436 to 1440.

Calls: [catsearch_afp](catsearch.c.md#catsearch_afp)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_catsearch_ext

```c
int afp_catsearch_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1443 to 1447.

Calls: [catsearch_afp](catsearch.c.md#catsearch_afp)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

# Types

### struct catsearch_spec_layout

Defined at line 125.
* `size_t fixed_len`
* `size_t lname_offset`
* `size_t pdinfo_offset`
* `int has_lname`
* `int has_pdinfo`

### struct dsitem

Defined at line 112.
* `cnid_t ds_did`
* `uint32_t ds_checked`

### struct finderinfo

Defined at line 61.
* `uint32_t f_type`
* `uint32_t creator`
* `uint16_t attrs`
* `uint16_t label`
* `char reserved`

### struct scrit

Defined at line 88.
* `uint32_t rbitmap`
* `uint16_t fbitmap`
* `uint16_t dbitmap`
* `uint16_t attr`
* `time_t cdate`
* `time_t mdate`
* `time_t bdate`
* `uint32_t pdid`
* `uint16_t offcnt`
* `struct finderinfo finfo`
* `char lname`
* `char utf8name`

# Typedefs and enums

* `typedef char packed_finder`

# Macros

* Undocumented: `CATPBIT_PARTIAL`, `DS_BSIZE`, `NUM_ROUNDS`, `VETO_STR`

# File-scope variables

`c2`, `dsidx`, `dssize`, `dstack`, `save_cidx`
