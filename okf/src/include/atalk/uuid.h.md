---
type: C Header File
title: "include/atalk/uuid.h"
description: "No functions or types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/uuid.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [bin/misc/uuidtest.c](../../bin/misc/uuidtest.c.md)
* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/acls.h](../../etc/afpd/acls.h.md)
* [etc/afpd/afp_dsi.c](../../etc/afpd/afp_dsi.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [include/atalk/cnid.h](cnid.h.md)
* [include/atalk/cnid_mysql_private.h](cnid_mysql_private.h.md)
* [include/atalk/cnid_sqlite_private.h](cnid_sqlite_private.h.md)
* [libatalk/acl/aclldap.h](../../libatalk/acl/aclldap.h.md)
* [libatalk/acl/cache.c](../../libatalk/acl/cache.c.md)
* [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md)
* [libatalk/acl/uuid.c](../../libatalk/acl/uuid.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)

# Typedefs and enums

* `typedef unsigned char atalk_uuid_t`
* `typedef const unsigned char * uuidp_t`
* `enum uuidtype_t`: `UUID_USER`, `UUID_GROUP`, `UUID_ENOENT`

# Macros

* Undocumented: `UUIDTYPESTR_MASK`, `UUID_BINSIZE`
