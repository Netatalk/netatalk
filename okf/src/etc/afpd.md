---
type: Subsystem
title: "etc/afpd"
description: "66 files, 607 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/afpd"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T06:55:24+02:00 }
---

# Files

* [etc/afpd/acl_mappings.h](afpd/acl_mappings.h.md): Stuff for mapping between ACL implementations.
* [etc/afpd/acls.c](afpd/acls.c.md): ACL support of afpd: FPAccess, FPGetACL, FPSetACL, and the Darwin ACE mapping.
* [etc/afpd/acls.h](afpd/acls.h.md): Definitions for ACL mapping code.
* [etc/afpd/ad_cache.c](afpd/ad_cache.c.md): AppleDouble metadata and resource fork caching in the directory cache.
* [etc/afpd/ad_cache.h](afpd/ad_cache.h.md): includes 3 project headers.
* [etc/afpd/afp_asp.c](afpd/afp_asp.c.md): AFP sessions over ASP: signal handling and the request loop.
* [etc/afpd/afp_config.c](afpd/afp_config.c.md): Configuration setup of afpd at startup and reload: UAMs, listeners, and server-wide settings.
* [etc/afpd/afp_config.h](afpd/afp_config.h.md): includes 3 project headers.
* [etc/afpd/afp_dsi.c](afpd/afp_dsi.c.md): AFP sessions over DSI: signal handling, session reconnects, and the request loop.
* [etc/afpd/afp_options.c](afpd/afp_options.c.md): Command line parsing and version output of afpd.
* [etc/afpd/afprun.c](afpd/afprun.c.md): Shell command execution as the session user for preexec, postexec, and the FCE notify script.
* [etc/afpd/afpstats.c](afpd/afpstats.c.md): The afpstats socket: session statistics for the afpstats command.
* [etc/afpd/afpstats.h](afpd/afpstats.h.md): includes 1 project header.
* [etc/afpd/appl.c](afpd/appl.c.md): The APPL desktop database: FPAddAPPL, FPRemoveAPPL, and FPGetAPPL.
* [etc/afpd/auth.c](afpd/auth.c.md): AFP login, logout, password, and session token commands, and UAM loading and dispatch.
* [etc/afpd/auth.h](afpd/auth.h.md): 1 type, includes 1 project header.
* [etc/afpd/catsearch.c](afpd/catsearch.c.md): FPCatSearch implementation.
* [etc/afpd/desktop.c](afpd/desktop.c.md): Manage the AppleDesktop folder and its contents.
* [etc/afpd/desktop.h](afpd/desktop.h.md): 1 type, includes 2 project headers.
* [etc/afpd/dircache.c](afpd/dircache.c.md): Directory Cache.
* [etc/afpd/dircache.h](afpd/dircache.h.md): includes 3 project headers.
* [etc/afpd/directory.c](afpd/directory.c.md): Directory resolution by CNID and path, the AFP directory commands, FPMapID, and FPMapName.
* [etc/afpd/directory.h](afpd/directory.h.md): 1 function, 2 types, includes 4 project headers.
* [etc/afpd/enumerate.c](afpd/enumerate.c.md): Directory listing: FPEnumerate, FPEnumerateExt, and FPEnumerateExt2.
* [etc/afpd/extattrs.c](afpd/extattrs.c.md): The AFP extended attribute commands: FPListExtAttrs, FPGetExtAttr, FPSetExtAttr, and FPRemoveExtAttr.
* [etc/afpd/extattrs.h](afpd/extattrs.h.md): No functions or types.
* [etc/afpd/fce_api.c](afpd/fce_api.c.md): File change event API for netatalk.
* [etc/afpd/fce_api_internal.h](afpd/fce_api_internal.h.md): Internal definitions for File Change Events API.
* [etc/afpd/fce_util.c](afpd/fce_util.c.md): Event coalescing for the file change event (FCE) API.
* [etc/afpd/file.c](afpd/file.c.md): File parameters and the AFP file commands: FPCreateFile, FPCopyFile, FPExchangeFiles, and the file ID commands.
* [etc/afpd/file.h](afpd/file.h.md): includes 4 project headers.
* [etc/afpd/filedir.c](afpd/filedir.c.md): AFP commands for both files and directories: FPGetFileDirParms, FPSetFileDirParms, FPRename, FPDelete, and FPMoveAndRename.
* [etc/afpd/filedir.h](afpd/filedir.h.md): includes 2 project headers.
* [etc/afpd/fork.c](afpd/fork.c.md): Fork commands: FPOpenFork, FPRead, FPWrite, FPCloseFork, byte range locks, and the flush commands.
* [etc/afpd/fork.h](afpd/fork.h.md): 1 function, 2 types, includes 3 project headers.
* [etc/afpd/hash.c](afpd/hash.c.md): Chained hash table for the directory cache.
* [etc/afpd/hash.h](afpd/hash.h.md): 7 functions, includes 1 project header.
* [etc/afpd/icon.c](afpd/icon.c.md): Default server icons for the FPGetSrvrInfo reply and the virtual Icon file.
* [etc/afpd/icon.h](afpd/icon.h.md): No functions or types.
* [etc/afpd/idle_worker.c](afpd/idle_worker.c.md): Idle worker thread that runs deferred directory cache work between requests.
* [etc/afpd/idle_worker.h](afpd/idle_worker.h.md): No functions or types.
* [etc/afpd/main.c](afpd/main.c.md): The master afpd: startup, listeners, session children, and shutdown.
* [etc/afpd/mangle.c](afpd/mangle.c.md): mangle, demangle (filename)
* [etc/afpd/mangle.h](afpd/mangle.h.md): includes 6 project headers.
* [etc/afpd/messages.c](afpd/messages.c.md): FPGetSrvrMsg: the login message, and the server messages from the message file.
* [etc/afpd/misc.h](afpd/misc.h.md): includes 1 project header.
* [etc/afpd/nfsquota.c](afpd/nfsquota.c.md): User quota of an NFS volume through rquotad.
* [etc/afpd/ofork.c](afpd/ofork.c.md): Open fork table of a session.
* [etc/afpd/pfd_cache.c](afpd/pfd_cache.c.md): Cache of parent directory descriptors for the stat calls that check directory cache entries.
* [etc/afpd/pfd_cache.h](afpd/pfd_cache.h.md): 1 type, includes 2 project headers.
* [etc/afpd/quota.c](afpd/quota.c.md): Free space under user and group quotas through libquota or quotactl.
* [etc/afpd/spotlight.c](afpd/spotlight.c.md): FPSpotlightRPC: Spotlight queries and attribute requests over the search backends.
* [etc/afpd/spotlight_marshalling.c](afpd/spotlight_marshalling.c.md): Packing and unpacking of Spotlight RPC requests and replies.
* [etc/afpd/spotlight_queries.c](afpd/spotlight_queries.c.md): Session-local Spotlight query ownership and deferred cancellation.
* [etc/afpd/status.c](afpd/status.c.md): The FPGetSrvrInfo status block.
* [etc/afpd/status.h](afpd/status.h.md): includes 4 project headers.
* [etc/afpd/switch.c](afpd/switch.c.md): AFP command dispatch tables of the session.
* [etc/afpd/switch.h](afpd/switch.h.md): No functions or types.
* [etc/afpd/uam.c](afpd/uam.c.md): UAM module loading and the server services that UAMs call.
* [etc/afpd/uam_auth.h](afpd/uam_auth.h.md): interface between uam.c and auth.c
* [etc/afpd/unix.c](afpd/unix.c.md): Unix modes, owners, and AFP access rights of files and directories.
* [etc/afpd/unix.h](afpd/unix.h.md): 1 function, includes 1 project header.
* [etc/afpd/virtual_icon.c](afpd/virtual_icon.c.md): The virtual Icon file of a volume root.
* [etc/afpd/virtual_icon.h](afpd/virtual_icon.h.md): includes 2 project headers.
* [etc/afpd/volume.c](afpd/volume.c.md): Volume commands: FPOpenVol, FPCloseVol, FPGetVolParms, and volume space.
* [etc/afpd/volume.h](afpd/volume.h.md): includes 4 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 347 includes
* [etc/spotlight](spotlight.md): 3 includes
* [sys/netatalk](../sys/netatalk.md): 1 includes

# Headers included by

* [etc/spotlight](spotlight.md): 3 includes

# Calls into

* [libatalk/util](../libatalk/util.md): 259 calls
* [libatalk/adouble](../libatalk/adouble.md): 200 calls
* [libatalk/compat](../libatalk/compat.md): 59 calls
* [include/atalk](../include/atalk.md): 45 calls
* [libatalk/cnid](../libatalk/cnid.md): 39 calls
* [libatalk/unicode](../libatalk/unicode.md): 37 calls
* [libatalk/dsi](../libatalk/dsi.md): 35 calls
* [libatalk/vfs](../libatalk/vfs.md): 18 calls
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

* [dirlookup](afpd/directory.c.md#dirlookup): 40 callers
* [uam_afpserver_option](afpd/uam.c.md#uam_afpserver_option): 29 callers
* [cname](afpd/directory.c.md#cname): 28 callers
* [dircache_search_by_name](afpd/dircache.c.md#dircache_search_by_name): 23 callers
* [dir_remove](afpd/directory.c.md#dir_remove): 21 callers
* [get_afp_errno](afpd/directory.c.md#get_afp_errno): 21 callers
* [of_findname](afpd/ofork.c.md#of_findname): 18 callers
* [movecwd](afpd/directory.c.md#movecwd): 16 callers
* [dirlookup_strict](afpd/directory.c.md#dirlookup_strict): 15 callers
* [dir_modify](afpd/directory.c.md#dir_modify): 13 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[acl_from_mode](afpd/acls.c.md#acl_from_mode), [afp_null_nolog](afpd/auth.c.md#afp_null_nolog), [fce_cleanup](afpd/fce_api.c.md#fce_cleanup), [getnfsquota](afpd/nfsquota.c.md#getnfsquota), [hash_alloc_insert](afpd/hash.c.md#hash_alloc_insert), [hash_destroy](afpd/hash.c.md#hash_destroy), [hash_free_nodes](afpd/hash.c.md#hash_free_nodes), [hash_set_allocator](afpd/hash.c.md#hash_set_allocator), [hash_verify](afpd/hash.c.md#hash_verify), [hnode_getkey](afpd/hash.c.md#hnode_getkey), [pfd_stats_get](afpd/pfd_cache.c.md#pfd_stats_get), [sl_unpack](afpd/spotlight_marshalling.c.md#sl_unpack), [spotlight_get_cpx_qtype_string](afpd/spotlight_marshalling.c.md#spotlight_get_cpx_qtype_string), [spotlight_get_qtype_string](afpd/spotlight_marshalling.c.md#spotlight_get_qtype_string)
