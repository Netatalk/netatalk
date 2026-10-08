---
type: C Header File
title: "etc/afpd/acls.h"
description: "Definitions for ACL mapping code."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/acls.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* [directory.h](directory.h.md)

# Included by

* [etc/afpd/acl_mappings.h](acl_mappings.h.md)
* [etc/afpd/acls.c](acls.c.md)
* [etc/afpd/auth.c](auth.c.md)
* [etc/afpd/switch.c](switch.c.md)
* [etc/afpd/unix.c](unix.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Types

### struct darwin_ace_t

Defined at line 105.
* `atalk_uuid_t darwin_ace_uuid`
* `uint32_t darwin_ace_flags`
* `uint32_t darwin_ace_rights`

### struct darwin_acl_header_t

Defined at line 112.
* `uint32_t darwin_acl_count`
* `uint32_t darwin_acl_flags`

# Typedefs and enums

* `enum (anonymous)`: `kFileSec_UUID`, `kFileSec_GRPUUID`, `kFileSec_ACL`, `kFileSec_REMOVEACL`, `kFileSec_Inherit`

# Macros

* Undocumented: `ACE_TRIVIAL`, `DARWIN_ACE_ADD_FILE`, `DARWIN_ACE_ADD_SUBDIRECTORY`, `DARWIN_ACE_APPEND_DATA`, `DARWIN_ACE_DELETE`, `DARWIN_ACE_DELETE_CHILD`, `DARWIN_ACE_EXECUTE`, `DARWIN_ACE_FLAGS_DENY`, `DARWIN_ACE_FLAGS_DIRECTORY_INHERIT`, `DARWIN_ACE_FLAGS_FILE_INHERIT`, `DARWIN_ACE_FLAGS_INHERITED`, `DARWIN_ACE_FLAGS_KINDMASK`, `DARWIN_ACE_FLAGS_LIMIT_INHERIT`, `DARWIN_ACE_FLAGS_ONLY_INHERIT`, `DARWIN_ACE_FLAGS_PERMIT`, `DARWIN_ACE_INHERIT_CONTROL_FLAGS`, `DARWIN_ACE_LIST_DIRECTORY`, `DARWIN_ACE_READ_ATTRIBUTES`, `DARWIN_ACE_READ_DATA`, `DARWIN_ACE_READ_EXTATTRIBUTES`, `DARWIN_ACE_READ_SECURITY`, `DARWIN_ACE_SEARCH`, `DARWIN_ACE_TAKE_OWNERSHIP`, `DARWIN_ACE_WRITE_ATTRIBUTES`, `DARWIN_ACE_WRITE_DATA`, `DARWIN_ACE_WRITE_EXTATTRIBUTES`, `DARWIN_ACE_WRITE_SECURITY`, `DARWIN_ACL_FLAGS_PRIVATE`, `KAUTH_ACL_DEFER_INHERIT`, `KAUTH_ACL_NO_INHERIT`
