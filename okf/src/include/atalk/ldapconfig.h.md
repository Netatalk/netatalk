---
type: C Header File
title: "include/atalk/ldapconfig.h"
description: "LDAP preference types and configuration lifecycle interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/ldapconfig.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `iniparser.h`

# Included by

* [bin/afpldaptest/uuidtest.c](../../bin/afpldaptest/uuidtest.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md)
* [libatalk/acl/ldap_config.c](../../libatalk/acl/ldap_config.c.md)
* [libatalk/acl/uuid.c](../../libatalk/acl/uuid.c.md)

# Types

### struct ldap_pref

Defined at line 42.
* `const void * pref`
* `char * name`
* `int strorint`: string to just store in char * or convert to int ?
* `int intfromarray`: convert to int, but use string to int mapping array [pref_array](ldapconfig.h.md#struct-pref_array)[]
* `int valid`: -1 = mandatory, 0 = omittable/valid
* `int valid_save`: copy of 'valid', used when resettting config

### struct pref_array

Defined at line 51.
* `const char * pref`: name of pref from ldap_prefs[] to which this value corresponds
* `char * valuestring`: config string
* `int value`: corresponding value

# Typedefs and enums

* `enum ldap_uuid_encoding_type`: `LDAP_UUID_ENCODING_STRING`, `LDAP_UUID_ENCODING_MSGUID`

# File-scope variables

`ldap_server`
