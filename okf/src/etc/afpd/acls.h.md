---
type: C Header File
title: "etc/afpd/acls.h"
description: "AFP ACL command handlers and access permission helpers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/acls.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
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

Access Control List Entry (ACE).

Defined at line 108.
* `atalk_uuid_t darwin_ace_uuid`
* `uint32_t darwin_ace_flags`
* `uint32_t darwin_ace_rights`

### struct darwin_acl_header_t

Access Control List.

Defined at line 115.
* `uint32_t darwin_acl_count`
* `uint32_t darwin_acl_flags`

# Typedefs and enums

* `enum (anonymous)`: `kFileSec_UUID`, `kFileSec_GRPUUID`, `kFileSec_ACL`, `kFileSec_REMOVEACL`, `kFileSec_Inherit`

# Macros

* `ACE_TRIVIAL`: Some stuff for the handling of NFSv4 ACLs.
* `DARWIN_ACE_FLAGS_DENY`: 0x00000002
* `DARWIN_ACE_FLAGS_DIRECTORY_INHERIT`: 0x00000040
* `DARWIN_ACE_FLAGS_FILE_INHERIT`: 0x00000020
* `DARWIN_ACE_FLAGS_INHERITED`: 0x00000010
* `DARWIN_ACE_FLAGS_LIMIT_INHERIT`: 0x00000080
* `DARWIN_ACE_FLAGS_ONLY_INHERIT`: 0x00000100
* `DARWIN_ACE_FLAGS_PERMIT`: 0x00000001
* `DARWIN_ACE_INHERIT_CONTROL_FLAGS`: All flag bits controlling ACE inheritance.
* `DARWIN_ACL_FLAGS_PRIVATE`: ACL Flags.
* `KAUTH_ACL_DEFER_INHERIT`: inheritance will be deferred until the first rename operation
* `KAUTH_ACL_NO_INHERIT`: this ACL must not be overwritten as part of an inheritance operation
* Undocumented: `DARWIN_ACE_ADD_FILE`, `DARWIN_ACE_ADD_SUBDIRECTORY`, `DARWIN_ACE_APPEND_DATA`, `DARWIN_ACE_DELETE`, `DARWIN_ACE_DELETE_CHILD`, `DARWIN_ACE_EXECUTE`, `DARWIN_ACE_FLAGS_KINDMASK`, `DARWIN_ACE_LIST_DIRECTORY`, `DARWIN_ACE_READ_ATTRIBUTES`, `DARWIN_ACE_READ_DATA`, `DARWIN_ACE_READ_EXTATTRIBUTES`, `DARWIN_ACE_READ_SECURITY`, `DARWIN_ACE_SEARCH`, `DARWIN_ACE_TAKE_OWNERSHIP`, `DARWIN_ACE_WRITE_ATTRIBUTES`, `DARWIN_ACE_WRITE_DATA`, `DARWIN_ACE_WRITE_EXTATTRIBUTES`, `DARWIN_ACE_WRITE_SECURITY`
