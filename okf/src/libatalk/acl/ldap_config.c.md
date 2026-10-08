---
type: C Source File
title: "libatalk/acl/ldap_config.c"
description: "2 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/acl/ldap_config.c"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/acl](../acl.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/iniparser_util.h](../../include/atalk/iniparser_util.h.md)
* [atalk/ldapconfig.h](../../include/atalk/ldapconfig.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `ctype.h`, `errno.h`, `iniparser.h`, `ldap.h`, `stdio.h`, `stdlib.h`, `string.h`

# Functions

### acl_ldap_freeconfig

```c
void acl_ldap_freeconfig(void)
```

Defined at lines 38 to 50.

Called by: [configinit](../../etc/afpd/afp_config.c.md#configinit)

Uses file-scope variables: `ldap_prefs` in [libatalk/acl/ldap.c](ldap.c.md)

### acl_ldap_readconfig

```c
int acl_ldap_readconfig(dictionary *iniconfig)
```

Defined at lines 52 to 121.

Called by: [configinit](../../etc/afpd/afp_config.c.md#configinit), [parse_ldapconf](../../bin/misc/uuidtest.c.md#parse_ldapconf)

Uses file-scope variables: `ldap_auth_method` in [libatalk/acl/ldap.c](ldap.c.md), `ldap_config_valid` in [libatalk/acl/ldap.c](ldap.c.md), `ldap_prefs` in [libatalk/acl/ldap.c](ldap.c.md), `prefs_array` in [libatalk/acl/ldap.c](ldap.c.md)
