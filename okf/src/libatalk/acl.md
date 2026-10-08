---
type: Subsystem
title: "libatalk/acl"
description: "7 files, 26 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/acl"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [libatalk/acl/aclldap.h](acl/aclldap.h.md): includes 1 project header.
* [libatalk/acl/cache.c](acl/cache.c.md): 7 functions, 1 type, includes 4 project headers.
* [libatalk/acl/cache.h](acl/cache.h.md): LDAP cache interface.
* [libatalk/acl/ldap.c](acl/ldap.c.md): 5 functions, includes 5 project headers.
* [libatalk/acl/ldap_config.c](acl/ldap_config.c.md): 2 functions, includes 4 project headers.
* [libatalk/acl/unix.c](acl/unix.c.md): 7 functions, includes 5 project headers.
* [libatalk/acl/uuid.c](acl/uuid.c.md): 5 functions, includes 7 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 23 includes

# Calls into

* [libatalk/util](util.md): 5 calls

# Called from

* [etc/afpd](../etc/afpd.md): 17 calls
* [bin/misc](../bin/misc.md): 5 calls
* [libatalk/util](util.md): 2 calls
* [libatalk/vfs](vfs.md): 1 calls

# Most called functions

* [uuid_bin2string](acl/uuid.c.md#uuid_bin2string): 7 callers
* [get_nfsv4_acl](acl/unix.c.md#get_nfsv4_acl): 6 callers
* [getuuidfromname](acl/uuid.c.md#getuuidfromname): 6 callers
* [getnamefromuuid](acl/uuid.c.md#getnamefromuuid): 4 callers
* [acl_ldap_readconfig](acl/ldap_config.c.md#acl_ldap_readconfig): 2 callers
* [hashstring](acl/cache.c.md#hashstring): 2 callers
* [hashuuid](acl/cache.c.md#hashuuid): 2 callers
* [ldap_getattr_fromfilter_withbase_scope](acl/ldap.c.md#ldap_getattr_fromfilter_withbase_scope): 2 callers
* [localuuid_from_id](acl/uuid.c.md#localuuid_from_id): 2 callers
* [uuid_string2bin](acl/uuid.c.md#uuid_string2bin): 2 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[nfsv4_chmod](acl/unix.c.md#nfsv4_chmod), [posix_chmod](acl/unix.c.md#posix_chmod), [posix_fchmod](acl/unix.c.md#posix_fchmod)
