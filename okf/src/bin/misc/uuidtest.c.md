---
type: C Source File
title: "bin/misc/uuidtest.c"
description: "The afpldaptest command: map names to UUIDs and back through LDAP, with a local fallback."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/misc/uuidtest.c"
tags: ["bin/misc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [bin/misc](../misc.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/ldapconfig.h](../../include/atalk/ldapconfig.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* System headers: `iniparser.h`, `ldap.h`, `stdarg.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Functions

### usage

```c
static void usage(void)
```

Defined at lines 52 to 55.

Print the usage of afpldaptest to stdout.

Called by: [main](uuidtest.c.md#main)

### parse_ldapconf

```c
static void parse_ldapconf(void)
```

Defined at lines 66 to 98.

Read the LDAP settings of afp.conf on the first call and report whether they are valid.

The call loads afp.conf from the configuration directory and passes it to acl_ldap_readconfig. It prints whether the settings are valid and which bind they use, and an unsupported authentication method exits with EXIT_FAILURE. A build without LDAP only prints a notice.

Calls: [acl_ldap_readconfig](../../libatalk/acl/ldap_config.c.md#acl_ldap_readconfig)

Called by: [main](uuidtest.c.md#main)

Uses file-scope variables: `ldap_auth_method` in [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md), `ldap_config_valid` in [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 114 to 209.

Map user and group names to UUIDs and back through the netatalk UUID layer.

The options run in order. The -u and -g options print the UUID of a user or a group, and -i prints the name and the type behind a UUID. The first lookup reads the LDAP settings of afp.conf. The lookups log to /dev/tty at the info level, or at maxdebug after -v.

Parameters:
* `argc`: argument count
* `argv`: argument vector

Returns: 0, also when a lookup fails, while an unknown option or a missing argument exits with EXIT_FAILURE

Calls: [getnamefromuuid](../../libatalk/acl/uuid.c.md#getnamefromuuid), [getuuidfromname](../../libatalk/acl/uuid.c.md#getuuidfromname), [parse_ldapconf](uuidtest.c.md#parse_ldapconf), [setuplog](../../libatalk/util/logger.c.md#setuplog), [usage](uuidtest.c.md#usage), [uuid_bin2string](../../libatalk/acl/uuid.c.md#uuid_bin2string), [uuid_string2bin](../../libatalk/acl/uuid.c.md#uuid_string2bin)

# Macros

* Undocumented: `LDAP_DEPRECATED`, `STRNCMP`
