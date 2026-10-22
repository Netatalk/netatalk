---
type: C Source File
title: "etc/afpd/catsearch.c"
description: "FPCatSearch implementation."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/catsearch.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-22T19:27:15+11:00 }
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

Defined at lines 140 to 148.

Add to a specification length without wrapping around.

Parameters:
* `len`: length so far
* `add`: bytes to add

Returns: 0, or AFPERR_PARAM when the sum would overflow size_t

Called by: [catsearch_spec_layout](catsearch.c.md#catsearch_spec_layout)

### catsearch_spec_layout

```c
static int catsearch_spec_layout(uint16_t fbitmap, uint16_t dbitmap, uint32_t rbitmap, struct catsearch_spec_layout *layout)
```

Defined at lines 167 to 255.

Compute the fixed part of an FPCatSearch specification from the request bitmap.

Each criterion bit of rbitmap adds the size of its field. The long name and the UTF-8 name add their offset fields and record where those sit. The offspring count bit adds a field only for a search of directories alone. For files alone it means the resource fork length, which adds nothing. With both bitmaps it is an error.

Parameters:
* `fbitmap`: file bitmap of the request
* `dbitmap`: directory bitmap of the request
* `rbitmap`: request bitmap of the criteria
* `layout`: fixed length and name field offsets

Returns: 0, AFPERR_BITMAP for an offspring count with both bitmaps, or AFPERR_PARAM on overflow

Calls: [catsearch_spec_addlen](catsearch.c.md#catsearch_spec_addlen)

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp)

### catsearch_validate_spec

```c
static int catsearch_validate_spec(const unsigned char *spec, size_t spec_len, const struct catsearch_spec_layout *layout)
```

Defined at lines 271 to 319.

Check that a specification record holds its fixed part and the names it points to.

The long name offset must point at a length byte whose name fits in the record. The UTF-8 name offset must leave room for the 4-byte encoding hint, the 2-byte length, and the name. A UTF-8 name length above UTF8FILELEN_EARLY counts as UTF8FILELEN_EARLY.

Parameters:
* `spec`: specification record, past its length field
* `spec_len`: length of the record
* `layout`: layout from [catsearch_spec_layout](catsearch.c.md#struct-catsearch_spec_layout)

Returns: 0, or AFPERR_PARAM when a field lies outside the record

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp)

### clearstack

```c
static void clearstack(void)
```

Defined at lines 322 to 329.

Clears directory stack.

Called by: [addstack](catsearch.c.md#addstack), [catsearch](catsearch.c.md#catsearch)

Uses file-scope variables: `dsidx`, `save_cidx`

### addstack

```c
static int addstack(char *uname, struct dir *dir, int pidx)
```

Defined at lines 342 to 366.

Push a directory onto the search stack.

The stack grows in steps of DS_BSIZE entries. A failed growth clears the stack and frees its memory.

Parameters:
* `uname`: unused
* `dir`: directory to search later
* `pidx`: unused

Returns: 0, or -1 when realloc fails

Calls: [clearstack](catsearch.c.md#clearstack)

Called by: [catsearch](catsearch.c.md#catsearch)

Uses file-scope variables: `dsidx`, `dssize`, `dstack`

### reducestack

```c
static int reducestack(void)
```

Defined at lines 372 to 391.

Removes checked items from top of directory stack.

Returns: index of the first unchecked elements or -1.

Called by: [catsearch](catsearch.c.md#catsearch)

Uses file-scope variables: `dsidx`, `dstack`, `save_cidx`

### adl_lkup

```c
static struct adouble * adl_lkup(struct vol *vol, struct path *path, struct adouble *adp)
```

Defined at lines 407 to 448.

Get the AppleDouble metadata of a search candidate once.

A non-NULL adp means an earlier criterion loaded the metadata already. An open fork of the file supplies its adouble, which also fills a dircache entry without metadata. Any other candidate reads through ad_metadata_cached into a static adouble.

Parameters:
* `vol`: volume of the search
* `path`: candidate with its stat result, its d_cached memo set
* `adp`: metadata from an earlier call, or NULL

Returns: adp, the adouble of an open fork, the static adouble, or NULL without metadata

Calls: [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [of_findname](ofork.c.md#of_findname), [path_resolve_cached_file](file.c.md#path_resolve_cached_file)

Called by: [crit_check](catsearch.c.md#crit_check), [crit_mac_name](catsearch.c.md#crit_mac_name), [unpack_finderinfo](catsearch.c.md#unpack_finderinfo)

### unpack_buffer

```c
static struct finderinfo * unpack_buffer(struct finderinfo *finfo, char *buffer)
```

Defined at lines 462 to 473.

Split packed Finder info into the type, creator, flags, and label that FPCatSearch compares.

The Finder flags word lands in both attrs and label, attrs masked with 0xff00 and label with 0xff. The search criteria pass through the same split. All fields keep their byte order.

Parameters:
* `finfo`: unpacked fields
* `buffer`: 32-byte Finder info

Returns: finfo

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp), [unpack_finderinfo](catsearch.c.md#unpack_finderinfo)

### unpack_finderinfo

```c
static struct finderinfo * unpack_finderinfo(struct vol *vol, struct path *path, struct adouble **adp, struct finderinfo *finfo, int islnk)
```

Defined at lines 487 to 495.

Unpack the Finder info of a search candidate, loading its metadata on first use.

Parameters:
* `vol`: volume of the search
* `path`: candidate, its d_cached memo set by adl_lkup
* `adp`: metadata of the candidate, loaded through adl_lkup
* `finfo`: unpacked fields
* `islnk`: nonzero for a symlink, passed to get_finderinfo

Returns: finfo

Calls: [adl_lkup](catsearch.c.md#adl_lkup), [get_finderinfo](file.c.md#get_finderinfo), [unpack_buffer](catsearch.c.md#unpack_buffer)

Called by: [crit_check](catsearch.c.md#crit_check)

### crit_mac_name

```c
static int crit_mac_name(struct vol *vol, struct path *path, struct adouble **adp)
```

Defined at lines 512 to 553.

Set a search candidate's Mac name and the CNID its name needs.

The file's dircache entry, memoed in the path for [getfilparams()](file.c.md#getfilparams), holds the CNID while the candidate's stat shows the entry's inode; any other file gets it from the CNID database.

Parameters:
* `vol`: volume of the search
* `path`: candidate, receives its Mac name, and its CNID when the name needs one
* `adp`: metadata of the candidate, loaded through adl_lkup for a CNID database lookup

Returns: 1, 0 to skip the file, or -1 when a CNID database reset makes the search fail

Calls: [adl_lkup](catsearch.c.md#adl_lkup), [get_id](file.c.md#get_id), [path_resolve_cached_file](file.c.md#path_resolve_cached_file), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [utompath](desktop.c.md#utompath)

Called by: [crit_check](catsearch.c.md#crit_check)

### crit_check

```c
static int crit_check(struct vol *vol, struct path *path)
```

Defined at lines 576 to 749.

Test a search candidate against the FPCatSearch criteria.

A directory needs a directory bitmap and a file a file bitmap. Cheap tests run first, the modification date and then the names; the creation and backup dates, the attributes and the Finder info, which need the AppleDouble metadata, follow. A file's Mac name, which may cost a CNID lookup, is made only for a name test and for a match.

Each range test uses c1 as the lower bound and c2 as the upper one. Each mask test applies the bits of c2 and expects the value in c1.

Parameters:
* `vol`: volume of the search
* `path`: candidate; a match, and a file a name test reads, receive the Mac name, and the CNID when the name needs one

Returns: 1 for a match, 0 otherwise, or -1 when a CNID database reset makes the search fail

Calls: [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [adl_lkup](catsearch.c.md#adl_lkup), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [crit_mac_name](catsearch.c.md#crit_mac_name), [strcasecmp_w](../../libatalk/unicode/util_unistr.c.md#strcasecmp_w), [strcasestr_w](../../libatalk/unicode/util_unistr.c.md#strcasestr_w), [unpack_finderinfo](catsearch.c.md#unpack_finderinfo)

Called by: [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db)

Uses file-scope variables: `c2`

### rslt_add

```c
static int rslt_add(const AFPObj *obj, struct vol *vol, struct path *path, char **buf, int ext)
```

Defined at lines 766 to 815.

Append one FPCatSearch result record to the reply.

A record holds its length, the file or directory flag, a filler byte for FPCatSearchExt, and the parameters of the request bitmap. FPCatSearchExt uses a 2-byte length and FPCatSearch a 1-byte one. An odd parameter length counts 1 more byte to stay even.

Parameters:
* `obj`: session object
* `vol`: volume of the search
* `path`: matched file or directory
* `buf`: reply cursor, advanced past the record
* `ext`: nonzero for FPCatSearchExt

Returns: 1 for an added record, or 0 when getfilparams or getdirparams fails

Calls: [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams)

Called by: [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db)

### catsearch

```c
static int catsearch(const AFPObj *obj, struct vol *vol, struct dir *dir, int rmatches, uint32_t *pos, char *rbuf, uint32_t *nrecs, int *rsize, int ext)
```

Defined at lines 847 to 1087.

Search the directory tree of a volume for FPCatSearch, resuming at a saved position.

The criteria come from the globals c1 and c2. A stack of directory IDs replaces recursion, and the open directory and its position survive between calls. A position of 0 starts a new search from dir. Any other position must equal the one that the last call returned.

A call pauses after rmatches matches or 448 bytes of results. Every 200 entries it also pauses once the clock moved past its start second, because Mac OS 9 clients time out on slow commands. Unreadable directories drop out of the search.

Parameters:
* `obj`: session object
* `vol`: volume to search
* `dir`: directory to start from
* `rmatches`: maximum number of matches to return
* `pos`: catalog position, 0 for a new search
* `rbuf`: result records
* `nrecs`: match count, raised for each record added
* `rsize`: length of the result records
* `ext`: nonzero for FPCatSearchExt

Returns: AFP_OK after a pause, AFPERR_EOF at the end of the tree, AFPERR_CATCHNG for a stale position, AFPERR_NFILE, or AFPERR_MISC

Calls: [addstack](catsearch.c.md#addstack), [check_dirent](enumerate.c.md#check_dirent), [clearstack](catsearch.c.md#clearstack), [crit_check](catsearch.c.md#crit_check), [dir_add](directory.c.md#dir_add), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [movecwd](directory.c.md#movecwd), [of_stat](ofork.c.md#of_stat), [reducestack](catsearch.c.md#reducestack), [rslt_add](catsearch.c.md#rslt_add), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp)

Uses file-scope variables: `dstack`, `save_cidx`

### catsearch_db

```c
static int catsearch_db(const AFPObj *obj, struct vol *vol, const char *uname, int rmatches, uint32_t *pos, char *rbuf, uint32_t *nrecs, int *rsize, int ext)
```

Defined at lines 1110 to 1272.

Search the CNID database by name for an FPCatSearch request.

A new search looks up the CNIDs of the lowercased uname through cnid_find. Each call then resolves, stats, and tests the next CNIDs against the criteria in c1 and c2. The search always covers all of the volume. A call pauses after rmatches matches or 448 bytes of results. A CNID that no longer resolves drops out, but a missing file fails the search.

Parameters:
* `obj`: session object
* `vol`: volume to search
* `uname`: name on disk to look up
* `rmatches`: maximum number of matches to return
* `pos`: catalog position, 0 for a new search
* `rbuf`: result records
* `nrecs`: match count, raised for each record added
* `rsize`: length of the result records
* `ext`: nonzero for FPCatSearchExt

Returns: AFP_OK after a pause, AFPERR_EOF after the last CNID, AFPERR_CATCHNG for a stale position, or AFPERR_MISC

Calls: [cnid_find](../../libatalk/cnid/cnid.c.md#cnid_find), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [crit_check](catsearch.c.md#crit_check), [dirlookup](directory.c.md#dirlookup), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [movecwd](directory.c.md#movecwd), [of_stat](ofork.c.md#of_stat), [rslt_add](catsearch.c.md#rslt_add), [utompath](desktop.c.md#utompath)

Called by: [catsearch_afp](catsearch.c.md#catsearch_afp)

### catsearch_afp

```c
static int catsearch_afp(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int ext)
```

Defined at lines 1293 to 1576.

Parse an FPCatSearch or FPCatSearchExt request and run the search.

The request carries the volume, the match limit, the catalog position, the file and directory bitmaps, the request bitmap, and 2 specification records. The first record holds the values and range minimums, the second the range maximums and masks. FPCatSearch returns only the name and the parent ID. A full UTF-8 name search on a volume with AFPVOL_SEARCHDB uses the CNID database, and every other search walks the tree. The reply starts with the new catalog position, the bitmaps, and the match count.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return
* `ext`: nonzero for FPCatSearchExt

Returns: the search result, AFPERR_BITMAP, or AFPERR_PARAM

Calls: [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db), [catsearch_spec_layout](catsearch.c.md#catsearch_spec_layout), [catsearch_validate_spec](catsearch.c.md#catsearch_validate_spec), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [mtoupath](desktop.c.md#mtoupath), [unpack_buffer](catsearch.c.md#unpack_buffer)

Called by: [afp_catsearch](catsearch.c.md#afp_catsearch), [afp_catsearch_ext](catsearch.c.md#afp_catsearch_ext)

Uses file-scope variables: `c2`

### afp_catsearch

```c
int afp_catsearch(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1589 to 1593.

FPCatSearch: search a volume for files and directories that match the criteria.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of catsearch_afp

Calls: [catsearch_afp](catsearch.c.md#catsearch_afp)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_catsearch_ext

```c
int afp_catsearch_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1606 to 1610.

FPCatSearchExt: search a volume for matching files and directories, with the full result bitmaps.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of catsearch_afp

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

Directory tree search is recursive by its nature.

Defined at line 112.
* `cnid_t ds_did`: CNID of this directory.
* `uint32_t ds_checked`: Have we checked this directory ?

### struct finderinfo

Defined at line 61.
* `uint32_t f_type`
* `uint32_t creator`
* `uint16_t attrs`: File attributes (high 8 bits).
* `uint16_t label`: Label (low 8 bits ).
* `char reserved`: Unknown (at least for now...).

### struct scrit

This is our search-criteria structure.

Defined at line 88.
* `uint32_t rbitmap`: Request bitmap - which values should we check ?
* `uint16_t fbitmap`: file bitmap - which values should we return ?
* `uint16_t dbitmap`: directory bitmap - which values should we return ?
* `uint16_t attr`: File attributes.
* `time_t cdate`: Creation date.
* `time_t mdate`: Last modification date.
* `time_t bdate`: Last backup date.
* `uint32_t pdid`: Parent DID.
* `uint16_t offcnt`: Offspring count.
* `struct finderinfo finfo`: Finder info.
* `char lname`: Long name.
* `char utf8name`: UTF8 or UCS2 name for convert_charset dest_len parameter +2.

# Typedefs and enums

* `typedef char packed_finder`

# Macros

* Undocumented: `CATPBIT_PARTIAL`, `DS_BSIZE`, `NUM_ROUNDS`, `VETO_STR`

# File-scope variables

`c2`, `dsidx`, `dssize`, `dstack`, `save_cidx`
