---
type: Subsystem
title: "libatalk/util"
description: "20 files, 190 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/util"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
---

# Files

* [libatalk/util/afp_util.c](util/afp_util.c.md): 2 functions, includes 2 project headers.
* [libatalk/util/atalk_addr.c](util/atalk_addr.c.md): 1 function, includes 2 project headers.
* [libatalk/util/bprint.c](util/bprint.c.md): 1 function, includes 1 project header.
* [libatalk/util/cnid.c](util/cnid.c.md): 3 functions, includes 6 project headers.
* [libatalk/util/constant_time.c](util/constant_time.c.md): 1 function, includes 1 project header.
* [libatalk/util/fault.c](util/fault.c.md): 8 functions, includes 2 project headers.
* [libatalk/util/getiface.c](util/getiface.c.md): 4 functions, includes 1 project header.
* [libatalk/util/gettok.c](util/gettok.c.md): 2 functions, includes 1 project header.
* [libatalk/util/locking.c](util/locking.c.md): Netatalk utility functions: locking.
* [libatalk/util/logger.c](util/logger.c.md): 19 functions, includes 4 project headers.
* [libatalk/util/netatalk_conf.c](util/netatalk_conf.c.md): 52 functions, includes 13 project headers.
* [libatalk/util/pathconv.c](util/pathconv.c.md): 1 function, includes 4 project headers.
* [libatalk/util/queue.c](util/queue.c.md): 9 functions, includes 1 project header.
* [libatalk/util/server_child.c](util/server_child.c.md): functions to handle child processes
* [libatalk/util/server_ipc.c](util/server_ipc.c.md): 23 functions, 2 types, includes 7 project headers.
* [libatalk/util/server_lock.c](util/server_lock.c.md): 3 functions, includes 2 project headers.
* [libatalk/util/sigpipe.c](util/sigpipe.c.md): 4 functions, includes 1 project header.
* [libatalk/util/socket.c](util/socket.c.md): 13 functions, includes 3 project headers.
* [libatalk/util/strdicasecmp.c](util/strdicasecmp.c.md): 2 functions, includes 1 project header.
* [libatalk/util/unix.c](util/unix.c.md): 27 functions, includes 11 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 69 includes
* [sys/netatalk](../sys/netatalk.md): 1 includes

# Calls into

* [libatalk/compat](compat.md): 18 calls
* [libatalk/unicode](unicode.md): 7 calls
* [libatalk/vfs](vfs.md): 3 calls
* [libatalk/acl](acl.md): 2 calls
* [libatalk/cnid](cnid.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 253 calls
* [libatalk/adouble](adouble.md): 28 calls
* [bin/nad](../bin/nad.md): 18 calls
* [libatalk/dsi](dsi.md): 18 calls
* [etc/atalkd](../etc/atalkd.md): 12 calls
* [etc/netatalk](../etc/netatalk.md): 12 calls
* [libatalk/vfs](vfs.md): 12 calls
* [etc/uams](../etc/uams.md): 9 calls
* [bin/dbd](../bin/dbd.md): 8 calls
* [etc/papd](../etc/papd.md): 6 calls
* [libatalk/asp](asp.md): 6 calls
* [libatalk/atp](atp.md): 6 calls
* [libatalk/cnid](cnid.md): 6 calls
* [etc/spotlight](../etc/spotlight.md): 5 calls
* [libatalk/acl](acl.md): 5 calls
* [bin/misc](../bin/misc.md): 3 calls
* [bin/nbp](../bin/nbp.md): 3 calls
* [bin/pap](../bin/pap.md): 2 calls
* [bin/aecho](../bin/aecho.md): 1 calls
* [bin/afppasswd](../bin/afppasswd.md): 1 calls
* [bin/getzones](../bin/getzones.md): 1 calls
* [bin/rtmpqry](../bin/rtmpqry.md): 1 calls
* [libatalk/nbp](nbp.md): 1 calls

# Most called functions

* [getvolbyvid](util/netatalk_conf.c.md#getvolbyvid): 47 callers
* [fullpathname](util/unix.c.md#fullpathname): 33 callers
* [ostat](util/unix.c.md#ostat): 26 callers
* [become_root](util/unix.c.md#become_root): 21 callers
* [unbecome_root](util/unix.c.md#unbecome_root): 21 callers
* [ipc_send_cache_hint](util/server_ipc.c.md#ipc_send_cache_hint): 18 callers
* [getcwdpath](util/unix.c.md#getcwdpath): 15 callers
* [atalk_aton](util/atalk_addr.c.md#atalk_aton): 12 callers
* [atalk_sigpipe_notify](util/sigpipe.c.md#atalk_sigpipe_notify): 10 callers
* [cnid_for_path](util/cnid.c.md#cnid_for_path): 10 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[conf_testutil_set_lastvid](util/netatalk_conf.c.md#conf_testutil_set_lastvid), [fault_setup_thread](util/fault.c.md#fault_setup_thread), [getvolbyname](util/netatalk_conf.c.md#getvolbyname), [lock_reg](util/locking.c.md#lock_reg), [make_log_entry](util/logger.c.md#make_log_entry), [netatalk_panic](util/fault.c.md#netatalk_panic), [prequeue](util/queue.c.md#prequeue), [volume_unlink](util/netatalk_conf.c.md#volume_unlink)
