---
type: C Source File
title: "libatalk/acl/ldap.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/acl/ldap.c"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/acl](../acl.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/ldapconfig.h](../../include/atalk/ldapconfig.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* System headers: `ctype.h`, `errno.h`, `ldap.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/time.h`

# Functions

### ldap_getattr_fromfilter_withbase_scope

```c
static int ldap_getattr_fromfilter_withbase_scope(const char *searchbase, const char *filter, char *attributes[], int scope, ldapcon_t conflags, char **result)
```

Defined at lines 124 to 289.

LDAP get attribute from filter with base and scope.

All connection managment to the LDAP server is done here. Just set KEEPALIVE if you know you will be dispatching more than one search in a row, then don't set it with the last search. You MUST dispatch the queries timely, otherwise the LDAP handle might timeout.

Parameters:
* `searchbase`: Base DN for LDAP search
* `filter`: LDAP search filter
* `attributes`: Array of attribute names to retrieve
* `scope`: Search scope (LDAP_SCOPE_BASE, LDAP_SCOPE_ONELEVEL, LDAP_SCOPE_SUBTREE)
* `conflags`: Connection flags (KEEPALIVE)
* `result`: unique search result, allocated here, caller must free

Returns: -1 on error

Returns: 0 nothing found

Returns: 1 successful search, result int 'result'

Called by: [ldap_getnamefromuuid](ldap.c.md#ldap_getnamefromuuid), [ldap_getuuidfromname](ldap.c.md#ldap_getuuidfromname)

Uses file-scope variables: `ldap_auth_dn`, `ldap_auth_method`, `ldap_auth_pw`, `ldap_uri`

### gen_uuid_filter

```c
static char * gen_uuid_filter(const char *uuidstr_in, const char *attr_filter)
```

Defined at lines 298 to 355.

Generate LDAP filter string for UUID query.

Parameters:
* `uuidstr_in`: the UUID as string
* `attr_filter`: optional attribute

Returns: pointer to static filter string

Called by: [ldap_getnamefromuuid](ldap.c.md#ldap_getnamefromuuid)

Uses file-scope variables: `ldap_uuid_attr`, `ldap_uuid_encoding`

### ldap_escape_filter_value

```c
static int ldap_escape_filter_value(const char *src, char *dst, size_t dstlen)
```

Defined at lines 362 to 387.

Called by: [ldap_getuuidfromname](ldap.c.md#ldap_getuuidfromname)

### ldap_getuuidfromname

```c
int ldap_getuuidfromname(const char *name, uuidtype_t type, char **uuid_string)
```

Defined at lines 400 to 477. Declared in [libatalk/acl/aclldap.h](aclldap.h.md).

Search UUID for name in LDAP.

Caller must free uuid_string when done with it

Parameters:
* `name`: name to search
* `type`: type of USER or GROUP
* `uuid_string`: result as pointer to allocated UUID-string

Returns: 0 on success, -1 on error or not found

Calls: [ldap_escape_filter_value](ldap.c.md#ldap_escape_filter_value), [ldap_getattr_fromfilter_withbase_scope](ldap.c.md#ldap_getattr_fromfilter_withbase_scope)

Called by: [getuuidfromname](uuid.c.md#getuuidfromname)

Uses file-scope variables: `ldap_config_valid`, `ldap_group_attr`, `ldap_groupbase`, `ldap_groupscope`, `ldap_name_attr`, `ldap_userbase`, `ldap_userscope`, `ldap_uuid_attr`, `ldap_uuid_encoding`

### ldap_getnamefromuuid

```c
int ldap_getnamefromuuid(const char *uuidstr, char **name, uuidtype_t *type)
```

Defined at lines 490 to 538. Declared in [libatalk/acl/aclldap.h](aclldap.h.md).

LDAP search wrapper.

returns allocated storage in name, caller must free it

Parameters:
* `uuidstr`: uuid to search as ascii string
* `name`: return pointer to name as allocated string
* `type`: return type: USER or GROUP

Returns: 0 on success, -1 on errror or not found

Calls: [gen_uuid_filter](ldap.c.md#gen_uuid_filter), [ldap_getattr_fromfilter_withbase_scope](ldap.c.md#ldap_getattr_fromfilter_withbase_scope)

Called by: [getnamefromuuid](uuid.c.md#getnamefromuuid)

Uses file-scope variables: `ldap_config_valid`, `ldap_group_attr`, `ldap_groupbase`, `ldap_groupfilter`, `ldap_groupscope`, `ldap_name_attr`, `ldap_userbase`, `ldap_userfilter`, `ldap_userscope`

# Typedefs and enums

* `enum ldapcon_t`: `KEEPALIVE`

# Macros

* Undocumented: `LDAP_BIN_UUID_LEN`, `LDAP_DEPRECATED`, `MAX_FILTER_SIZE`

# File-scope variables

`ldap_auth_dn`, `ldap_auth_method`, `ldap_auth_pw`, `ldap_config_valid`, `ldap_group_attr`, `ldap_groupbase`, `ldap_groupfilter`, `ldap_groupscope`, `ldap_name_attr`, `ldap_prefs`, `ldap_uid_attr`, `ldap_uri`, `ldap_userbase`, `ldap_userfilter`, `ldap_userscope`, `ldap_uuid_attr`, `ldap_uuid_encoding`, `ldap_uuid_string`, `prefs_array`
