---
type: C Source File
title: "libatalk/util/netatalk_conf.c"
description: "Reading afp.conf and extmap.conf: server options, volume definitions, and the extension map."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/netatalk_conf.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T15:29:27+11:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/iniparser_util.h](../../include/atalk/iniparser_util.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* System headers: `arpa/inet.h`, `bstrlib.h`, `ctype.h`, `errno.h`, `grp.h`, `iniparser.h`, `inttypes.h`, `limits.h`, `netdb.h`, `netinet/in.h`, `pwd.h`, `regex.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/param.h`, `sys/socket.h`, `time.h`, `unistd.h`, `utime.h`

# Functions

### conf_testutil_set_lastvid

```c
void conf_testutil_set_lastvid(uint16_t vid)
```

Defined at lines 109 to 112.

Test seam: set the volume-id counter.

Saturating it (UINT16_MAX) makes the next [creatvol()](netatalk_conf.c.md#creatvol) take the vid-overflow failure path. Lives here because the static it mutates does; declared in the test-local subtests_conf.h, not the public API.

Parameters:
* `vid`: new counter value ([unload_volumes()](netatalk_conf.c.md#unload_volumes) resets it to 0)

Uses file-scope variables: `lastvid`

### rewrite_vol_uuid_conf

```c
static int rewrite_vol_uuid_conf(AFPObj *obj, struct vol *Volumes)
```

Defined at lines 127 to 195.

Rewrite the volume UUID file from the volume chain, which drops the deleted volumes.

The call truncates the file and writes the header and one line per volume. It takes the write lock without waiting, so a lock that another process holds fails the call. A file that cannot open is no failure when every volume has a UUID.

Parameters:
* `obj`: AFP object with the UUID file path
* `Volumes`: volume chain

Returns: 0, or -1 when the open, the lock, the truncate, or the seek fails

Called by: [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols)

Uses file-scope variables: `UUID_FILE_HEADER`, `Volumes`

### get_vol_uuid

```c
static char * get_vol_uuid(const AFPObj *obj, const char *volname)
```

Defined at lines 210 to 356.

Return the UUID of a volume from the volume UUID file, adding one when the file has none.

Each line holds the volume name and then the UUID, and the name can stand in double quotes. The call returns the UUID in upper case. A volume without a line gets 16 random bytes as its UUID, appended under a write lock. Neither lock waits, so a lock that another process holds fails the call.

Parameters:
* `obj`: AFP object with the UUID file path
* `volname`: local name of the volume

Returns: the UUID string, which the caller frees, or NULL on error

Calls: [randombytes](unix.c.md#randombytes), [uuid_bin2string](../acl/uuid.c.md#uuid_bin2string)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

Uses file-scope variables: `UUID_FILE_HEADER`

### do_check_ea_support

```c
static int do_check_ea_support(const struct vol *vol)
```

Defined at lines 370 to 402.

Tell whether the filesystem of a volume supports extended attributes.

The marker attribute org.netatalk.has-Extended-Attributes on the volume root answers at once. A read-only volume cannot take a test write, so the call assumes support. Otherwise it sets the marker as root, and a failure means no support.

Parameters:
* `vol`: volume

Returns: 1 with support, 0 without

Calls: [become_root](unix.c.md#become_root), [sys_lgetxattr](../vfs/extattr.c.md#sys_lgetxattr), [sys_setxattr](../vfs/extattr.c.md#sys_setxattr), [unbecome_root](unix.c.md#unbecome_root)

Called by: [check_ea_support](netatalk_conf.c.md#check_ea_support)

### check_ea_support

```c
static void check_ea_support(struct vol *vol)
```

Defined at lines 413 to 431.

Pick the EA backend and the AppleDouble version of a volume by its EA support.

With ea = auto the volume gets sys or ad. With AppleDouble in extended attributes and no support, the volume falls back to AppleDouble v2 files.

Parameters:
* `vol`: volume

Calls: [do_check_ea_support](netatalk_conf.c.md#do_check_ea_support)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### check_vol_acl_support

```c
static int check_vol_acl_support(const struct vol *vol)
```

Defined at lines 440 to 478.

Check whether a volume supports ACLs.

Parameters:
* `vol`: volume

Returns: 0 if not, 1 if yes

Calls: [get_nfsv4_acl](../acl/unix.c.md#get_nfsv4_acl)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### volxlate

```c
static char * volxlate(const AFPObj *obj, char *dest, size_t destlen, const char *src, const struct passwd *pwd, const char *path, const char *volname)
```

Defined at lines 519 to 702.

Expand the variables of an afp.conf value.

The variables are:

```
$b   basename of path
$c   client address with port, or net.node over AppleTalk
$d   volume path on the server
$f   full name, the gecos field up to its first comma
$g   primary group of the user
$h   host name
$i   client address without port, or the network number over AppleTalk
$s   server name, or the host name without one
$u   user name, after the NT domain separator when one is set
$v   volume name, or the basename of path without one
$$   a dollar sign
```

A volume path expands with a NULL path and volname, and a volume name with the path. Only $c checks for an AFP session, and outside one it expands to nothing. The $f and $g variables stay as typed without a user entry, and $u stays as typed without a login. A variable that lacks its value, such as $b without a path, expands to nothing. An unknown variable stays as typed.

The $f variable cuts the gecos field of pwd at its first comma, in place. The result stops at destlen bytes.

Parameters:
* `obj`: AFP object with the session, the user, and the options
* `dest`: buffer of destlen + 1 bytes, or NULL to allocate one
* `destlen`: length limit of the result
* `src`: value with variables
* `pwd`: user entry, or NULL
* `path`: volume path, or NULL
* `volname`: volume name, or NULL

Returns: dest or the new buffer, which the caller frees, or NULL for a NULL src or a failed calloc

Calls: [getip_port](socket.c.md#getip_port), [getip_string](socket.c.md#getip_string), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [creatvol](netatalk_conf.c.md#creatvol), [dbpath_is_volume_root](netatalk_conf.c.md#dbpath_is_volume_root), [getvolbypath](netatalk_conf.c.md#getvolbypath), [readvolfile](netatalk_conf.c.md#readvolfile), [user_state_paths](netatalk_conf.c.md#user_state_paths)

### accessvol

```c
static int accessvol(const AFPObj *obj, const char *args, const char *name)
```

Defined at lines 724 to 762.

check user access list for volume

This function wants a string consisting of names seperated by comma or space. Names may be quoted within a pair of quotes. Groups are denoted by a leading @ symbol.

Example:

```
user1 user2, user3, @group1, @group2, @group3, "user name1", "@group name1"
```

Parameters:
* `obj`: [AFPObj](../../include/atalk/globals.h.md#struct-afpobj) containing user and group info
* `args`: access list string; NULL allows everybody to have access
* `name`: user name to check

Returns: -1: no list

Returns: 0: list exists, but name isn't in it

Returns: 1: in list

Calls: [gmem](unix.c.md#gmem), [strtok_quote](unix.c.md#strtok_quote)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### hostaccessvol

```c
static int hostaccessvol(const AFPObj *obj, const char *volname, const char *args)
```

Defined at lines 778 to 844.

check host access list for volume

This function wants a string consisting of ip addresses or networks in CIDR notation seperated by comma or space.

Parameters:
* `obj`: [AFPObj](../../include/atalk/globals.h.md#struct-afpobj) containing client info
* `volname`: volume name (not used)
* `args`: access list string; NULL allows all hosts to have access

Returns: -1: no list

Returns: 0: list exists, but host isn't in it

Returns: 1: host is in list

Calls: [apply_ip_mask](socket.c.md#apply_ip_mask), [compare_ip](socket.c.md#compare_ip), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### netatalk_readbuf_clamp

```c
int netatalk_readbuf_clamp(int readbuf, uint32_t quantum, uint64_t limit)
```

Defined at lines 860 to 879.

Cap dsireadbuf so that dsireadbuf times quantum stays within limit.

The limit overrides the dsireadbuf floor of 6. The cap stays at 1 or more, so a quantum above the limit still gets 1. The call never shrinks the quantum, so read-ahead sizing leaves the frame size that the server accepts as it is. A quantum of 0 means no limit, and the config path never passes one.

Parameters:
* `readbuf`: number of quanta to read ahead
* `quantum`: server quantum in bytes
* `limit`: byte limit of the read-ahead buffer

Returns: readbuf, or the cap when readbuf exceeds it

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [dsi_tcp_open](../dsi/dsi_tcp.c.md#dsi_tcp_open)

### getoption_str

```c
static const char * getoption_str(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, const char *defval)
```

Defined at lines 892 to 909.

Get option string from config, use default value if not set.

Parameters:
* `conf`: config handle
* `vol`: volume name (must be section name i.e. wo vars expanded)
* `opt`: option
* `defsec`: if "option" is not found in "vol", try to find it in section "defsec"
* `defval`: if neither "vol" nor "defsec" contain "opt" return "defval"

Returns: const option string from "vol" or "defsec", or "defval" if not found

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [creatvol](netatalk_conf.c.md#creatvol), [dbpath_is_volume_root](netatalk_conf.c.md#dbpath_is_volume_root), [getoption_strdup_alias](netatalk_conf.c.md#getoption_strdup_alias), [getoption_uint32_strict](netatalk_conf.c.md#getoption_uint32_strict), [getvolbypath](netatalk_conf.c.md#getvolbypath), [private_option_warning](netatalk_conf.c.md#private_option_warning), [readvolfile](netatalk_conf.c.md#readvolfile), [user_state_paths](netatalk_conf.c.md#user_state_paths)

### getoption_strdup

```c
static char * getoption_strdup(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, const char *defval)
```

Defined at lines 924 to 945.

Get option string from config, use default value if not set.

Returns a dynamically allocated string which caller must free

Parameters:
* `conf`: config handle
* `vol`: volume name (must be section name i.e. wo vars expanded)
* `opt`: option
* `defsec`: if "option" is not found in "vol", try to find it in section "defsec"
* `defval`: if neither "vol" nor "defsec" contain "opt" return "defval"

Returns: dynamically allocated option string from "vol" or "defsec", or "defval" if not found

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### getoption_strdup_alias

```c
static char * getoption_strdup_alias(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *alias, const char *defsec, const char *defval, const char *alias_warn, bool *alias_used)
```

Defined at lines 966 to 995.

Get a string option with a deprecated alias.

The canonical option wins when it has a non-empty value. A non-empty alias is still reported even when the canonical option is also present, so stale deprecated settings are not silently hidden.

Parameters:
* `conf`: config handle
* `vol`: section name
* `opt`: canonical option
* `alias`: deprecated option name
* `defsec`: fallback section for both options
* `defval`: default value when neither option is set
* `alias_warn`: warning to emit when the alias is set
* `alias_used`: set when the returned value is from the alias; may be NULL

Returns: dynamically allocated option string, or NULL on allocation failure

Calls: [getoption_str](netatalk_conf.c.md#getoption_str)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### conf_parse_bool

```c
static int conf_parse_bool(const char *opt, const char *val)
```

Defined at lines 1009 to 1034.

Parse one boolean config value strictly.

Accepted spellings (case-insensitive, whole-word): true: yes, y, true, t, on, enabled, 1 false: no, n, false, f, off, disabled, 0

Parameters:
* `opt`: option name, for the warning
* `val`: raw config value (may be NULL or empty = unset)

Returns: 1 (true), 0 (false), or -1 (unset, or invalid with a warning)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [getoption_bool](netatalk_conf.c.md#getoption_bool), [vdgoption_bool](netatalk_conf.c.md#vdgoption_bool)

Mentioned in the documentation of: [conf_key_present](netatalk_conf.c.md#conf_key_present)

### conf_key_present

```c
static int conf_key_present(INIPARSER_DICTIONARY *conf, const char *opt)
```

Defined at lines 1053 to 1057.

Whether a [Global] key is present with a non-empty value.

The explicitness probe for the multi protocol defaults: "the operator said something" is recorded separately from the parsed value so it stays correct when compiled defaults change. Empty values ("key = ") count as unset, matching every parse in this file. Note the deliberate asymmetry with booleans: a boolean only counts as explicit when it parses ([conf_parse_bool()](netatalk_conf.c.md#conf_parse_bool) != -1, invalid = warned + unset), while int options count on presence because [safe_atoi()](netatalk_conf.c.md#safe_atoi) already resolved an invalid value to the default with its own warning.

Parameters:
* `conf`: config handle
* `opt`: option name

Returns: 1 when the key is set non-empty in [Global], else 0

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### conf_int_key_usable

```c
static int conf_int_key_usable(INIPARSER_DICTIONARY *conf, const char *opt, int min, int max)
```

Defined at lines 1073 to 1087.

Whether a [Global] int key holds a value within bounds.

Explicitness probe for bounded int options: a value that does not parse or falls outside [min, max] was replaced by the default, so it is not an operator instruction and must not suppress a coherency default.

Parameters:
* `conf`: config handle
* `opt`: option name
* `min`: lowest accepted value
* `max`: highest accepted value

Returns: 1 when the key is set to a value in [min, max], else 0

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### getoption_bool

```c
static int getoption_bool(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, int defval)
```

Defined at lines 1100 to 1118.

Get boolean option from config, use default value if not set.

Parameters:
* `conf`: config handle
* `vol`: volume name (must be section name i.e. wo vars expanded)
* `opt`: option
* `defsec`: if "option" is not found in "vol", try to find it in section "defsec"
* `defval`: if neither "vol" nor "defsec" contain "opt" return "defval"

Returns: const option string from "vol" or "defsec", or "defval" if not found

Calls: [conf_parse_bool](netatalk_conf.c.md#conf_parse_bool)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [creatvol](netatalk_conf.c.md#creatvol)

### safe_atoi

```c
int safe_atoi(const char *str, const char *param_name, int min_val, int max_val, int default_val)
```

Defined at lines 1131 to 1166.

Safe string to integer conversion with bounds checking.

Parameters:
* `str`: String to convert
* `param_name`: Parameter name for error messages
* `min_val`: Minimum allowed value
* `max_val`: Maximum allowed value
* `default_val`: Default value to return on error

Returns: Converted integer value or default_val on error

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [getoption_int](netatalk_conf.c.md#getoption_int)

Mentioned in the documentation of: [conf_key_present](netatalk_conf.c.md#conf_key_present), [getoption_int](netatalk_conf.c.md#getoption_int)

### getoption_int

```c
static int getoption_int(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, int defval)
```

Defined at lines 1182 to 1200.

Get integer option from config, use default value if not set.

Uses [safe_atoi()](netatalk_conf.c.md#safe_atoi) for bounds-checked string-to-integer conversion with overflow detection and non-numeric character rejection.

Parameters:
* `conf`: config handle
* `vol`: volume name (must be section name i.e. wo vars expanded)
* `opt`: option
* `defsec`: if "option" is not found in "vol", try to find it in section "defsec"
* `defval`: if neither "vol" nor "defsec" contain "opt" return "defval"

Returns: int option from "vol" or "defsec", or "defval" if not found

Calls: [safe_atoi](netatalk_conf.c.md#safe_atoi)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### getoption_uint32_strict

```c
static uint32_t getoption_uint32_strict(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, uint32_t minval, uint32_t maxval, uint32_t defval)
```

Defined at lines 1215 to 1266.

Get uint32 option from config, use default value if unset or invalid.

Parameters:
* `conf`: config handle
* `vol`: volume name (must be section name i.e. wo vars expanded)
* `opt`: option
* `defsec`: if "option" is not found in "vol", try to find it in section "defsec"
* `minval`: minimum accepted value
* `maxval`: maximum accepted value
* `defval`: if unset or out of range return "defval"

Returns: uint32 option from "vol" or "defsec", or "defval"

Calls: [getoption_str](netatalk_conf.c.md#getoption_str)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### vdgoption_bool

```c
static int vdgoption_bool(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, int defval)
```

Defined at lines 1284 to 1307.

Get boolean option from volume, default section or global - use default value if not set.

Order of precedence: volume -> default section -> global -> default value

"vdg" means volume, default section or global

Parameters:
* `conf`: config handle
* `vol`: volume name (must be section name i.e. wo vars expanded)
* `opt`: option
* `defsec`: if "option" is not found in "vol", try to find it in section "defsec"
* `defval`: if neither "vol", "defsec" nor global contain "opt" return "defval"

Returns: 1 or 0 from "vol", "defsec" or global, or "defval" if not found

Calls: [conf_parse_bool](netatalk_conf.c.md#conf_parse_bool)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### vdgoption_str

```c
static const char * vdgoption_str(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, const char *defval)
```

Defined at lines 1325 to 1345.

Get string option from volume, default section or global - use default value if not set.

Order of precedence: volume -> default section -> global -> default value

"vdg" means volume, default section or global

Parameters:
* `conf`: config handle
* `vol`: volume name (must be section name i.e. wo vars expanded)
* `opt`: option
* `defsec`: if "option" is not found in "vol", try to find it in section "defsec"
* `defval`: if neither "vol", "defsec" nor global contain "opt" return "defval"

Returns: const option string from "vol", "defsec" or global, or "defval" if not found

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### apply_multiprotocol_defaults

```c
static void apply_multiprotocol_defaults(AFPObj *obj, const char *volname)
```

Defined at lines 1360 to 1425.

Apply the multi protocol coherency defaults to the process options.

Defaults the settings safe concurrent access by other processes (Samba, NFS, local tools) requires: strict locking on, dircache validation on every access, rfork caching off, (Solaris) F_SHARE reservations on. Defaults only: an explicit afp.conf setting wins, with a warning when it weakens coherency. Idempotent across config reloads.

Parameters:
* `obj`: handle (options are process-global)
* `volname`: volume name, for the log messages

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### log_ea_format_advice

```c
static void log_ea_format_advice(const struct vol *volume, const char *ea_setting)
```

Defined at lines 1440 to 1463.

Log the storage-format implications of the coherency posture.

States which metadata format the volume uses, since only 'ea = samba' is readable by Samba, and points an ea = samba volume that has not declared multi protocol at the switch. The format is never changed implicitly: the other accessor may be NFS or a local process, and existing metadata is not converted in place.

Parameters:
* `volume`: the volume
* `ea_setting`: explicit non-samba ea value ("sys", "ad", "none"), NULL when unset (auto-detect)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### private_option_warning

```c
static void private_option_warning(const AFPObj *obj, const char *section, const char *name, const char *option, const char *value)
```

Defined at lines 1474 to 1486.

Log a private volume's own option that the volume does not take.

Parameters:
* `obj`: handle
* `section`: the volume's section
* `name`: the volume's name
* `option`: the option
* `value`: the value a private volume uses

Calls: [getoption_str](netatalk_conf.c.md#getoption_str)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### creatvol

```c
static struct vol * creatvol(AFPObj *obj, const struct passwd *pwd, const char *section, const char *name, const char *path_in, const char *preset)
```

Defined at lines 1499 to 2367.

Create volume struct.

Parameters:
* `obj`: handle
* `pwd`: struct passwd of logged in user, may be NULL in master afpd
* `section`: volume name wo variables expanded (exactly as in iniconfig)
* `name`: volume name
* `path_in`: volume path
* `preset`: default preset, may be NULL

Returns: vol on success, NULL on error

Calls: [accessvol](netatalk_conf.c.md#accessvol), [apply_multiprotocol_defaults](netatalk_conf.c.md#apply_multiprotocol_defaults), [become_root](unix.c.md#become_root), [check_ea_support](netatalk_conf.c.md#check_ea_support), [check_vol_acl_support](netatalk_conf.c.md#check_vol_acl_support), [convert_charset](../unicode/charcnv.c.md#convert_charset), [convert_string](../unicode/charcnv.c.md#convert_string), [get_vol_uuid](netatalk_conf.c.md#get_vol_uuid), [getoption_bool](netatalk_conf.c.md#getoption_bool), [getoption_str](netatalk_conf.c.md#getoption_str), [hostaccessvol](netatalk_conf.c.md#hostaccessvol), [initvol_vfs](../vfs/vfs.c.md#initvol_vfs), [log_ea_format_advice](netatalk_conf.c.md#log_ea_format_advice), [private_option_warning](netatalk_conf.c.md#private_option_warning), [strdup_w](../unicode/util_unistr.c.md#strdup_w), [strlcpy](../compat/strlcpy.c.md#strlcpy), [strnlen](../compat/misc.c.md#strnlen), [unbecome_root](unix.c.md#unbecome_root), [vdgoption_bool](netatalk_conf.c.md#vdgoption_bool), [vdgoption_str](netatalk_conf.c.md#vdgoption_str), [volume_free](netatalk_conf.c.md#volume_free), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [getvolbypath](netatalk_conf.c.md#getvolbypath), [readvolfile](netatalk_conf.c.md#readvolfile)

Uses file-scope variables: `Volumes`, `lastvid`

Mentioned in the documentation of: [conf_testutil_set_lastvid](netatalk_conf.c.md#conf_testutil_set_lastvid)

### volfile_changed

```c
static int volfile_changed(AFPObj *obj)
```

Defined at lines 2378 to 2401.

Tell whether afp.conf changed since the last check, by its mtime.

A newer mtime becomes the stored one. A failed stat also counts as a change, so the reload that follows fails as well.

Parameters:
* `obj`: AFP object with the config file path and the stored mtime

Returns: 1 for a change or a failed stat, 0 otherwise

Called by: [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols)

### vol_section

```c
static int vol_section(const char *sec)
```

Defined at lines 2409 to 2416.

Tell whether an afp.conf section defines a volume.

Parameters:
* `sec`: section name

Returns: 0 for the Global section, 1 for any other

Called by: [readvolfile](netatalk_conf.c.md#readvolfile)

### readvolfile

```c
static int readvolfile(AFPObj *obj, const struct passwd *pwent)
```

Defined at lines 2438 to 2582.

Create the volumes of the afp.conf sections and add them to the volume list.

Every section other than Global names a volume with a path and a volume name. Its preset falls back to the vol preset of Global. The call expands the variables of the path and then of the name. The [Homes] section gives the home of the user in an AFP session. It needs a user other than the guest, with a home that matches the basedir regex. The regex compiles once per process.

Every section that fails, apart from a skipped [Homes], adds 1 to vols_skipped of obj. The master reads the volumes too, for the Zeroconf announcements.

Parameters:
* `obj`: AFP object with the parsed afp.conf
* `pwent`: user entry of the session, or NULL

Returns: 0

Calls: [creatvol](netatalk_conf.c.md#creatvol), [getoption_str](netatalk_conf.c.md#getoption_str), [realpath_safe](unix.c.md#realpath_safe), [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy), [vol_section](netatalk_conf.c.md#vol_section), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols)

Uses file-scope variables: `have_uservol`

### setextmap

```c
static int setextmap(char *ext, char *type, char *creator)
```

Defined at lines 2600 to 2636.

Add an extension mapping to the unsorted extension map.

The call skips the leading dot of ext. A repeated extension keeps its first mapping, compared by strdiacasecmp without regard to case. The call stores the first 4 bytes of type and of creator, or 4 zeros for an empty one.

Parameters:
* `ext`: extension with its leading dot
* `type`: file type, up to 4 characters
* `creator`: creator code, up to 4 characters

Returns: 0, or -1 when an allocation fails

Calls: [strdiacasecmp](strdicasecmp.c.md#strdiacasecmp)

Called by: [readextmap](netatalk_conf.c.md#readextmap)

Uses file-scope variables: `Extmap`

### extmap_cmp

```c
static int extmap_cmp(const void *map1, const void *map2)
```

Defined at lines 2645 to 2650.

Order two extension map entries by extension, for qsort.

Parameters:
* `map1`: first entry
* `map2`: second entry

Returns: the strdiacasecmp result of the two extensions

Calls: [strdiacasecmp](strdicasecmp.c.md#strdiacasecmp)

Called by: [sortextmap](netatalk_conf.c.md#sortextmap)

### sortextmap

```c
static void sortextmap(void)
```

Defined at lines 2658 to 2682.

Count and sort the extension map, and make its "." line the default mapping.

The empty extension of a "." line sorts first.

Calls: [extmap_cmp](netatalk_conf.c.md#extmap_cmp)

Called by: [readextmap](netatalk_conf.c.md#readextmap)

Uses file-scope variables: `Defextmap`, `Extmap`, `Extmap_cnt`

### free_extmap

```c
static void free_extmap(void)
```

Defined at lines 2687 to 2701.

Free the extension map and clear the default mapping.

Called by: [afp_config_free](netatalk_conf.c.md#afp_config_free)

Uses file-scope variables: `Defextmap`, `Extmap`, `Extmap_cnt`

### ext_cmp_key

```c
static int ext_cmp_key(const void *key, const void *obj)
```

Defined at lines 2710 to 2715.

Compare an extension with an extension map entry, for bsearch.

Parameters:
* `key`: extension without its dot
* `obj`: map entry

Returns: the strdiacasecmp result of `key` and the entry extension

Calls: [strdiacasecmp](strdicasecmp.c.md#strdiacasecmp)

Called by: [getextmap](netatalk_conf.c.md#getextmap)

### getextmap

```c
struct extmap * getextmap(const char *)
```

Defined at lines 2728 to 2750. Declared in [etc/afpd/file.h](../../etc/afpd/file.h.md).

Return the extension mapping for a file name.

The extension follows the last dot and matches by strdiacasecmp without regard to case. A name without an extension, or one that the map lacks, gets the default mapping.

Parameters:
* `path`: file name

Returns: the mapping, or the default mapping, which is NULL without a "." line

Calls: [ext_cmp_key](netatalk_conf.c.md#ext_cmp_key)

Called by: [get_finderinfo](../../etc/afpd/file.c.md#get_finderinfo), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

Uses file-scope variables: `Defextmap`, `Extmap`, `Extmap_cnt`

### getdefextmap

```c
struct extmap * getdefextmap(void)
```

Defined at lines 2758 to 2761. Declared in [etc/afpd/file.h](../../etc/afpd/file.h.md).

Return the default extension mapping, the "." line of the extension map.

Returns: the mapping, or NULL without a "." line

Called by: [setfilparams](../../etc/afpd/file.c.md#setfilparams)

Uses file-scope variables: `Defextmap`

### readextmap

```c
static int readextmap(const char *file)
```

Defined at lines 2773 to 2804.

Read an extension map file and sort the map.

Each line whose first token starts with a dot maps that extension to a type and a creator, and the call ignores every other line. The "." line sets the default mapping.

Parameters:
* `file`: path of the extension map file

Returns: 0, or -1 when the file cannot open or close

Calls: [initline](gettok.c.md#initline), [parseline](gettok.c.md#parseline), [setextmap](netatalk_conf.c.md#setextmap), [sortextmap](netatalk_conf.c.md#sortextmap)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### volume_unlink

```c
void volume_unlink(struct vol *volume)
```

Defined at lines 2818 to 2837.

Remove a volume from the volume list.

Removing the first volume empties the list, which drops the other volumes too.

Parameters:
* `volume`: volume to remove

Uses file-scope variables: `Volumes`

### volume_free

```c
void volume_free(struct vol *vol)
```

Defined at lines 2847 to 2868.

Free a volume and the strings that loading it allocated.

Opening a volume with afp_openvol allocates more, and closevol frees those.

Parameters:
* `vol`: volume to free

Called by: [creatvol](netatalk_conf.c.md#creatvol), [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols), [unload_volumes](netatalk_conf.c.md#unload_volumes)

### load_charset

```c
int load_charset(struct vol *vol)
```

Defined at lines 2876 to 2891.

Load the Mac and volume charsets of a volume.

Parameters:
* `vol`: volume with the codepage names, receives the two charsets

Returns: 0, or -1 when a charset fails to load

Calls: [add_charset](../unicode/charcnv.c.md#add_charset)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional)

### load_afp_conf_vols

```c
int load_afp_conf_vols(AFPObj *obj, lv_flags_t flags)
```

Defined at lines 2914 to 3107.

Load the volumes of afp.conf, or reload them after a change.

In an AFP session that [login()](../../etc/afpd/auth.c.md#login) has not named a user for, the call loads nothing. After a read that parsed the file and dropped no volume, an unchanged mtime returns at once unless flags holds LV_FORCE. The call reads afp.conf as root under a read lock, with 1 retry after a second. A failed open or lock leaves the reload state untouched. A reload marks every volume deleted, creates the volumes again, and frees the marked volumes that are not open.

Without LV_ALL the volumes follow the session user, and a reload for a user other than root refreshes the groups of the user. With LV_ALL in an AFP session the call also rewrites the volume UUID file.

Parameters:
* `obj`: AFP object with the options and the session user
* `flags`: LV_ALL to skip the user lookup and the access checks, a private volume still following obj->uid, LV_FORCE to reload an unchanged file, or LV_DEFAULT

Returns: 0, or -1 on error

Calls: [become_root](unix.c.md#become_root), [readvolfile](netatalk_conf.c.md#readvolfile), [rewrite_vol_uuid_conf](netatalk_conf.c.md#rewrite_vol_uuid_conf), [set_groups](unix.c.md#set_groups), [unbecome_root](unix.c.md#unbecome_root), [volfile_changed](netatalk_conf.c.md#volfile_changed), [volume_free](netatalk_conf.c.md#volume_free)

Called by: [afp_getsrvrparms](../../etc/afpd/volume.c.md#afp_getsrvrparms), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [asp_process_deferred_signals](../../etc/afpd/afp_asp.c.md#asp_process_deferred_signals), [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../bin/nad/nad.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main), [sighup_impl](../../etc/netatalk/netatalk.c.md#sighup_impl)

Uses file-scope variables: `Volumes`, `have_uservol`

### unload_volumes

```c
void unload_volumes(AFPObj *obj)
```

Defined at lines 3117 to 3135.

Free every volume and reset the load state, so the next load reads afp.conf again.

The volume ID counter restarts at 0.

Parameters:
* `obj`: AFP object whose stored afp.conf mtime resets

Calls: [volume_free](netatalk_conf.c.md#volume_free)

Called by: [configfree](../../etc/afpd/afp_config.c.md#configfree)

Uses file-scope variables: `Volumes`, `have_uservol`, `lastvid`

Mentioned in the documentation of: [conf_testutil_set_lastvid](netatalk_conf.c.md#conf_testutil_set_lastvid)

### getvolumes

```c
struct vol * getvolumes(void)
```

Defined at lines 3143 to 3146.

Return the head of the volume list, which holds open and closed volumes.

Returns: the first volume, or NULL without volumes

Called by: [afp_getsrvrparms](../../etc/afpd/volume.c.md#afp_getsrvrparms), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [any_volume_uses_localsearch](../../etc/netatalk/netatalk.c.md#any_volume_uses_localsearch), [close_all_vol](../../etc/afpd/volume.c.md#close_all_vol), [flushvoltime](../../etc/afpd/volume.c.md#flushvoltime), [localsearch_backend_active](../../etc/afpd/spotlight.c.md#localsearch_backend_active), [markroot](../../etc/afpd/volume.c.md#markroot), [pollvoltime](../../etc/afpd/volume.c.md#pollvoltime), [process_cache_hints](../../etc/afpd/dircache.c.md#process_cache_hints), [register_stuff](../../etc/netatalk/afp_avahi.c.md#register_stuff), [register_stuff](../../etc/netatalk/afp_mdns.c.md#register_stuff), [server_ipc_volumes](../../etc/afpd/volume.c.md#server_ipc_volumes), [set_sl_volumes](../../etc/netatalk/netatalk.c.md#set_sl_volumes), [validate_singleuser_config](../../etc/netatalk/netatalk.c.md#validate_singleuser_config), [vol_setdate](../../etc/afpd/volume.c.md#vol_setdate)

Uses file-scope variables: `Volumes`

### getvolbyvid

```c
struct vol * getvolbyvid(const uint16_t vid)
```

Defined at lines 3154 to 3169.

Find an open volume by its volume ID.

Parameters:
* `vid`: volume ID

Returns: the volume, or NULL for an unknown ID or a volume that is not open

Called by: [afp_access](../../etc/afpd/acls.c.md#afp_access), [afp_addappl](../../etc/afpd/appl.c.md#afp_addappl), [afp_addcomment](../../etc/afpd/desktop.c.md#afp_addcomment), [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_closedir](../../etc/afpd/directory.c.md#afp_closedir), [afp_closedt](../../etc/afpd/desktop.c.md#afp_closedt), [afp_closevol](../../etc/afpd/volume.c.md#afp_closevol), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_createid](../../etc/afpd/file.c.md#afp_createid), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_flush](../../etc/afpd/fork.c.md#afp_flush), [afp_getacl](../../etc/afpd/acls.c.md#afp_getacl), [afp_getappl](../../etc/afpd/appl.c.md#afp_getappl), [afp_getcomment](../../etc/afpd/desktop.c.md#afp_getcomment), [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [afp_getfildirparams](../../etc/afpd/filedir.c.md#afp_getfildirparams), [afp_geticon](../../etc/afpd/desktop.c.md#afp_geticon), [afp_geticoninfo](../../etc/afpd/desktop.c.md#afp_geticoninfo), [afp_getvolparams](../../etc/afpd/volume.c.md#afp_getvolparams), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [afp_opendir](../../etc/afpd/directory.c.md#afp_opendir), [afp_opendt](../../etc/afpd/desktop.c.md#afp_opendt), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_rename](../../etc/afpd/filedir.c.md#afp_rename), [afp_resolveid](../../etc/afpd/file.c.md#afp_resolveid), [afp_rmvappl](../../etc/afpd/appl.c.md#afp_rmvappl), [afp_rmvcomment](../../etc/afpd/desktop.c.md#afp_rmvcomment), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [afp_setdirparams](../../etc/afpd/directory.c.md#afp_setdirparams), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [afp_setfildirparams](../../etc/afpd/filedir.c.md#afp_setfildirparams), [afp_setfilparams](../../etc/afpd/file.c.md#afp_setfilparams), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [afp_spotlight_rpc](../../etc/afpd/spotlight.c.md#afp_spotlight_rpc), [afp_syncdir](../../etc/afpd/directory.c.md#afp_syncdir), [catsearch_afp](../../etc/afpd/catsearch.c.md#catsearch_afp), [deferred_batch_chain](../../etc/afpd/dircache.c.md#deferred_batch_chain), [dircache_flush_deferred_for_vol](../../etc/afpd/dircache.c.md#dircache_flush_deferred_for_vol), [dircache_process_deferred_chain](../../etc/afpd/dircache.c.md#dircache_process_deferred_chain), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [process_cache_hints](../../etc/afpd/dircache.c.md#process_cache_hints), [sd_drop](../../etc/afpd/enumerate.c.md#sd_drop)

Uses file-scope variables: `Volumes`

### getuserbypath

```c
static char * getuserbypath(const char *path)
```

Defined at lines 3181 to 3228.

get username by path

[getvolbypath()](netatalk_conf.c.md#getvolbypath) assumes that the user home directory has the same name as the username. If that is not true, [getuserbypath()](netatalk_conf.c.md#getuserbypath) is called and tries to retrieve the username from the directory owner, checking its validity.

Parameters:
* `path`: absolute volume path

Returns: NULL if no match is found, pointer to username if successful

Calls: [realpath_safe](unix.c.md#realpath_safe)

Called by: [getvolbypath](netatalk_conf.c.md#getvolbypath)

### getvolbypath

```c
struct vol * getvolbypath(AFPObj *obj, const char *path)
```

Defined at lines 3258 to 3468.

Find the volume that holds a path, or create the home volume for it.

Only the user of an AFP session gets a [Homes] volume from load_afp_conf_vols. The dbd and nad tools find or create one here, by its path. A relative path starts at the current directory. The call strips trailing slashes, and the first volume in list order whose path holds the path matches.

The steps follow the numbered code comments:

1. Search the volume list.
1. Require a [Homes] section, which load_afp_conf_vols records.
1. Match the path against the basedir regex.
1. Take the matched part as the base of the home path.
1. Take the next path element as the user name, or the owner of that directory through getuserbypath for an unknown name.
1. Append the path option of [Homes].
1. Create the volume.

Step 5 reads the user name at the match length, counted from the path start, so the regex has to match at the start. The call sets the user name of obj and the groups of the process to the home user. The regex compiles once per process.

Parameters:
* `obj`: AFP object with the parsed afp.conf, receives the user name
* `path`: absolute or relative path

Returns: the volume, or NULL on error

Calls: [become_root](unix.c.md#become_root), [creatvol](netatalk_conf.c.md#creatvol), [getoption_str](netatalk_conf.c.md#getoption_str), [getuserbypath](netatalk_conf.c.md#getuserbypath), [realpath_safe](unix.c.md#realpath_safe), [set_groups](unix.c.md#set_groups), [strip_trailing_slashes](unix.c.md#strip_trailing_slashes), [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy), [unbecome_root](unix.c.md#unbecome_root), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional), [resolve_metadata_volume](../../bin/nad/megatron.c.md#resolve_metadata_volume)

Uses file-scope variables: `Volumes`, `have_uservol`

Mentioned in the documentation of: [getuserbypath](netatalk_conf.c.md#getuserbypath)

### getvolbyname

```c
struct vol * getvolbyname(const char *name)
```

Defined at lines 3479 to 3492.

Find the first volume whose section name is a prefix of name.

The comparison stops at the length of the section name, so a section named "Data" also matches "Data2".

Parameters:
* `name`: section name to look for

Returns: the volume, or NULL without a match

Uses file-scope variables: `Volumes`

### dbpath_is_volume_root

```c
static bool dbpath_is_volume_root(AFPObj *obj, const char *dbpath, const struct passwd *pwd)
```

Defined at lines 3507 to 3546.

Tell whether a directory is the path of a volume that afp.conf names.

A single-user server makes its [Global] vol dbpath owner-only, which would close such a share to everyone else. The call logs an error when the directory is a volume path. The [Homes] section and volume paths that do not exist do not count.

Parameters:
* `obj`: AFP object with the parsed afp.conf
* `dbpath`: directory to check
* `pwd`: user entry for the variables in the volume paths

Returns: true when the directory is the path of a volume

Calls: [getoption_str](netatalk_conf.c.md#getoption_str), [realpath_safe](unix.c.md#realpath_safe), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [user_state_paths](netatalk_conf.c.md#user_state_paths)

### user_state_paths

```c
static int user_state_paths(AFPObj *obj, bool singleuser)
```

Defined at lines 3569 to 3673.

Keep the signature and volume uuid files in the user's state.

A single-user server cannot write the system state directory, so its afp_signature.conf and afp_voluuid.conf live in the [Global] vol dbpath directory, which the server creates or makes mode 0700. Every other process takes the same paths when that directory is a user's owner-only directory and the caller is root or that user: the serving user's nad, which carries no single-user flag, and root's nad and dbd run on the user's configuration, so the uuid any of them generates is the one the server reads back, and a root service's own state keeps the built-in paths. A variable in the value cannot expand at parse time, before the session's user and the host name are known. The generate-once, read-back-later code that serves the root service then runs unchanged.

Parameters:
* `obj`: the object being configured; its options take the paths
* `singleuser`: the server's flag: the directory is required, created when absent and tightened when wider than 0700

Returns: 0, with both paths set when the directory qualifies; -1 (logged) when a single-user server has no usable directory

Calls: [dbpath_is_volume_root](netatalk_conf.c.md#dbpath_is_volume_root), [getoption_str](netatalk_conf.c.md#getoption_str), [strip_trailing_slashes](unix.c.md#strip_trailing_slashes), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### splice_size_round

```c
static int splice_size_round(int size)
```

Defined at lines 3683 to 3697.

Round size up to a power of two, as the kernel sizes a pipe.

The result is at least SPLICE_SIZE_MIN, and a rounded size logs a note.

Parameters:
* `size`: splice size from afp.conf

Returns: the rounded size

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### afp_config_parse

```c
int afp_config_parse(AFPObj *AFPObj, char *processname)
```

Defined at lines 3714 to 4420.

Read afp.conf into the options of an AFP object.

The call loads afp.conf as root from the command line path or the default path, and keeps the parsed file in iniconfig of the object. It sets up logging first, then reads the Global options, the charsets, and the extension map. Last it bounds the timing and buffer values.

Parameters:
* ``: AFP object to configure
* `processname`: name for log lines, or NULL to keep the current one

Returns: 0, or -1 when afp.conf cannot load, a single-user server has no usable vol dbpath, an allocation fails, or gethostname fails

Calls: [atalk_aton](atalk_addr.c.md#atalk_aton), [become_root](unix.c.md#become_root), [conf_int_key_usable](netatalk_conf.c.md#conf_int_key_usable), [conf_key_present](netatalk_conf.c.md#conf_key_present), [conf_parse_bool](netatalk_conf.c.md#conf_parse_bool), [getoption_bool](netatalk_conf.c.md#getoption_bool), [getoption_int](netatalk_conf.c.md#getoption_int), [getoption_str](netatalk_conf.c.md#getoption_str), [getoption_strdup](netatalk_conf.c.md#getoption_strdup), [getoption_strdup_alias](netatalk_conf.c.md#getoption_strdup_alias), [getoption_uint32_strict](netatalk_conf.c.md#getoption_uint32_strict), [netatalk_readbuf_clamp](netatalk_conf.c.md#netatalk_readbuf_clamp), [readextmap](netatalk_conf.c.md#readextmap), [safe_atoi](netatalk_conf.c.md#safe_atoi), [set_charset_name](../unicode/charcnv.c.md#set_charset_name), [set_processname](logger.c.md#set_processname), [setuplog](logger.c.md#setuplog), [splice_size_round](netatalk_conf.c.md#splice_size_round), [strip_trailing_slashes](unix.c.md#strip_trailing_slashes), [unbecome_root](unix.c.md#unbecome_root), [user_state_paths](netatalk_conf.c.md#user_state_paths)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../bin/nad/nad.c.md#main), [main](../../etc/afpd/main.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main)

### afp_config_free

```c
void afp_config_free(AFPObj *obj)
```

Defined at lines 4430 to 4590.

Free the options that afp_config_parse allocated, with the extension map and the parsed afp.conf.

The call also clears the option flags and the charset names.

Parameters:
* `obj`: AFP object to clear

Calls: [atalk_aton](atalk_addr.c.md#atalk_aton), [free_charset_names](../unicode/charcnv.c.md#free_charset_names), [free_extmap](netatalk_conf.c.md#free_extmap)

Called by: [main](../../etc/afpd/main.c.md#main)

# Macros

* Undocumented: `EABUFSZ`, `IS_VAR`, `MAXPRESETLEN`, `MAXVAL`, `UUID_PRINTABLE_STRING_LENGTH`, `VOLPASSLEN`

# File-scope variables

`Defextmap`, `Extmap`, `Extmap_cnt`, `UUID_FILE_HEADER`, `Volumes`, `have_uservol`, `lastvid`
