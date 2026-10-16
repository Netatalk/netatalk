# src/etc/afpd

* [acl_mappings.h](acl_mappings.h.md) - Stuff for mapping between ACL implementations.
* [acls.c](acls.c.md) - ACL support of afpd: FPAccess, FPGetACL, FPSetACL, and the Darwin ACE mapping.
* [acls.h](acls.h.md) - Definitions for ACL mapping code.
* [ad_cache.c](ad_cache.c.md) - AppleDouble metadata and resource fork caching in the directory cache.
* [ad_cache.h](ad_cache.h.md) - includes 3 project headers.
* [afp_asp.c](afp_asp.c.md) - AFP sessions over ASP: signal handling and the request loop.
* [afp_config.c](afp_config.c.md) - Configuration setup of afpd at startup and reload: UAMs, listeners, and server-wide settings.
* [afp_config.h](afp_config.h.md) - includes 3 project headers.
* [afp_dsi.c](afp_dsi.c.md) - AFP sessions over DSI: signal handling, session reconnects, and the request loop.
* [afp_options.c](afp_options.c.md) - Command line parsing and version output of afpd.
* [afprun.c](afprun.c.md) - Shell command execution as the session user for preexec, postexec, and the FCE notify script.
* [afpstats.c](afpstats.c.md) - The afpstats socket: session statistics for the afpstats command.
* [afpstats.h](afpstats.h.md) - includes 1 project header.
* [appl.c](appl.c.md) - The APPL desktop database: FPAddAPPL, FPRemoveAPPL, and FPGetAPPL.
* [auth.c](auth.c.md) - AFP login, logout, password, and session token commands, and UAM loading and dispatch.
* [auth.h](auth.h.md) - 1 type, includes 1 project header.
* [catsearch.c](catsearch.c.md) - FPCatSearch implementation.
* [desktop.c](desktop.c.md) - Manage the AppleDesktop folder and its contents.
* [desktop.h](desktop.h.md) - 1 type, includes 2 project headers.
* [dircache.c](dircache.c.md) - Directory Cache.
* [dircache.h](dircache.h.md) - includes 3 project headers.
* [directory.c](directory.c.md) - Directory resolution by CNID and path, the AFP directory commands, FPMapID, and FPMapName.
* [directory.h](directory.h.md) - 1 function, 2 types, includes 4 project headers.
* [enumerate.c](enumerate.c.md) - Directory listing: FPEnumerate, FPEnumerateExt, and FPEnumerateExt2.
* [extattrs.c](extattrs.c.md) - The AFP extended attribute commands: FPListExtAttrs, FPGetExtAttr, FPSetExtAttr, and FPRemoveExtAttr.
* [extattrs.h](extattrs.h.md) - No functions or types.
* [fce_api.c](fce_api.c.md) - File change event API for netatalk.
* [fce_api_internal.h](fce_api_internal.h.md) - Internal definitions for File Change Events API.
* [fce_util.c](fce_util.c.md) - Event coalescing for the file change event (FCE) API.
* [file.c](file.c.md) - File parameters and the AFP file commands: FPCreateFile, FPCopyFile, FPExchangeFiles, and the file ID commands.
* [file.h](file.h.md) - includes 4 project headers.
* [filedir.c](filedir.c.md) - AFP commands for both files and directories: FPGetFileDirParms, FPSetFileDirParms, FPRename, FPDelete, and FPMoveAndRename.
* [filedir.h](filedir.h.md) - includes 2 project headers.
* [fork.c](fork.c.md) - Fork commands: FPOpenFork, FPRead, FPWrite, FPCloseFork, byte range locks, and the flush commands.
* [fork.h](fork.h.md) - 1 function, 2 types, includes 3 project headers.
* [hash.c](hash.c.md) - Chained hash table for the directory cache.
* [hash.h](hash.h.md) - 7 functions, includes 1 project header.
* [icon.c](icon.c.md) - Default server icons for the FPGetSrvrInfo reply and the virtual Icon file.
* [icon.h](icon.h.md) - No functions or types.
* [idle_worker.c](idle_worker.c.md) - Idle worker thread that runs deferred directory cache work between requests.
* [idle_worker.h](idle_worker.h.md) - No functions or types.
* [main.c](main.c.md) - The master afpd: startup, listeners, session children, and shutdown.
* [mangle.c](mangle.c.md) - mangle, demangle (filename)
* [mangle.h](mangle.h.md) - includes 6 project headers.
* [messages.c](messages.c.md) - FPGetSrvrMsg: the login message, and the server messages from the message file.
* [misc.h](misc.h.md) - includes 1 project header.
* [nfsquota.c](nfsquota.c.md) - User quota of an NFS volume through rquotad.
* [ofork.c](ofork.c.md) - Open fork table of a session.
* [pfd_cache.c](pfd_cache.c.md) - Cache of parent directory descriptors for the stat calls that check directory cache entries.
* [pfd_cache.h](pfd_cache.h.md) - 1 type, includes 2 project headers.
* [quota.c](quota.c.md) - Free space under user and group quotas through libquota or quotactl.
* [spotlight.c](spotlight.c.md) - FPSpotlightRPC: Spotlight queries and attribute requests over the search backends.
* [spotlight_marshalling.c](spotlight_marshalling.c.md) - Packing and unpacking of Spotlight RPC requests and replies.
* [spotlight_queries.c](spotlight_queries.c.md) - Session-local Spotlight query ownership and deferred cancellation.
* [status.c](status.c.md) - The FPGetSrvrInfo status block.
* [status.h](status.h.md) - includes 4 project headers.
* [switch.c](switch.c.md) - AFP command dispatch tables of the session.
* [switch.h](switch.h.md) - No functions or types.
* [uam.c](uam.c.md) - UAM module loading and the server services that UAMs call.
* [uam_auth.h](uam_auth.h.md) - interface between uam.c and auth.c
* [unix.c](unix.c.md) - Unix modes, owners, and AFP access rights of files and directories.
* [unix.h](unix.h.md) - 1 function, includes 1 project header.
* [virtual_icon.c](virtual_icon.c.md) - The virtual Icon file of a volume root.
* [virtual_icon.h](virtual_icon.h.md) - includes 2 project headers.
* [volume.c](volume.c.md) - Volume commands: FPOpenVol, FPCloseVol, FPGetVolParms, and volume space.
* [volume.h](volume.h.md) - includes 4 project headers.
