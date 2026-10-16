---
type: Subsystem
title: "include/atalk"
description: "45 files, 32 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/include/atalk"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

# Files

* [include/atalk/acl.h](atalk/acl.h.md): Portable ACL retrieval, modification and permission helpers.
* [include/atalk/adouble.h](atalk/adouble.h.md): AppleDouble metadata, fork I/O and locking types and interface.
* [include/atalk/aep.h](atalk/aep.h.md): AppleTalk Echo Protocol operation codes.
* [include/atalk/afp.h](atalk/afp.h.md): Apple Filing Protocol command, capability and error constants.
* [include/atalk/afp_util.h](atalk/afp_util.h.md): AFP command and error name lookup interface.
* [include/atalk/asp.h](atalk/asp.h.md): AppleTalk Session Protocol state and session operations.
* [include/atalk/atp.h](atalk/atp.h.md): AppleTalk Transaction Protocol packet types and transaction interface.
* [include/atalk/bstrlib_compat.h](atalk/bstrlib_compat.h.md): Compatibility aliases for the bstrlib string interface.
* [include/atalk/byteorder.h](atalk/byteorder.h.md): Byte order macros for reading and writing packet fields.
* [include/atalk/cnid.h](atalk/cnid.h.md): CNID database types, backend operations and catalog lookup interface.
* [include/atalk/cnid_mysql_private.h](atalk/cnid_mysql_private.h.md): Private MySQL CNID backend state and status flags.
* [include/atalk/cnid_sqlite_private.h](atalk/cnid_sqlite_private.h.md): Private SQLite CNID backend state and status flags.
* [include/atalk/compat.h](atalk/compat.h.md): Portability declarations for missing system and C library functions.
* [include/atalk/constant_time.h](atalk/constant_time.h.md): Constant time memory comparison helper.
* [include/atalk/dalloc.h](atalk/dalloc.h.md): Typed talloc container types and value access interface.
* [include/atalk/ddp.h](atalk/ddp.h.md): Datagram Delivery Protocol type codes.
* [include/atalk/directory.h](atalk/directory.h.md): Shared AFP directory, path and directory cache entry types.
* [include/atalk/dsi.h](atalk/dsi.h.md): Data Stream Interface packet types, session state and I/O operations.
* [include/atalk/ea.h](atalk/ea.h.md): Portable extended attribute operations and AppleDouble EA types.
* [include/atalk/errchk.h](atalk/errchk.h.md): Error checking, logging and cleanup control macros.
* [include/atalk/fce_api.h](atalk/fce_api.h.md): File change event types, registration and notification interface.
* [include/atalk/globals.h](atalk/globals.h.md): AFP server options, session state and transport callback types.
* [include/atalk/hash.h](atalk/hash.h.md): Shared hash table node, allocator and callback types.
* [include/atalk/iniparser_util.h](atalk/iniparser_util.h.md): INI configuration access helpers and compatibility macros.
* [include/atalk/ldapconfig.h](atalk/ldapconfig.h.md): LDAP preference types and configuration lifecycle interface.
* [include/atalk/logger.h](atalk/logger.h.md): Logging levels, categories, configuration and message interface.
* [include/atalk/nbp.h](atalk/nbp.h.md): Name Binding Protocol packet types and name lookup interface.
* [include/atalk/netatalk_conf.h](atalk/netatalk_conf.h.md): Netatalk configuration and volume lookup interface.
* [include/atalk/netddp.h](atalk/netddp.h.md): Portable DDP socket lifecycle and datagram I/O interface.
* [include/atalk/pap.h](atalk/pap.h.md): Printer Access Protocol operation codes and packet sizes.
* [include/atalk/queue.h](atalk/queue.h.md): Queue node types and allocation, insertion and removal interface.
* [include/atalk/rpc.h](atalk/rpc.h.md): RPC locking interface for AppleDouble files.
* [include/atalk/rtmp.h](atalk/rtmp.h.md): Routing Table Maintenance Protocol packet types and operation codes.
* [include/atalk/server_child.h](atalk/server_child.h.md): AFP child process state and session management interface.
* [include/atalk/server_ipc.h](atalk/server_ipc.h.md): Parent and child IPC message types and cache hint interface.
* [include/atalk/spotlight.h](atalk/spotlight.h.md): Spotlight query types, backend callbacks and RPC interface.
* [include/atalk/srp.h](atalk/srp.h.md): SRP group constants and verifier validation helpers.
* [include/atalk/uam.h](atalk/uam.h.md): User authentication module registration and server callback interface.
* [include/atalk/unicode.h](atalk/unicode.h.md): Character set conversion types and Unicode string operations.
* [include/atalk/unix.h](atalk/unix.h.md): Unix filesystem, privilege and resource usage helpers.
* [include/atalk/util.h](atalk/util.h.md): Shared socket, signal, module loading and event loop helpers.
* [include/atalk/uuid.h](atalk/uuid.h.md): User and group UUID lookup, conversion and cache interface.
* [include/atalk/vfs.h](atalk/vfs.h.md): Volume filesystem operation callbacks and initialization interface.
* [include/atalk/volume.h](atalk/volume.h.md): AFP volume state, configuration flags and extension mapping types.
* [include/atalk/zip.h](atalk/zip.h.md): Zone Information Protocol packet types and operation codes.

# Includes headers from

* [sys/netatalk](../sys/netatalk.md): 7 includes

# Headers included by

* [etc/afpd](../etc/afpd.md): 347 includes
* [bin/nad](../bin/nad.md): 76 includes
* [libatalk/util](../libatalk/util.md): 69 includes
* [libatalk/vfs](../libatalk/vfs.md): 52 includes
* [etc/uams](../etc/uams.md): 51 includes
* [libatalk/adouble](../libatalk/adouble.md): 48 includes
* [etc/papd](../etc/papd.md): 40 includes
* [libatalk/dsi](../libatalk/dsi.md): 34 includes
* [etc/spotlight](../etc/spotlight.md): 32 includes
* [libatalk/unicode](../libatalk/unicode.md): 29 includes
* [etc/atalkd](../etc/atalkd.md): 27 includes
* [libatalk/asp](../libatalk/asp.md): 26 includes
* [etc/netatalk](../etc/netatalk.md): 25 includes
* [libatalk/acl](../libatalk/acl.md): 23 includes
* [libatalk/atp](../libatalk/atp.md): 22 includes
* [libatalk/cnid](../libatalk/cnid.md): 21 includes
* [bin/dbd](../bin/dbd.md): 16 includes
* [bin/nbp](../bin/nbp.md): 12 includes
* [libatalk/nbp](../libatalk/nbp.md): 10 includes
* [bin/pap](../bin/pap.md): 8 includes
* [bin/afppasswd](../bin/afppasswd.md): 6 includes
* [bin/aecho](../bin/aecho.md): 5 includes
* [bin/getzones](../bin/getzones.md): 5 includes
* [libatalk/dalloc](../libatalk/dalloc.md): 4 includes
* [bin/afpldaptest](../bin/afpldaptest.md): 3 includes
* [bin/rtmpqry](../bin/rtmpqry.md): 3 includes
* [bin/fce](../bin/fce.md): 2 includes
* [libatalk/compat](../libatalk/compat.md): 2 includes
* [sys/netatalk](../sys/netatalk.md): 2 includes
* [bin/netacnv](../bin/netacnv.md): 1 includes
* [libatalk/netddp](../libatalk/netddp.md): 1 includes

# Calls into

* [libatalk/compat](../libatalk/compat.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 45 calls
* [bin/afppasswd](../bin/afppasswd.md): 12 calls
* [libatalk/nbp](../libatalk/nbp.md): 9 calls
* [libatalk/adouble](../libatalk/adouble.md): 7 calls
* [libatalk/atp](../libatalk/atp.md): 7 calls
* [bin/getzones](../bin/getzones.md): 4 calls
* [bin/rtmpqry](../bin/rtmpqry.md): 4 calls
* [etc/papd](../etc/papd.md): 4 calls
* [bin/nad](../bin/nad.md): 3 calls
* [etc/uams](../etc/uams.md): 3 calls
* [bin/aecho](../bin/aecho.md): 1 calls
* [bin/nbp](../bin/nbp.md): 1 calls
* [etc/netatalk](../etc/netatalk.md): 1 calls

# Most called functions

* [path_isadir](atalk/directory.h.md#path_isadir): 18 callers
* [netddp_sendto](atalk/netddp.h.md#netddp_sendto): 12 callers
* [ad_meta_open](atalk/adouble.h.md#ad_meta_open): 10 callers
* [ad_rsrc_open](atalk/adouble.h.md#ad_rsrc_open): 8 callers
* [netddp_recvfrom](atalk/netddp.h.md#netddp_recvfrom): 8 callers
* [srp_valid_username](atalk/srp.h.md#srp_valid_username): 8 callers
* [netddp_close](atalk/netddp.h.md#netddp_close): 6 callers
* [srp_verifier_mode_is_safe](atalk/srp.h.md#srp_verifier_mode_is_safe): 5 callers
* [mod_close](atalk/util.h.md#mod_close): 4 callers
* [ad_meta_loaded](atalk/adouble.h.md#ad_meta_loaded): 3 callers
