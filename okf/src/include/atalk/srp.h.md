---
type: C Header File
title: "include/atalk/srp.h"
description: "3 functions."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/srp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `ctype.h`, `string.h`, `sys/stat.h`, `sys/types.h`

# Included by

* [bin/afppasswd/afppasswd.c](../../bin/afppasswd/afppasswd.c.md)
* [bin/afppasswd/afppasswd_migrate.c](../../bin/afppasswd/afppasswd_migrate.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [etc/uams/uams_srp.c](../../etc/uams/uams_srp.c.md)

# Functions

### srp_valid_username

```c
static int srp_valid_username(const char *name)
```

Defined at lines 58 to 64.

Calls: [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [add_record](../../bin/afppasswd/afppasswd_migrate.c.md#add_record), [create_file](../../bin/afppasswd/afppasswd.c.md#create_file), [create_private_srp_verifier](../../bin/afppasswd/afppasswd.c.md#create_private_srp_verifier), [create_srp_directory](../../bin/afppasswd/afppasswd.c.md#create_srp_directory), [disable_srp_verifier](../../bin/afppasswd/afppasswd.c.md#disable_srp_verifier), [srp_lookup_verifier](../../etc/uams/uams_srp.c.md#srp_lookup_verifier), [update_passwd](../../bin/afppasswd/afppasswd.c.md#update_passwd), [update_srp_passwd](../../bin/afppasswd/afppasswd.c.md#update_srp_passwd)

### srp_valid_fields

```c
static int srp_valid_fields(const char *fields)
```

Defined at lines 70 to 105.

Called by: [add_record](../../bin/afppasswd/afppasswd_migrate.c.md#add_record), [srp_lookup_verifier](../../etc/uams/uams_srp.c.md#srp_lookup_verifier), [update_srp_passwd](../../bin/afppasswd/afppasswd.c.md#update_srp_passwd)

### srp_verifier_mode_is_safe

```c
static int srp_verifier_mode_is_safe(mode_t mode)
```

Defined at lines 108 to 111.

Called by: [create_verifiers](../../bin/afppasswd/afppasswd_migrate.c.md#create_verifiers), [srp_lookup_verifier](../../etc/uams/uams_srp.c.md#srp_lookup_verifier), [srp_verifier_store_is_private](../../etc/netatalk/netatalk.c.md#srp_verifier_store_is_private), [validate_source](../../bin/afppasswd/afppasswd_migrate.c.md#validate_source), [validate_srp_verifier_file](../../bin/afppasswd/afppasswd.c.md#validate_srp_verifier_file)

# Macros

* Undocumented: `SRP_DISABLED_CHAR`, `SRP_FIELDS_LEN`, `SRP_FORMAT_LEN`, `SRP_GROUP_INDEX`, `SRP_HEX_SALT_LEN`, `SRP_HEX_V_LEN`, `SRP_NBYTES`, `SRP_SALT_LEN`, `SRP_SHA1_LEN`, `SRP_USERNAME_MAX_LEN`

# File-scope variables

`srp_N_bytes`, `srp_g_byte`
