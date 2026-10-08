---
type: C Source File
title: "etc/afpd/acls.c"
description: "21 functions, includes 22 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/acls.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [acl_mappings.h](acl_mappings.h.md)
* [acls.h](acls.h.md)
* [ad_cache.h](ad_cache.h.md)
* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [auth.h](auth.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [fork.h](fork.h.md)
* [unix.h](unix.h.md)
* [volume.h](volume.h.md)
* System headers: `bstrlib.h`, `errno.h`, `grp.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `strings.h`, `sys/acl.h`

# Functions

### solaris_acl_rights

```c
static int solaris_acl_rights(const AFPObj *obj, const char *path, struct stat *sb, struct maccess *ma, uint32_t *rights_out)
```

Defined at lines 102 to 234.

Compile access rights for a user to one file-system object.

This combines all access rights for a user to one fs-object and returns the result as a Darwin allowed rights ACE. This must honor trivial ACEs which are a mode_t mapping.

Parameters:
* `obj`: handle
* `path`: path to filesystem object
* `sb`: struct stat of path
* `ma`: UARights struct
* `rights_out`: mapped Darwin ACL rights

Returns: 0 or -1 on error

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [get_nfsv4_acl](../../libatalk/acl/unix.c.md#get_nfsv4_acl), [gmem](../../libatalk/util/unix.c.md#gmem)

Called by: [acltoownermode](acls.c.md#acltoownermode), [check_acl_access](acls.c.md#check_acl_access)

### map_aces_solaris_to_darwin

```c
static int map_aces_solaris_to_darwin(const ace_t *aces, darwin_ace_t *darwin_aces, int ace_count)
```

Defined at lines 242 to 316.

Maps ACE array from Solaris to Darwin.

Returns: number of mapped ACEs or -1 on error.

Note: Darwin ACEs are stored in network byte order.

Note: All errors while mapping (e.g. getting UUIDs from LDAP) are fatal.

Calls: [getuuidfromname](../../libatalk/acl/uuid.c.md#getuuidfromname)

Called by: [map_acl](acls.c.md#map_acl)

### map_aces_darwin_to_solaris

```c
static int map_aces_darwin_to_solaris(darwin_ace_t *darwin_aces, ace_t *nfsv4_aces, int ace_count)
```

Defined at lines 324 to 413.

Maps ACE array from Darwin to Solaris.

Returns: number of mapped ACEs or -1 on error.

Note: Darwin ACEs are expected in network byte order.

Note: All errors while mapping (e.g. getting UUIDs from LDAP) are fatal.

Calls: [getnamefromuuid](../../libatalk/acl/uuid.c.md#getnamefromuuid)

Called by: [map_acl](acls.c.md#map_acl)

Uses file-scope variables: `uuidtype` in [libatalk/acl/uuid.c](../../libatalk/acl/uuid.c.md)

### posix_permset_to_darwin_rights

```c
static uint32_t posix_permset_to_darwin_rights(acl_entry_t e, int is_dir)
```

Defined at lines 422 to 467.

Called by: [map_acl_posix_to_darwin](acls.c.md#map_acl_posix_to_darwin), [posix_acl_rights](acls.c.md#posix_acl_rights)

### posix_acl_rights

```c
static int posix_acl_rights(const AFPObj *obj, const char *path, const struct stat *sb, uint32_t *result)
```

Defined at lines 484 to 586.

Compile access rights for a user to one file-system object.

This combines combines all access rights for a user to one fs-object and returns the result as a Darwin allowed rights ACE. This must honor trivial ACEs which are a mode_t mapping.

Parameters:
* `obj`: handle
* `path`: path to filesystem object
* `sb`: struct stat of path
* `result`: resulting Darwin allow ACE

Returns: 0 or -1 on error

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [gmem](../../libatalk/util/unix.c.md#gmem), [posix_permset_to_darwin_rights](acls.c.md#posix_permset_to_darwin_rights)

Called by: [check_acl_access](acls.c.md#check_acl_access)

### acl_permset_to_uarights

```c
static uint8_t acl_permset_to_uarights(acl_entry_t entry)
```

Defined at lines 598 to 632.

Convert Posix ACL permissions into access rights.

Helper function for [posix_acls_to_uaperms()](acls.c.md#posix_acls_to_uaperms) to convert Posix ACL permissions into access rights needed to fill ua_permissions of a FPUnixPrivs structure.

Parameters:
* `entry`: Posix ACL entry

Returns: access rights

Called by: [posix_acls_to_uaperms](acls.c.md#posix_acls_to_uaperms)

### posix_acls_to_uaperms

```c
static int posix_acls_to_uaperms(const AFPObj *obj, const char *path, struct stat *sb, struct maccess *ma)
```

Defined at lines 652 to 752.

Update FPUnixPrivs for a file-system object on a volume supporting ACLs.

Checks permissions granted by ACLS for a user to one fs-object and updates user and group permissions in given struct maccess. As OS X doesn't conform to Posix 1003.1e Draft 17 it expects proper group permissions in st_mode of struct stat even if the fs-object has an ACL_MASK entry, st_mode gets modified to properly reflect group permissions.

Parameters:
* `obj`: handle
* `path`: path to filesystem object
* `sb`: struct stat of path
* `ma`: struct maccess of path

Returns: 0 or -1 on error

Calls: [acl_permset_to_uarights](acls.c.md#acl_permset_to_uarights), [gmem](../../libatalk/util/unix.c.md#gmem)

Called by: [acltoownermode](acls.c.md#acltoownermode)

Mentioned in the documentation of: [acl_permset_to_uarights](acls.c.md#acl_permset_to_uarights)

### map_darwin_right_to_posix_permset

```c
static acl_perm_t map_darwin_right_to_posix_permset(uint32_t darwin_ace_rights, int is_dir)
```

Defined at lines 768 to 787.

Map Darwin ACE rights to POSIX 1e perm.

We can only map few rights:

* DARWIN_ACE_READ_DATA -> ACL_READ
* DARWIN_ACE_WRITE_DATA -> ACL_WRITE
* DARWIN_ACE_DELETE_CHILD & (is_dir == 1) -> ACL_WRITE
* DARWIN_ACE_EXECUTE -> ACL_EXECUTE

Parameters:
* `darwin_ace_rights`: result of the mapping
* `is_dir`: 1 for dirs, 0 for files

Returns: mapping result as acl_perm_t, -1 on error

Called by: [map_aces_darwin_to_posix](acls.c.md#map_aces_darwin_to_posix)

### posix_acl_add_perm

```c
static int posix_acl_add_perm(acl_t *aclp, acl_tag_t type, uid_t id, acl_perm_t perm)
```

Defined at lines 805 to 855.

Add a ACL_USER or ACL_GROUP permission to an ACL, extending existing ACEs.

Add a permission of "type" for user or group "id" to an ACL. Scan the ACL for existing permissions for this type/id, if there is one add the perm, otherwise create a new ACL entry. perm can be or'ed ACL_READ, ACL_WRITE and ACL_EXECUTE.

Parameters:
* `aclp`: pointer to ACL
* `type`: acl_tag_t of ACL_USER or ACL_GROUP
* `id`: uid_t uid for ACL_USER, or gid casted to uid_t for ACL_GROUP
* `perm`: acl_perm_t permissions to add

Returns: 0 on success, -1 on failure

Called by: [map_aces_darwin_to_posix](acls.c.md#map_aces_darwin_to_posix)

### map_aces_darwin_to_posix

```c
static int map_aces_darwin_to_posix(const darwin_ace_t *darwin_aces, acl_t *def_aclp, acl_t *acc_aclp, int ace_count, uint32_t *default_acl_flags)
```

Defined at lines 879 to 1008.

Map Darwin ACL to POSIX ACL.

aclp must point to a acl_init'ed acl_t or an acl_t that can e.g. contain default ACEs. Mapping pecularities:

* we create a default ace (which inherits to files and dirs) if either DARWIN_ACE_FLAGS_FILE_INHERIT or DARWIN_ACE_FLAGS_DIRECTORY_INHERIT is requested
* we throw away DARWIN_ACE_FLAGS_LIMIT_INHERIT (can't be mapped), thus the ACL will not be limited

Parameters:
* `darwin_aces`: pointer to darwin_aces buffer
* `def_aclp`: directories: pointer to an initialized acl_t with the default acl files: *def_aclp will be NULL
* `acc_aclp`: pointer to an initialized acl_t with the access acl
* `ace_count`: number of ACEs in darwin_aces buffer
* `default_acl_flags`: flags to indicate if the object has a basic default acl or an extended default acl.

Returns: 0 on success storing the result in aclp, -1 on error. default_acl_flags is set to HAS_DEFAULT_ACL|HAS_EXT_DEFAULT_ACL in case there is at least one extended default ace. Otherwise default_acl_flags is left unchanged.

Calls: [getnamefromuuid](../../libatalk/acl/uuid.c.md#getnamefromuuid), [map_darwin_right_to_posix_permset](acls.c.md#map_darwin_right_to_posix_permset), [posix_acl_add_perm](acls.c.md#posix_acl_add_perm), [uuid_bin2string](../../libatalk/acl/uuid.c.md#uuid_bin2string)

Called by: [set_acl](acls.c.md#set_acl)

Uses file-scope variables: `uuidtype` in [libatalk/acl/uuid.c](../../libatalk/acl/uuid.c.md)

### map_acl_posix_to_darwin

```c
static int map_acl_posix_to_darwin(int type, const acl_t acl, darwin_ace_t *darwin_aces)
```

Defined at lines 1015 to 1106.

Map ACEs from POSIX to Darwin.

Returns: number of mapped ACES, -1 on error.

Note: type is either POSIX_DEFAULT_2_DARWIN or POSIX_ACCESS_2_DARWIN, cf. acl_get_file.

Calls: [getuuidfromname](../../libatalk/acl/uuid.c.md#getuuidfromname), [posix_permset_to_darwin_rights](acls.c.md#posix_permset_to_darwin_rights)

Called by: [map_acl](acls.c.md#map_acl)

### map_acl

```c
static int map_acl(int type, void *acl, darwin_ace_t *buf, int ace_count)
```

Defined at lines 1120 to 1160.

Multiplex ACL mapping (SOLARIS_2_DARWIN, DARWIN_2_SOLARIS, POSIX_2_DARWIN, DARWIN_2_POSIX).

Reads from 'aces' buffer, writes to 'rbuf' buffer. Caller must provide buffer. Darwin ACEs are read and written in network byte order. Needs to know how many ACEs are in the ACL (ace_count) for Solaris ACLs. Ignores trivial ACEs.

Returns: no of mapped ACEs or -1 on error.

Calls: [map_aces_darwin_to_solaris](acls.c.md#map_aces_darwin_to_solaris), [map_aces_solaris_to_darwin](acls.c.md#map_aces_solaris_to_darwin), [map_acl_posix_to_darwin](acls.c.md#map_acl_posix_to_darwin)

Called by: [get_and_map_acl](acls.c.md#get_and_map_acl), [set_acl](acls.c.md#set_acl)

### get_and_map_acl

```c
static int get_and_map_acl(char *name, char *rbuf, size_t *rbuflen)
```

Defined at lines 1168 to 1241.

Get ACL from object omitting trivial ACEs.

Map to Darwin ACL style and store Darwin ACL at rbuf. Add length of ACL written to rbuf to *rbuflen.

Returns: 0 on success, -1 on error.

Calls: [get_nfsv4_acl](../../libatalk/acl/unix.c.md#get_nfsv4_acl), [map_acl](acls.c.md#map_acl)

Called by: [afp_getacl](acls.c.md#afp_getacl)

### remove_acl

```c
static int remove_acl(const struct vol *vol, const char *path, int dir)
```

Defined at lines 1246 to 1265.

Removes all non-trivial ACLs from object.

Returns: full AFPERR code.

Calls: [remove_nfsv4_acl_vfs](../../libatalk/vfs/acl.c.md#remove_nfsv4_acl_vfs), [remove_posix_acl_vfs](../../libatalk/vfs/acl.c.md#remove_posix_acl_vfs)

Called by: [afp_setacl](acls.c.md#afp_setacl)

Calls through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_posix_remove_acl](../../libatalk/vfs/vfs.c.md#rf_posix_remove_acl), [RF_solaris_remove_acl](../../libatalk/vfs/vfs.c.md#rf_solaris_remove_acl), [vfs_remove_acl](../../libatalk/vfs/vfs.c.md#vfs_remove_acl)

### set_acl

```c
static int set_acl(const struct vol *vol, char *name, int inherit, darwin_ace_t *daces, uint32_t ace_count)
```

Defined at lines 1279 to 1408. Defined 2 times under conditional compilation.

Set ACL.

Note: Subtleties: * the client sends a complete list of ACEs, not only new ones. So we don't need to do any combination business (one exception being 'kFileSec_Inherit': see next)
* client might request that we add inherited ACEs via 'kFileSec_Inherit'. We will store inherited ACEs first, which is Darwins canonical order.

Returns: AFPerror code

Calls: [get_nfsv4_acl](../../libatalk/acl/unix.c.md#get_nfsv4_acl), [map_aces_darwin_to_posix](acls.c.md#map_aces_darwin_to_posix), [map_acl](acls.c.md#map_acl)

Called by: [afp_setacl](acls.c.md#afp_setacl)

Calls through [`vfs_ops::vfs_solaris_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_solaris_acl](../../libatalk/vfs/vfs.c.md#rf_solaris_acl), [vfs_solaris_acl](../../libatalk/vfs/vfs.c.md#vfs_solaris_acl)

### acl_from_mode

```c
static acl_t acl_from_mode(mode_t mode)
```

Defined at lines 1413 to 1492.

### check_acl_access

```c
static int check_acl_access(const AFPObj *obj, const struct vol *vol, struct dir *dir, const char *path, const uuidp_t uuid, uint32_t requested_rights)
```

Defined at lines 1612 to 1743.

Checks if a given UUID has requested_rights (type darwin_ace_rights) for path.

Note: this gets called frequently and is a good place for optimizations !

Parameters:
* `obj`: AFP object
* `vol`: volume
* `dir`: directory
* `path`: path to filesystem object
* `uuid`: UUID of user
* `requested_rights`: requested Darwin ACE

Returns: AFP result code

Calls: [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [ostat](../../libatalk/util/unix.c.md#ostat), [posix_acl_rights](acls.c.md#posix_acl_rights), [solaris_acl_rights](acls.c.md#solaris_acl_rights)

Called by: [afp_access](acls.c.md#afp_access)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_access

```c
int afp_access(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1749 to 1804.

Calls: [check_acl_access](acls.c.md#check_acl_access), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_statdir](ofork.c.md#of_statdir)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `uuid` in [etc/afpd/auth.h](auth.h.md)

### afp_getacl

```c
int afp_getacl(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1806 to 1922.

Calls: [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_and_map_acl](acls.c.md#get_and_map_acl), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [getuuidfromname](../../libatalk/acl/uuid.c.md#getuuidfromname), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [localuuid_from_id](../../libatalk/acl/uuid.c.md#localuuid_from_id), [of_statdir](ofork.c.md#of_statdir)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### afp_setacl

```c
int afp_setacl(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1924 to 2108.

Calls: [cname](directory.c.md#cname), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_statdir](ofork.c.md#of_statdir), [ostat](../../libatalk/util/unix.c.md#ostat), [remove_acl](acls.c.md#remove_acl), [set_acl](acls.c.md#set_acl), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

### acltoownermode

```c
int acltoownermode(const AFPObj *obj, const struct vol *vol, char *path, struct stat *st, struct maccess *ma)
```

Defined at lines 2120 to 2147.

map ACL to user maccess

This is the magic function that makes ACLs usable by calculating the access granted by ACEs to the logged in user.

Calls: [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [posix_acls_to_uaperms](acls.c.md#posix_acls_to_uaperms), [solaris_acl_rights](acls.c.md#solaris_acl_rights)

Called by: [accessmode](unix.c.md#accessmode)

# Macros

* Undocumented: `DARWIN_2_POSIX_ACCESS`, `DARWIN_2_POSIX_DEFAULT`, `DARWIN_2_SOLARIS`, `HAS_DEFAULT_ACL`, `HAS_EXT_DEFAULT_ACL`, `IS_DIR`, `MAP_MASK`, `POSIX_ACCESS_2_DARWIN`, `POSIX_DEFAULT_2_DARWIN`, `SOLARIS_2_DARWIN`
