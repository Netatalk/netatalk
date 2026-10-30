---
type: Subsystem
title: "etc/afpd"
description: "66 files, 670 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/afpd"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-30T10:57:24+11:00 }
---

# Files

* [etc/afpd/acl_mappings.h](afpd/acl_mappings.h.md): Mapping tables between Darwin and NFSv4 ACL rights and flags.
* [etc/afpd/acls.c](afpd/acls.c.md): ACL support of afpd: FPAccess, FPGetACL, FPSetACL, and the Darwin ACE mapping.
* [etc/afpd/acls.h](afpd/acls.h.md): AFP ACL command handlers and access permission helpers.
* [etc/afpd/ad_cache.c](afpd/ad_cache.c.md): AppleDouble metadata and resource fork caching in the directory cache.
* [etc/afpd/ad_cache.h](afpd/ad_cache.h.md): AppleDouble metadata and resource fork cache interface.
* [etc/afpd/afp_asp.c](afpd/afp_asp.c.md): AFP sessions over ASP: signal handling and the request loop.
* [etc/afpd/afp_config.c](afpd/afp_config.c.md): Configuration setup of afpd at startup and reload: UAMs, listeners, and server-wide settings.
* [etc/afpd/afp_config.h](afpd/afp_config.h.md): AFP daemon configuration initialization and cleanup interface.
* [etc/afpd/afp_dsi.c](afpd/afp_dsi.c.md): AFP sessions over DSI: signal handling, session reconnects, and the request loop.
* [etc/afpd/afp_options.c](afpd/afp_options.c.md): Command line parsing and version output of afpd.
* [etc/afpd/afprun.c](afpd/afprun.c.md): Shell command execution as the session user for preexec, postexec, and the FCE notify script.
* [etc/afpd/afpstats.c](afpd/afpstats.c.md): The afpstats socket: session statistics for the afpstats command.
* [etc/afpd/afpstats.h](afpd/afpstats.h.md): AFP statistics service initialization and connection interface.
* [etc/afpd/appl.c](afpd/appl.c.md): The APPL desktop database: FPAddAPPL, FPRemoveAPPL, and FPGetAPPL.
* [etc/afpd/auth.c](afpd/auth.c.md): AFP login, logout, password, and session token commands, and UAM loading and dispatch.
* [etc/afpd/auth.h](afpd/auth.h.md): AFP authentication and session command handlers.
* [etc/afpd/catsearch.c](afpd/catsearch.c.md): FPCatSearch implementation.
* [etc/afpd/desktop.c](afpd/desktop.c.md): Manage the AppleDesktop folder and its contents.
* [etc/afpd/desktop.h](afpd/desktop.h.md): AFP desktop database command handlers and path conversion helpers.
* [etc/afpd/dircache.c](afpd/dircache.c.md): Directory Cache.
* [etc/afpd/dircache.h](afpd/dircache.h.md): Directory cache lookup, invalidation and maintenance interface.
* [etc/afpd/directory.c](afpd/directory.c.md): Directory resolution by CNID and path, the AFP directory commands, FPMapID, and FPMapName.
* [etc/afpd/directory.h](afpd/directory.h.md): AFP directory command handlers and path resolution helpers.
* [etc/afpd/enumerate.c](afpd/enumerate.c.md): Directory listing: FPEnumerate, FPEnumerateExt, and FPEnumerateExt2.
* [etc/afpd/extattrs.c](afpd/extattrs.c.md): The AFP extended attribute commands: FPListExtAttrs, FPGetExtAttr, FPSetExtAttr, and FPRemoveExtAttr.
* [etc/afpd/extattrs.h](afpd/extattrs.h.md): AFP extended attribute command handlers.
* [etc/afpd/fce_api.c](afpd/fce_api.c.md): File change event API for netatalk.
* [etc/afpd/fce_api_internal.h](afpd/fce_api_internal.h.md): Internal file change event packet, history and coalescing types.
* [etc/afpd/fce_util.c](afpd/fce_util.c.md): Event coalescing for the file change event (FCE) API.
* [etc/afpd/file.c](afpd/file.c.md): File parameters and the AFP file commands: FPCreateFile, FPCopyFile, FPExchangeFiles, and the file ID commands.
* [etc/afpd/file.h](afpd/file.h.md): AFP file command handlers and file metadata helpers.
* [etc/afpd/filedir.c](afpd/filedir.c.md): AFP commands for both files and directories: FPGetFileDirParms, FPSetFileDirParms, FPRename, FPDelete, and FPMoveAndRename.
* [etc/afpd/filedir.h](afpd/filedir.h.md): Shared AFP file and directory command handlers and name checks.
* [etc/afpd/fork.c](afpd/fork.c.md): Fork commands: FPOpenFork, FPRead, FPWrite, FPCloseFork, byte range locks, and the flush commands.
* [etc/afpd/fork.h](afpd/fork.h.md): Open fork state, byte range locking and AFP fork command handlers.
* [etc/afpd/hash.c](afpd/hash.c.md): Chained hash table for the directory cache.
* [etc/afpd/hash.h](afpd/hash.h.md): Hash table allocation, lookup and iteration interface for afpd.
* [etc/afpd/icon.c](afpd/icon.c.md): Default server icons for the FPGetSrvrInfo reply and the virtual Icon file.
* [etc/afpd/icon.h](afpd/icon.h.md): Desktop icon resource declarations, type codes and size constants.
* [etc/afpd/idle_worker.c](afpd/idle_worker.c.md): Idle worker thread that runs deferred directory cache work between requests.
* [etc/afpd/idle_worker.h](afpd/idle_worker.h.md): AFP session idle worker scheduling and lifecycle interface.
* [etc/afpd/main.c](afpd/main.c.md): The master afpd: startup, listeners, session children, and shutdown.
* [etc/afpd/mangle.c](afpd/mangle.c.md): mangle, demangle (filename)
* [etc/afpd/mangle.h](afpd/mangle.h.md): AFP filename mangling and demangling interface.
* [etc/afpd/messages.c](afpd/messages.c.md): FPGetSrvrMsg: the login message, and the server messages from the message file.
* [etc/afpd/misc.h](afpd/misc.h.md): AFP server message handler and unsupported command aliases.
* [etc/afpd/nfsquota.c](afpd/nfsquota.c.md): User quota of an NFS volume through rquotad.
* [etc/afpd/ofork.c](afpd/ofork.c.md): Open fork table of a session.
* [etc/afpd/pfd_cache.c](afpd/pfd_cache.c.md): Cache of parent directory descriptors for the stat calls that check directory cache entries.
* [etc/afpd/pfd_cache.h](afpd/pfd_cache.h.md): Parent directory descriptor cache and statistics interface.
* [etc/afpd/quota.c](afpd/quota.c.md): Free space under user and group quotas through libquota or quotactl.
* [etc/afpd/spotlight.c](afpd/spotlight.c.md): FPSpotlightRPC: Spotlight queries and attribute requests over the search backends.
* [etc/afpd/spotlight_marshalling.c](afpd/spotlight_marshalling.c.md): Packing and unpacking of Spotlight RPC requests and replies.
* [etc/afpd/spotlight_queries.c](afpd/spotlight_queries.c.md): Session-local Spotlight query ownership and deferred cancellation.
* [etc/afpd/status.c](afpd/status.c.md): The FPGetSrvrInfo status block.
* [etc/afpd/status.h](afpd/status.h.md): AFP server status construction and server information interface.
* [etc/afpd/switch.c](afpd/switch.c.md): AFP command dispatch tables of the session.
* [etc/afpd/switch.h](afpd/switch.h.md): AFP command dispatch tables and UAM handler replacement interface.
* [etc/afpd/uam.c](afpd/uam.c.md): UAM module loading and the server services that UAMs call.
* [etc/afpd/uam_auth.h](afpd/uam_auth.h.md): Authentication module types and loading interface for afpd.
* [etc/afpd/unix.c](afpd/unix.c.md): Unix modes, owners, and AFP access rights of files and directories.
* [etc/afpd/unix.h](afpd/unix.h.md): Unix permissions, ownership and quota interface for afpd.
* [etc/afpd/virtual_icon.c](afpd/virtual_icon.c.md): The virtual Icon file of a volume root.
* [etc/afpd/virtual_icon.h](afpd/virtual_icon.h.md): Synthetic volume icon lookup, metadata and resource fork interface.
* [etc/afpd/volume.c](afpd/volume.c.md): Volume commands: FPOpenVol, FPCloseVol, FPGetVolParms, and volume space.
* [etc/afpd/volume.h](afpd/volume.h.md): AFP volume command handlers and volume lifecycle helpers.

# Includes headers from

* [include/atalk](../include/atalk.md): 352 includes
* [etc/spotlight](spotlight.md): 3 includes
* [sys/netatalk](../sys/netatalk.md): 1 includes

# Headers included by

* [etc/spotlight](spotlight.md): 3 includes

# Calls into

* [libatalk/util](../libatalk/util.md): 278 calls
* [libatalk/adouble](../libatalk/adouble.md): 209 calls
* [include/atalk](../include/atalk.md): 63 calls
* [libatalk/compat](../libatalk/compat.md): 54 calls
* [libatalk/cnid](../libatalk/cnid.md): 48 calls
* [libatalk/dsi](../libatalk/dsi.md): 34 calls
* [libatalk/unicode](../libatalk/unicode.md): 34 calls
* [libatalk/vfs](../libatalk/vfs.md): 20 calls
* [libatalk/acl](../libatalk/acl.md): 17 calls
* [libatalk/asp](../libatalk/asp.md): 17 calls
* [libatalk/dalloc](../libatalk/dalloc.md): 14 calls
* [libatalk/atp](../libatalk/atp.md): 3 calls
* [libatalk/nbp](../libatalk/nbp.md): 3 calls

# Called from

* [etc/uams](uams.md): 77 calls
* [etc/spotlight](spotlight.md): 8 calls
* [bin/nad](../bin/nad.md): 1 calls

# Most called functions

* [dirlookup](afpd/directory.c.md#dirlookup): 39 callers
* [uam_afpserver_option](afpd/uam.c.md#uam_afpserver_option): 29 callers
* [cname](afpd/directory.c.md#cname): 28 callers
* [get_afp_errno](afpd/directory.c.md#get_afp_errno): 21 callers
* [of_findname](afpd/ofork.c.md#of_findname): 17 callers
* [dir_remove_tree](afpd/directory.c.md#dir_remove_tree): 16 callers
* [dircache_search_by_name](afpd/dircache.c.md#dircache_search_by_name): 15 callers
* [dirlookup_strict](afpd/directory.c.md#dirlookup_strict): 15 callers
* [movecwd](afpd/directory.c.md#movecwd): 15 callers
* [hnode_get](afpd/hash.c.md#hnode_get): 13 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[acl_from_mode](afpd/acls.c.md#acl_from_mode), [afp_null_nolog](afpd/auth.c.md#afp_null_nolog), [enumerate_stats_get](afpd/enumerate.c.md#enumerate_stats_get), [getnfsquota](afpd/nfsquota.c.md#getnfsquota), [hash_alloc_insert](afpd/hash.c.md#hash_alloc_insert), [hash_destroy](afpd/hash.c.md#hash_destroy), [hash_free_nodes](afpd/hash.c.md#hash_free_nodes), [hash_set_allocator](afpd/hash.c.md#hash_set_allocator), [hash_verify](afpd/hash.c.md#hash_verify), [hnode_getkey](afpd/hash.c.md#hnode_getkey), [offcnt_log_stats](afpd/directory.c.md#offcnt_log_stats), [offcnt_stats_get](afpd/directory.c.md#offcnt_stats_get), [pfd_stats_get](afpd/pfd_cache.c.md#pfd_stats_get), [sl_unpack](afpd/spotlight_marshalling.c.md#sl_unpack), [spotlight_get_cpx_qtype_string](afpd/spotlight_marshalling.c.md#spotlight_get_cpx_qtype_string), [spotlight_get_qtype_string](afpd/spotlight_marshalling.c.md#spotlight_get_qtype_string)
