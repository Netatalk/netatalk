---
type: Subsystem
title: "libatalk/util"
description: "20 files, 206 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/util"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T20:46:18+11:00 }
---

# Files

* [libatalk/util/afp_util.c](util/afp_util.c.md): Names of AFP command and result codes for log messages.
* [libatalk/util/atalk_addr.c](util/atalk_addr.c.md): Parsing of AppleTalk addresses in text form.
* [libatalk/util/bprint.c](util/bprint.c.md): Hex dump of a buffer to stdout.
* [libatalk/util/cnid.c](util/cnid.c.md): CNID resolution of paths, and volume UUIDs without dashes for CNID table names.
* [libatalk/util/constant_time.c](util/constant_time.c.md): Constant-time memory comparison.
* [libatalk/util/fault.c](util/fault.c.md): Fault signal handlers with backtraces, from Samba.
* [libatalk/util/getiface.c](util/getiface.c.md): Lists of the network interface names of the system.
* [libatalk/util/gettok.c](util/gettok.c.md): Line tokenizer for configuration files, used by the extmap.conf reader.
* [libatalk/util/locking.c](util/locking.c.md): Netatalk utility functions: locking.
* [libatalk/util/logger.c](util/logger.c.md): The netatalk logging system: log types, levels, files, and syslog.
* [libatalk/util/netatalk_conf.c](util/netatalk_conf.c.md): Reading afp.conf and extmap.conf: server options, volume definitions, and the extension map.
* [libatalk/util/pathconv.c](util/pathconv.c.md): UTF-8 to Mac file name conversion with mangling.
* [libatalk/util/queue.c](util/queue.c.md): Netatalk utility functions: queue.
* [libatalk/util/server_child.c](util/server_child.c.md): functions to handle child processes
* [libatalk/util/server_ipc.c](util/server_ipc.c.md): IPC over socketpair between parent and children.
* [libatalk/util/server_lock.c](util/server_lock.c.md): Pid files and daemonizing of the netatalk servers.
* [libatalk/util/sigpipe.c](util/sigpipe.c.md): Self-pipe that wakes a poll loop from signal handlers.
* [libatalk/util/socket.c](util/socket.c.md): Netatalk utility functions: socket.
* [libatalk/util/strdicasecmp.c](util/strdicasecmp.c.md): Case-insensitive string comparison through the Mac Roman uppercase map.
* [libatalk/util/unix.c](util/unix.c.md): Netatalk utility functions: unix.

# Includes headers from

* [include/atalk](../include/atalk.md): 69 includes
* [sys/netatalk](../sys/netatalk.md): 1 includes

# Calls into

* [libatalk/compat](compat.md): 19 calls
* [libatalk/unicode](unicode.md): 7 calls
* [libatalk/vfs](vfs.md): 3 calls
* [libatalk/acl](acl.md): 2 calls
* [libatalk/cnid](cnid.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 276 calls
* [libatalk/adouble](adouble.md): 29 calls
* [libatalk/dsi](dsi.md): 23 calls
* [bin/nad](../bin/nad.md): 18 calls
* [etc/netatalk](../etc/netatalk.md): 17 calls
* [libatalk/vfs](vfs.md): 13 calls
* [etc/atalkd](../etc/atalkd.md): 12 calls
* [libatalk/cnid](cnid.md): 11 calls
* [etc/uams](../etc/uams.md): 9 calls
* [bin/dbd](../bin/dbd.md): 8 calls
* [etc/papd](../etc/papd.md): 6 calls
* [libatalk/atp](atp.md): 6 calls
* [etc/spotlight](../etc/spotlight.md): 5 calls
* [libatalk/acl](acl.md): 5 calls
* [bin/nbp](../bin/nbp.md): 3 calls
* [bin/pap](../bin/pap.md): 2 calls
* [bin/aecho](../bin/aecho.md): 1 calls
* [bin/afpldaptest](../bin/afpldaptest.md): 1 calls
* [bin/afppasswd](../bin/afppasswd.md): 1 calls
* [bin/getzones](../bin/getzones.md): 1 calls
* [bin/rtmpqry](../bin/rtmpqry.md): 1 calls
* [libatalk/asp](asp.md): 1 calls
* [libatalk/nbp](nbp.md): 1 calls

# Most called functions

* [getvolbyvid](util/netatalk_conf.c.md#getvolbyvid): 48 callers
* [fullpathname](util/unix.c.md#fullpathname): 33 callers
* [ostat](util/unix.c.md#ostat): 25 callers
* [become_root](util/unix.c.md#become_root): 21 callers
* [unbecome_root](util/unix.c.md#unbecome_root): 21 callers
* [ipc_send_cache_hint](util/server_ipc.c.md#ipc_send_cache_hint): 18 callers
* [getcwdpath](util/unix.c.md#getcwdpath): 15 callers
* [getvolumes](util/netatalk_conf.c.md#getvolumes): 15 callers
* [atalk_aton](util/atalk_addr.c.md#atalk_aton): 12 callers
* [atalk_sigpipe_notify](util/sigpipe.c.md#atalk_sigpipe_notify): 11 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[conf_testutil_set_lastvid](util/netatalk_conf.c.md#conf_testutil_set_lastvid), [getvolbyname](util/netatalk_conf.c.md#getvolbyname), [ipc_get_hints_dropped](util/server_ipc.c.md#ipc_get_hints_dropped), [ipc_get_hints_sent](util/server_ipc.c.md#ipc_get_hints_sent), [lock_reg](util/locking.c.md#lock_reg), [make_log_entry](util/logger.c.md#make_log_entry), [netatalk_panic](util/fault.c.md#netatalk_panic), [prequeue](util/queue.c.md#prequeue), [volume_unlink](util/netatalk_conf.c.md#volume_unlink)
