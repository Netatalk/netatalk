---
type: C Source File
title: "libatalk/util/netatalk_conf.c"
description: "51 functions, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/netatalk_conf.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 103 to 106.

Test seam: set the volume-id counter.

Saturating it (UINT16_MAX) makes the next [creatvol()](netatalk_conf.c.md#creatvol) take the vid-overflow failure path. Lives here because the static it mutates does; declared in the test-local subtests_conf.h, not the public API.

Parameters:
* `vid`: new counter value ([unload_volumes()](netatalk_conf.c.md#unload_volumes) resets it to 0)

Uses file-scope variables: `lastvid`

### rewrite_vol_uuid_conf

```c
static int rewrite_vol_uuid_conf(AFPObj *obj, struct vol *Volumes)
```

Defined at lines 108 to 176.

Called by: [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols)

Uses file-scope variables: `UUID_FILE_HEADER`, `Volumes`

### get_vol_uuid

```c
static char * get_vol_uuid(const AFPObj *obj, const char *volname)
```

Defined at lines 185 to 331.

Get a volume's UUID from the config file.

If there is none, it is generated and stored there.

Returns: pointer to allocated storage on success, NULL on error.

Calls: [randombytes](unix.c.md#randombytes), [uuid_bin2string](../acl/uuid.c.md#uuid_bin2string)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

Uses file-scope variables: `UUID_FILE_HEADER`

### do_check_ea_support

```c
static int do_check_ea_support(const struct vol *vol)
```

Defined at lines 340 to 372.

Check if the underlying filesystem supports EAs.

If not, switch to ea=ad. As we can't check (requires write access) on ro-volumes, we assume ea=sys.

Calls: [become_root](unix.c.md#become_root), [sys_lgetxattr](../vfs/extattr.c.md#sys_lgetxattr), [sys_setxattr](../vfs/extattr.c.md#sys_setxattr), [unbecome_root](unix.c.md#unbecome_root)

Called by: [check_ea_support](netatalk_conf.c.md#check_ea_support)

### check_ea_support

```c
static void check_ea_support(struct vol *vol)
```

Defined at lines 374 to 392.

Calls: [do_check_ea_support](netatalk_conf.c.md#do_check_ea_support)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### check_vol_acl_support

```c
static int check_vol_acl_support(const struct vol *vol)
```

Defined at lines 401 to 439.

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

Defined at lines 471 to 654.

Handle variable substitutions.

here's what we understand:

```
$b   -> basename of path
$c   -> client ip/appletalk address
$d   -> volume pathname on server
$f   -> full name (whatever's in the gecos field)
$g   -> group
$h   -> hostname
$i   -> client ip/appletalk address without port
$s   -> server name (hostname if it doesn't exist)
$u   -> username (guest is usually nobody)
$v   -> volume name or basename if null
$$   -> $
```

This get's called from readvolfile with

```
path = NULL, volname = NULL for xlating the volumes path
path = path, volname = NULL for xlating the volumes name
... and from volumes options parsing code when xlating e.g. dbpath with
path = path, volname = volname
```

Using this information we can reject xlation of any variable depeninding on a login context which is not given in the afp master, where we must evaluate this whole stuff too for the Zeroconf announcements.

Calls: [getip_port](socket.c.md#getip_port), [getip_string](socket.c.md#getip_string), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [creatvol](netatalk_conf.c.md#creatvol), [dbpath_is_volume_root](netatalk_conf.c.md#dbpath_is_volume_root), [getvolbypath](netatalk_conf.c.md#getvolbypath), [readvolfile](netatalk_conf.c.md#readvolfile), [user_state_paths](netatalk_conf.c.md#user_state_paths)

### accessvol

```c
static int accessvol(const AFPObj *obj, const char *args, const char *name)
```

Defined at lines 676 to 714.

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

Defined at lines 730 to 796.

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

Defined at lines 806 to 825.

Bound dsireadbuf so dsireadbuf * quantum never exceeds limit. The limit always wins: on a constrained platform the result may drop below the usual dsireadbuf floor of 6, but never below 1. The quantum is never reduced here: frame acceptance must not change as a side effect of read-ahead sizing. A zero quantum cannot occur in the config path and is treated as "no limit".

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [dsi_tcp_open](../dsi/dsi_tcp.c.md#dsi_tcp_open)

### getoption_str

```c
static const char * getoption_str(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, const char *defval)
```

Defined at lines 838 to 855.

Get option string from config, use default value if not set.

Parameters:
* `conf`: config handle
* `vol`: volume name (must be section name i.e. wo vars expanded)
* `opt`: option
* `defsec`: if "option" is not found in "vol", try to find it in section "defsec"
* `defval`: if neither "vol" nor "defsec" contain "opt" return "defval"

Returns: const option string from "vol" or "defsec", or "defval" if not found

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [creatvol](netatalk_conf.c.md#creatvol), [dbpath_is_volume_root](netatalk_conf.c.md#dbpath_is_volume_root), [getoption_strdup_alias](netatalk_conf.c.md#getoption_strdup_alias), [getoption_uint32_strict](netatalk_conf.c.md#getoption_uint32_strict), [getvolbypath](netatalk_conf.c.md#getvolbypath), [readvolfile](netatalk_conf.c.md#readvolfile), [user_state_paths](netatalk_conf.c.md#user_state_paths)

### getoption_strdup

```c
static char * getoption_strdup(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, const char *defval)
```

Defined at lines 870 to 891.

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

Defined at lines 912 to 941.

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

Defined at lines 955 to 980.

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

Defined at lines 999 to 1003.

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

Defined at lines 1019 to 1033.

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

Defined at lines 1046 to 1064.

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

Defined at lines 1077 to 1112.

Safe string to integer conversion with bounds checking.

Parameters:
* `str`: String to convert
* `param_name`: Parameter name for error messages
* `min_val`: Minimum allowed value
* `max_val`: Maximum allowed value
* `default_val`: Default value to return on error

Returns: Converted integer value or default_val on error

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [configinit](../../etc/afpd/afp_config.c.md#configinit), [getoption_int](netatalk_conf.c.md#getoption_int)

Mentioned in the documentation of: [conf_key_present](netatalk_conf.c.md#conf_key_present), [getoption_int](netatalk_conf.c.md#getoption_int)

### getoption_int

```c
static int getoption_int(INIPARSER_DICTIONARY *conf, const char *vol, const char *opt, const char *defsec, int defval)
```

Defined at lines 1128 to 1146.

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

Defined at lines 1161 to 1212.

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

Defined at lines 1230 to 1253.

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

Defined at lines 1271 to 1291.

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

Defined at lines 1306 to 1371.

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

Defined at lines 1386 to 1409.

Log the storage-format implications of the coherency posture.

States which metadata format the volume uses, since only 'ea = samba' is readable by Samba, and points an ea = samba volume that has not declared multi protocol at the switch. The format is never changed implicitly: the other accessor may be NFS or a local process, and existing metadata is not converted in place.

Parameters:
* `volume`: the volume
* `ea_setting`: explicit non-samba ea value ("sys", "ad", "none"), NULL when unset (auto-detect)

Called by: [creatvol](netatalk_conf.c.md#creatvol)

### creatvol

```c
static struct vol * creatvol(AFPObj *obj, const struct passwd *pwd, const char *section, const char *name, const char *path_in, const char *preset)
```

Defined at lines 1422 to 2180.

Create volume struct.

Parameters:
* `obj`: handle
* `pwd`: struct passwd of logged in user, may be NULL in master afpd
* `section`: volume name wo variables expanded (exactly as in iniconfig)
* `name`: volume name
* `path_in`: volume path
* `preset`: default preset, may be NULL

Returns: vol on success, NULL on error

Calls: [accessvol](netatalk_conf.c.md#accessvol), [apply_multiprotocol_defaults](netatalk_conf.c.md#apply_multiprotocol_defaults), [become_root](unix.c.md#become_root), [check_ea_support](netatalk_conf.c.md#check_ea_support), [check_vol_acl_support](netatalk_conf.c.md#check_vol_acl_support), [convert_charset](../unicode/charcnv.c.md#convert_charset), [convert_string](../unicode/charcnv.c.md#convert_string), [get_vol_uuid](netatalk_conf.c.md#get_vol_uuid), [getoption_bool](netatalk_conf.c.md#getoption_bool), [getoption_str](netatalk_conf.c.md#getoption_str), [hostaccessvol](netatalk_conf.c.md#hostaccessvol), [initvol_vfs](../vfs/vfs.c.md#initvol_vfs), [log_ea_format_advice](netatalk_conf.c.md#log_ea_format_advice), [strdup_w](../unicode/util_unistr.c.md#strdup_w), [strlcpy](../compat/strlcpy.c.md#strlcpy), [strnlen](../compat/misc.c.md#strnlen), [unbecome_root](unix.c.md#unbecome_root), [vdgoption_bool](netatalk_conf.c.md#vdgoption_bool), [vdgoption_str](netatalk_conf.c.md#vdgoption_str), [volume_free](netatalk_conf.c.md#volume_free), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [getvolbypath](netatalk_conf.c.md#getvolbypath), [readvolfile](netatalk_conf.c.md#readvolfile)

Uses file-scope variables: `Volumes`, `lastvid`

Mentioned in the documentation of: [conf_testutil_set_lastvid](netatalk_conf.c.md#conf_testutil_set_lastvid)

### volfile_changed

```c
static int volfile_changed(AFPObj *obj)
```

Defined at lines 2184 to 2207.

Called by: [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols)

### vol_section

```c
static int vol_section(const char *sec)
```

Defined at lines 2209 to 2216.

Called by: [readvolfile](netatalk_conf.c.md#readvolfile)

### readvolfile

```c
static int readvolfile(AFPObj *obj, const struct passwd *pwent)
```

Defined at lines 2226 to 2370.

Read volumes from iniconfig and add the volumes contained within to the global volume list.

This gets called from the forked afpd childs. The master now reads this too for Zeroconf announcements.

Calls: [creatvol](netatalk_conf.c.md#creatvol), [getoption_str](netatalk_conf.c.md#getoption_str), [realpath_safe](unix.c.md#realpath_safe), [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy), [vol_section](netatalk_conf.c.md#vol_section), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols)

Uses file-scope variables: `have_uservol`

### setextmap

```c
static int setextmap(char *ext, char *type, char *creator)
```

Defined at lines 2375 to 2411.

Calls: [strdiacasecmp](strdicasecmp.c.md#strdiacasecmp)

Called by: [readextmap](netatalk_conf.c.md#readextmap)

Uses file-scope variables: `Extmap`

### extmap_cmp

```c
static int extmap_cmp(const void *map1, const void *map2)
```

Defined at lines 2414 to 2419.

Calls: [strdiacasecmp](strdicasecmp.c.md#strdiacasecmp)

Called by: [sortextmap](netatalk_conf.c.md#sortextmap)

### sortextmap

```c
static void sortextmap(void)
```

Defined at lines 2421 to 2445.

Calls: [extmap_cmp](netatalk_conf.c.md#extmap_cmp)

Called by: [readextmap](netatalk_conf.c.md#readextmap)

Uses file-scope variables: `Defextmap`, `Extmap`, `Extmap_cnt`

### free_extmap

```c
static void free_extmap(void)
```

Defined at lines 2447 to 2461.

Called by: [afp_config_free](netatalk_conf.c.md#afp_config_free)

Uses file-scope variables: `Defextmap`, `Extmap`, `Extmap_cnt`

### ext_cmp_key

```c
static int ext_cmp_key(const void *key, const void *obj)
```

Defined at lines 2463 to 2468.

Calls: [strdiacasecmp](strdicasecmp.c.md#strdiacasecmp)

Called by: [getextmap](netatalk_conf.c.md#getextmap)

### getextmap

```c
struct extmap * getextmap(const char *)
```

Defined at lines 2470 to 2492. Declared in [etc/afpd/file.h](../../etc/afpd/file.h.md).

Calls: [ext_cmp_key](netatalk_conf.c.md#ext_cmp_key)

Called by: [get_finderinfo](../../etc/afpd/file.c.md#get_finderinfo), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

Uses file-scope variables: `Defextmap`, `Extmap`, `Extmap_cnt`

### getdefextmap

```c
struct extmap * getdefextmap(void)
```

Defined at lines 2494 to 2497. Declared in [etc/afpd/file.h](../../etc/afpd/file.h.md).

Called by: [setfilparams](../../etc/afpd/file.c.md#setfilparams)

Uses file-scope variables: `Defextmap`

### readextmap

```c
static int readextmap(const char *file)
```

Defined at lines 2499 to 2530.

Calls: [initline](gettok.c.md#initline), [parseline](gettok.c.md#parseline), [setextmap](netatalk_conf.c.md#setextmap), [sortextmap](netatalk_conf.c.md#sortextmap)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### volume_unlink

```c
void volume_unlink(struct vol *volume)
```

Defined at lines 2539 to 2558.

Remove a volume from the linked list of volumes

Uses file-scope variables: `Volumes`

### volume_free

```c
void volume_free(struct vol *vol)
```

Defined at lines 2566 to 2587.

Free all resources allocated in a struct vol in [load_afp_conf_vols()](netatalk_conf.c.md#load_afp_conf_vols)

Actually opening a volume ([afp_openvol()](../../etc/afpd/volume.c.md#afp_openvol)) will allocate additional resources which are freed in [closevol()](../../bin/nad/nad_util.c.md#closevol)

Called by: [creatvol](netatalk_conf.c.md#creatvol), [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols), [unload_volumes](netatalk_conf.c.md#unload_volumes)

### load_charset

```c
int load_charset(struct vol *vol)
```

Defined at lines 2592 to 2607.

Load charsets for a volume

Calls: [add_charset](../unicode/charcnv.c.md#add_charset)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional)

### load_afp_conf_vols

```c
int load_afp_conf_vols(AFPObj *obj, lv_flags_t flags)
```

Defined at lines 2618 to 2790.

Initialize volumes and load ini configfile.

Parameters:
* `obj`: handle
* `flags`: flags controlling volume load behaviour: * LV_DEFAULT: load shares in a user/session context, this honors authorization
* LV_ALL: load shares that are available in the config file
* LV_FORCE: reload file even though the timestamp wasn't changed

Calls: [become_root](unix.c.md#become_root), [readvolfile](netatalk_conf.c.md#readvolfile), [rewrite_vol_uuid_conf](netatalk_conf.c.md#rewrite_vol_uuid_conf), [set_groups](unix.c.md#set_groups), [unbecome_root](unix.c.md#unbecome_root), [volfile_changed](netatalk_conf.c.md#volfile_changed), [volume_free](netatalk_conf.c.md#volume_free)

Called by: [afp_getsrvrparms](../../etc/afpd/volume.c.md#afp_getsrvrparms), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [asp_process_deferred_signals](../../etc/afpd/afp_asp.c.md#asp_process_deferred_signals), [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../bin/nad/nad.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main), [sighup_impl](../../etc/netatalk/netatalk.c.md#sighup_impl)

Uses file-scope variables: `Volumes`, `have_uservol`

Mentioned in the documentation of: [getvolbypath](netatalk_conf.c.md#getvolbypath), [volume_free](netatalk_conf.c.md#volume_free)

### unload_volumes

```c
void unload_volumes(AFPObj *obj)
```

Defined at lines 2792 to 2809.

Calls: [volume_free](netatalk_conf.c.md#volume_free)

Called by: [configfree](../../etc/afpd/afp_config.c.md#configfree)

Uses file-scope variables: `Volumes`, `have_uservol`, `lastvid`

Mentioned in the documentation of: [conf_testutil_set_lastvid](netatalk_conf.c.md#conf_testutil_set_lastvid)

### getvolumes

```c
struct vol * getvolumes(void)
```

Defined at lines 2811 to 2814.

Called by: [afp_getsrvrparms](../../etc/afpd/volume.c.md#afp_getsrvrparms), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [close_all_vol](../../etc/afpd/volume.c.md#close_all_vol), [pollvoltime](../../etc/afpd/volume.c.md#pollvoltime), [process_cache_hints](../../etc/afpd/dircache.c.md#process_cache_hints), [register_stuff](../../etc/netatalk/afp_avahi.c.md#register_stuff), [register_stuff](../../etc/netatalk/afp_mdns.c.md#register_stuff), [server_ipc_volumes](../../etc/afpd/volume.c.md#server_ipc_volumes), [validate_singleuser_config](../../etc/netatalk/netatalk.c.md#validate_singleuser_config), [vol_setdate](../../etc/afpd/volume.c.md#vol_setdate)

Uses file-scope variables: `Volumes`

### getvolbyvid

```c
struct vol * getvolbyvid(const uint16_t vid)
```

Defined at lines 2816 to 2831.

Called by: [afp_access](../../etc/afpd/acls.c.md#afp_access), [afp_addappl](../../etc/afpd/appl.c.md#afp_addappl), [afp_addcomment](../../etc/afpd/desktop.c.md#afp_addcomment), [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_closedir](../../etc/afpd/directory.c.md#afp_closedir), [afp_closedt](../../etc/afpd/desktop.c.md#afp_closedt), [afp_closevol](../../etc/afpd/volume.c.md#afp_closevol), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_createid](../../etc/afpd/file.c.md#afp_createid), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_flush](../../etc/afpd/fork.c.md#afp_flush), [afp_getacl](../../etc/afpd/acls.c.md#afp_getacl), [afp_getappl](../../etc/afpd/appl.c.md#afp_getappl), [afp_getcomment](../../etc/afpd/desktop.c.md#afp_getcomment), [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [afp_getfildirparams](../../etc/afpd/filedir.c.md#afp_getfildirparams), [afp_geticon](../../etc/afpd/desktop.c.md#afp_geticon), [afp_geticoninfo](../../etc/afpd/desktop.c.md#afp_geticoninfo), [afp_getvolparams](../../etc/afpd/volume.c.md#afp_getvolparams), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [afp_opendir](../../etc/afpd/directory.c.md#afp_opendir), [afp_opendt](../../etc/afpd/desktop.c.md#afp_opendt), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_rename](../../etc/afpd/filedir.c.md#afp_rename), [afp_resolveid](../../etc/afpd/file.c.md#afp_resolveid), [afp_rmvappl](../../etc/afpd/appl.c.md#afp_rmvappl), [afp_rmvcomment](../../etc/afpd/desktop.c.md#afp_rmvcomment), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [afp_setdirparams](../../etc/afpd/directory.c.md#afp_setdirparams), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [afp_setfildirparams](../../etc/afpd/filedir.c.md#afp_setfildirparams), [afp_setfilparams](../../etc/afpd/file.c.md#afp_setfilparams), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [afp_spotlight_rpc](../../etc/afpd/spotlight.c.md#afp_spotlight_rpc), [afp_syncdir](../../etc/afpd/directory.c.md#afp_syncdir), [catsearch_afp](../../etc/afpd/catsearch.c.md#catsearch_afp), [deferred_batch_chain](../../etc/afpd/dircache.c.md#deferred_batch_chain), [dircache_flush_deferred_for_vol](../../etc/afpd/dircache.c.md#dircache_flush_deferred_for_vol), [dircache_process_deferred_chain](../../etc/afpd/dircache.c.md#dircache_process_deferred_chain), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [process_cache_hints](../../etc/afpd/dircache.c.md#process_cache_hints)

Uses file-scope variables: `Volumes`

### getuserbypath

```c
static char * getuserbypath(const char *path)
```

Defined at lines 2843 to 2890.

get username by path

[getvolbypath()](netatalk_conf.c.md#getvolbypath) assumes that the user home directory has the same name as the username. If that is not true, [getuserbypath()](netatalk_conf.c.md#getuserbypath) is called and tries to retrieve the username from the directory owner, checking its validity.

Parameters:
* `path`: absolute volume path

Returns: NULL if no match is found, pointer to username if successful

Calls: [realpath_safe](unix.c.md#realpath_safe)

Called by: [getvolbypath](netatalk_conf.c.md#getvolbypath)

Mentioned in the documentation of: [getvolbypath](netatalk_conf.c.md#getvolbypath)

### getvolbypath

```c
struct vol * getvolbypath(AFPObj *obj, const char *path)
```

Defined at lines 2915 to 3134.

Search volume by path, creating user home vols as necessary.

Path may be absolute or relative. Ordinary volume structs are created when the ini config is initially parsed ([load_afp_conf_vols()](netatalk_conf.c.md#load_afp_conf_vols)), but user volumes are as [load_afp_conf_vols()](netatalk_conf.c.md#load_afp_conf_vols) only can create the user volume of the logged in user in an AFP session in afpd, but not when called from e.g. dbd. dbd thus needs a way to lookup and create struct vols for user home by path. This is what this func does as well.

1. Search "normal" volume list
1. Check if theres a [Homes] section, [load_afp_conf_vols()](netatalk_conf.c.md#load_afp_conf_vols) remembers this for us
1. If there is, match "path" with "basedir regex" to get the user home parent dir
1. Built user home path by appending the basedir matched in (3) and appending the username
1. The next path element then is the username * [getvolbypath()](netatalk_conf.c.md#getvolbypath) assumes that the user home directory has the same name as the username. If that is not true, [getuserbypath()](netatalk_conf.c.md#getuserbypath) is called and tries to retrieve the username from the directory owner, checking its validity
1. Append [Homes]->path subdirectory if defined
1. Create volume

Parameters:
* `obj`: handle
* `path`: path, may be relative or absolute

Calls: [become_root](unix.c.md#become_root), [creatvol](netatalk_conf.c.md#creatvol), [getoption_str](netatalk_conf.c.md#getoption_str), [getuserbypath](netatalk_conf.c.md#getuserbypath), [realpath_safe](unix.c.md#realpath_safe), [set_groups](unix.c.md#set_groups), [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy), [unbecome_root](unix.c.md#unbecome_root), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional), [resolve_metadata_volume](../../bin/nad/megatron.c.md#resolve_metadata_volume)

Uses file-scope variables: `Volumes`, `have_uservol`

Mentioned in the documentation of: [getuserbypath](netatalk_conf.c.md#getuserbypath)

### getvolbyname

```c
struct vol * getvolbyname(const char *name)
```

Defined at lines 3136 to 3149.

Uses file-scope variables: `Volumes`

### strip_trailing_slashes

```c
static void strip_trailing_slashes(char *path)
```

Defined at lines 3155 to 3162.

Drop trailing slashes, which make lstat() and an O_NOFOLLOW open follow a final symbolic link.

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [user_state_paths](netatalk_conf.c.md#user_state_paths)

### dbpath_is_volume_root

```c
static bool dbpath_is_volume_root(AFPObj *obj, const char *dbpath, const struct passwd *pwd)
```

Defined at lines 3170 to 3209.

Whether a directory is the path of a volume the configuration names.

A single-user server makes its [Global] vol dbpath owner-only, which would close such a share to everyone else; logged when it is one.

Calls: [getoption_str](netatalk_conf.c.md#getoption_str), [realpath_safe](unix.c.md#realpath_safe), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [user_state_paths](netatalk_conf.c.md#user_state_paths)

### user_state_paths

```c
static int user_state_paths(AFPObj *obj, bool singleuser)
```

Defined at lines 3233 to 3337.

Keep the signature and volume uuid files in the user's state.

A single-user server cannot write the system state directory, so its afp_signature.conf and afp_voluuid.conf live in the [Global] vol dbpath directory, the parent of its CNID directories, which the server creates or makes mode 0700 as it does the CNID directories below it. Every other process takes the same paths when that directory is a user's owner-only directory and the caller is root or that user: the serving user's nad and dbd, which carry no single-user flag, and root's tools run on the user's configuration, so the uuid any of them generates is the one the server reads back, and a root service's own state keeps the built-in paths. A variable in the value cannot expand at parse time, before the session's user and the host name are known. The generate-once, read-back-later code that serves the root service then runs unchanged.

Parameters:
* `obj`: the object being configured; its options take the paths
* `singleuser`: the server's flag: the directory is required, created when absent and tightened when wider than 0700

Returns: 0, with both paths set when the directory qualifies; -1 (logged) when a single-user server has no usable directory

Calls: [dbpath_is_volume_root](netatalk_conf.c.md#dbpath_is_volume_root), [getoption_str](netatalk_conf.c.md#getoption_str), [strip_trailing_slashes](netatalk_conf.c.md#strip_trailing_slashes), [volxlate](netatalk_conf.c.md#volxlate)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse)

### afp_config_parse

```c
int afp_config_parse(AFPObj *AFPObj, char *processname)
```

Defined at lines 3343 to 4029.

Initialize an [AFPObj](../../include/atalk/globals.h.md#struct-afpobj) and options from ini config file

Calls: [atalk_aton](atalk_addr.c.md#atalk_aton), [become_root](unix.c.md#become_root), [conf_int_key_usable](netatalk_conf.c.md#conf_int_key_usable), [conf_key_present](netatalk_conf.c.md#conf_key_present), [conf_parse_bool](netatalk_conf.c.md#conf_parse_bool), [getoption_bool](netatalk_conf.c.md#getoption_bool), [getoption_int](netatalk_conf.c.md#getoption_int), [getoption_str](netatalk_conf.c.md#getoption_str), [getoption_strdup](netatalk_conf.c.md#getoption_strdup), [getoption_strdup_alias](netatalk_conf.c.md#getoption_strdup_alias), [getoption_uint32_strict](netatalk_conf.c.md#getoption_uint32_strict), [netatalk_readbuf_clamp](netatalk_conf.c.md#netatalk_readbuf_clamp), [readextmap](netatalk_conf.c.md#readextmap), [safe_atoi](netatalk_conf.c.md#safe_atoi), [set_charset_name](../unicode/charcnv.c.md#set_charset_name), [set_processname](logger.c.md#set_processname), [setuplog](logger.c.md#setuplog), [strip_trailing_slashes](netatalk_conf.c.md#strip_trailing_slashes), [unbecome_root](unix.c.md#unbecome_root), [user_state_paths](netatalk_conf.c.md#user_state_paths)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../bin/nad/nad.c.md#main), [main](../../etc/afpd/main.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main)

### afp_config_free

```c
void afp_config_free(AFPObj *obj)
```

Defined at lines 4032 to 4176.

get rid of any allocated afp_option buffers.

Calls: [atalk_aton](atalk_addr.c.md#atalk_aton), [free_charset_names](../unicode/charcnv.c.md#free_charset_names), [free_extmap](netatalk_conf.c.md#free_extmap)

Called by: [main](../../etc/afpd/main.c.md#main)

# Macros

* Undocumented: `EABUFSZ`, `IS_VAR`, `MAXPRESETLEN`, `MAXVAL`, `UUID_PRINTABLE_STRING_LENGTH`, `VOLPASSLEN`

# File-scope variables

`Defextmap`, `Extmap`, `Extmap_cnt`, `UUID_FILE_HEADER`, `Volumes`, `have_uservol`, `lastvid`
