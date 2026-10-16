---
type: Build Record
title: "Build record"
description: "Netatalk commit 7279304e7fe0 rendered by process:okf-from-doxygen/1 from Doxygen 1.15.0."
resource: "https://github.com/Netatalk/netatalk/commit/7279304e7fe0fec6b442dadddb28eb72097e973b"
tags: ["build"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-17T10:10:08+11:00 }
---

# Source

* Commit: [7279304e7fe0fec6b442dadddb28eb72097e973b](https://github.com/Netatalk/netatalk/commit/7279304e7fe0fec6b442dadddb28eb72097e973b), committed 2026-10-17T10:10:08+11:00
* Scanned directories: `bin`, `etc`, `include`, `libatalk`, `sys`
* Doxygen: 1.15.0, configured from `doc/Doxyfile.in` with XML output and reference relations

# Preprocessor view

Conditional code is included as if these macros were defined, so the graph reflects a maximal feature build rather than any one platform:

`ATOMIC_INT_LOCK_FREE=2`, `AVAHI_GCC_UNUSED=`, `HAVE_ACLS=1`, `HAVE_AVAHI=1`, `HAVE_CUPS=1`, `HAVE_LDAP=1`, `HAVE_LIBQUOTA=1`, `HAVE_LISTXATTR=1`, `HAVE_MDNS=1`, `HAVE_NFSV4_ACLS=1`, `HAVE_POSIX_ACLS=1`, `HAVE_SIGALTSTACK=1`, `HAVE_USABLE_ICONV=1`, `SERVERTEXT=1`, `SPOTLIGHT_BACKEND_LOCALSEARCH=1`, `TCPWRAP=1`, `USE_CRACKLIB=1`, `WITH_DTRACE=1`, `WITH_LIBEV=1`, `WITH_RECVFILE=1`, `WITH_SENDFILE=1`, `WITH_SPOTLIGHT=1`, `__attribute__(x)=`, `_U_=`

# Counts

* Files: 355
* Subsystems: 32
* Functions: 2110
* Documented functions: 2058
* Types: 158
* Function tables: 55
* Call edges: 4551
* Variable-use edges: 1588
* Table edges: 248
* Dispatch edges: 183
* Fields called through: 51
* Documented-reference edges: 93
* Graph edges: 8182

# Edge kinds

* include: a file includes a project header
* call: a function body references another function
* table: a variable initializer names a function (dispatch and operation tables); for a structure-typed table the edge carries the field
* dispatch: a function body calls through a function-pointer field of a structure; one edge per function a table assigns to that field
* use: a function body references a file-scope variable
* doc: a Doxygen comment references a function

Doxygen records call edges from function bodies by name. A function whose address is taken inside another body appears as a callee of that body, which is the registration site rather than the eventual caller. A call through a function-pointer field reaches every function any table assigns to the field, so dispatch edges are candidates; a field filled by code rather than by a table initializer has no dispatch edge.
