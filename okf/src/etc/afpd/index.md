# src/etc/afpd

* [acl_mappings.h](acl_mappings.h.md) - Mapping tables between Darwin and NFSv4 ACL rights and flags.
* [acls.c](acls.c.md) - ACL support of afpd: FPAccess, FPGetACL, FPSetACL, and the Darwin ACE mapping.
* [acls.h](acls.h.md) - AFP ACL command handlers and access permission helpers.
* [ad_cache.c](ad_cache.c.md) - AppleDouble metadata and resource fork caching in the directory cache.
* [ad_cache.h](ad_cache.h.md) - AppleDouble metadata and resource fork cache interface.
* [afp_asp.c](afp_asp.c.md) - AFP sessions over ASP: signal handling and the request loop.
* [afp_config.c](afp_config.c.md) - Configuration setup of afpd at startup and reload: UAMs, listeners, and server-wide settings.
* [afp_config.h](afp_config.h.md) - AFP daemon configuration initialization and cleanup interface.
* [afp_dsi.c](afp_dsi.c.md) - AFP sessions over DSI: signal handling, session reconnects, and the request loop.
* [afp_options.c](afp_options.c.md) - Command line parsing and version output of afpd.
* [afprun.c](afprun.c.md) - Shell command execution as the session user for preexec, postexec, and the FCE notify script.
* [afpstats.c](afpstats.c.md) - The afpstats socket: session statistics for the afpstats command.
* [afpstats.h](afpstats.h.md) - AFP statistics service initialization and connection interface.
* [appl.c](appl.c.md) - The APPL desktop database: FPAddAPPL, FPRemoveAPPL, and FPGetAPPL.
* [auth.c](auth.c.md) - AFP login, logout, password, and session token commands, and UAM loading and dispatch.
* [auth.h](auth.h.md) - AFP authentication and session command handlers.
* [catsearch.c](catsearch.c.md) - FPCatSearch implementation.
* [desktop.c](desktop.c.md) - Manage the AppleDesktop folder and its contents.
* [desktop.h](desktop.h.md) - AFP desktop database command handlers and path conversion helpers.
* [dircache.c](dircache.c.md) - Directory Cache.
* [dircache.h](dircache.h.md) - Directory cache lookup, invalidation and maintenance interface.
* [directory.c](directory.c.md) - Directory resolution by CNID and path, the AFP directory commands, FPMapID, and FPMapName.
* [directory.h](directory.h.md) - AFP directory command handlers and path resolution helpers.
* [enumerate.c](enumerate.c.md) - Directory listing: FPEnumerate, FPEnumerateExt, and FPEnumerateExt2.
* [extattrs.c](extattrs.c.md) - The AFP extended attribute commands: FPListExtAttrs, FPGetExtAttr, FPSetExtAttr, and FPRemoveExtAttr.
* [extattrs.h](extattrs.h.md) - AFP extended attribute command handlers.
* [fce_api.c](fce_api.c.md) - File change event API for netatalk.
* [fce_api_internal.h](fce_api_internal.h.md) - Internal file change event packet, history and coalescing types.
* [fce_util.c](fce_util.c.md) - Event coalescing for the file change event (FCE) API.
* [file.c](file.c.md) - File parameters and the AFP file commands: FPCreateFile, FPCopyFile, FPExchangeFiles, and the file ID commands.
* [file.h](file.h.md) - AFP file command handlers and file metadata helpers.
* [filedir.c](filedir.c.md) - AFP commands for both files and directories: FPGetFileDirParms, FPSetFileDirParms, FPRename, FPDelete, and FPMoveAndRename.
* [filedir.h](filedir.h.md) - Shared AFP file and directory command handlers and name checks.
* [fork.c](fork.c.md) - Fork commands: FPOpenFork, FPRead, FPWrite, FPCloseFork, byte range locks, and the flush commands.
* [fork.h](fork.h.md) - Open fork state, byte range locking and AFP fork command handlers.
* [hash.c](hash.c.md) - Chained hash table for the directory cache.
* [hash.h](hash.h.md) - Hash table allocation, lookup and iteration interface for afpd.
* [icon.c](icon.c.md) - Default server icons for the FPGetSrvrInfo reply and the virtual Icon file.
* [icon.h](icon.h.md) - Desktop icon resource declarations, type codes and size constants.
* [idle_worker.c](idle_worker.c.md) - Idle worker thread that runs deferred directory cache work between requests.
* [idle_worker.h](idle_worker.h.md) - AFP session idle worker scheduling and lifecycle interface.
* [main.c](main.c.md) - The master afpd: startup, listeners, session children, and shutdown.
* [mangle.c](mangle.c.md) - mangle, demangle (filename)
* [mangle.h](mangle.h.md) - AFP filename mangling and demangling interface.
* [messages.c](messages.c.md) - FPGetSrvrMsg: the login message, and the server messages from the message file.
* [misc.h](misc.h.md) - AFP server message handler and unsupported command aliases.
* [nfsquota.c](nfsquota.c.md) - User quota of an NFS volume through rquotad.
* [ofork.c](ofork.c.md) - Open fork table of a session.
* [pfd_cache.c](pfd_cache.c.md) - Cache of parent directory descriptors for the stat calls that check directory cache entries.
* [pfd_cache.h](pfd_cache.h.md) - Parent directory descriptor cache and statistics interface.
* [quota.c](quota.c.md) - Free space under user and group quotas through libquota or quotactl.
* [spotlight.c](spotlight.c.md) - FPSpotlightRPC: Spotlight queries and attribute requests over the search backends.
* [spotlight_marshalling.c](spotlight_marshalling.c.md) - Packing and unpacking of Spotlight RPC requests and replies.
* [spotlight_queries.c](spotlight_queries.c.md) - Session-local Spotlight query ownership and deferred cancellation.
* [status.c](status.c.md) - The FPGetSrvrInfo status block.
* [status.h](status.h.md) - AFP server status construction and server information interface.
* [switch.c](switch.c.md) - AFP command dispatch tables of the session.
* [switch.h](switch.h.md) - AFP command dispatch tables and UAM handler replacement interface.
* [uam.c](uam.c.md) - UAM module loading and the server services that UAMs call.
* [uam_auth.h](uam_auth.h.md) - Authentication module types and loading interface for afpd.
* [unix.c](unix.c.md) - Unix modes, owners, and AFP access rights of files and directories.
* [unix.h](unix.h.md) - Unix permissions, ownership and quota interface for afpd.
* [virtual_icon.c](virtual_icon.c.md) - The virtual Icon file of a volume root.
* [virtual_icon.h](virtual_icon.h.md) - Synthetic volume icon lookup, metadata and resource fork interface.
* [volume.c](volume.c.md) - Volume commands: FPOpenVol, FPCloseVol, FPGetVolParms, and volume space.
* [volume.h](volume.h.md) - AFP volume command handlers and volume lifecycle helpers.
