---
type: Build Record
title: "Build record"
description: "Netatalk commit f9826e27d33e rendered by process:okf-from-doxygen/1 from Doxygen 1.9.8."
resource: "https://github.com/Netatalk/netatalk/commit/f9826e27d33ea4cca85e474c2e8b9c823a428bd7"
tags: ["build"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-09T22:52:52+11:00 }
---

# Source

* Commit: [f9826e27d33ea4cca85e474c2e8b9c823a428bd7](https://github.com/Netatalk/netatalk/commit/f9826e27d33ea4cca85e474c2e8b9c823a428bd7), committed 2026-10-09T22:52:52+11:00
* Scanned directories: `bin`, `etc`, `include`, `libatalk`, `sys`
* Doxygen: 1.9.8, configured from `doc/Doxyfile.in` with XML output and reference relations

# Preprocessor view

Conditional code is included as if these macros were defined, so the graph reflects a maximal feature build rather than any one platform:

`HAVE_ACLS=1`, `HAVE_AVAHI=1`, `HAVE_CUPS=1`, `HAVE_LDAP=1`, `HAVE_LIBQUOTA=1`, `HAVE_MDNS=1`, `HAVE_NFSV4_ACLS=1`, `HAVE_POSIX_ACLS=1`, `HAVE_SPLICE=1`, `HAVE_USABLE_ICONV=1`, `TCPWRAP=1`, `USE_CRACKLIB=1`, `WITH_DTRACE=1`, `WITH_RECVFILE=1`, `WITH_SENDFILE=1`, `WITH_SPOTLIGHT=1`, `__attribute__(x)=`, `_U_=`

# Counts

* Files: 357
* Subsystems: 30
* Functions: 2081
* Documented functions: 533
* Types: 160
* Function tables: 55
* Call edges: 4447
* Variable-use edges: 1683
* Table edges: 248
* Dispatch edges: 206
* Fields called through: 60
* Documented-reference edges: 139
* Graph edges: 8244

# Edge kinds

* include: a file includes a project header
* call: a function body references another function
* table: a variable initializer names a function (dispatch and operation tables); for a structure-typed table the edge carries the field
* dispatch: a function body calls through a function-pointer field of a structure; one edge per function a table assigns to that field
* use: a function body references a file-scope variable
* doc: a Doxygen comment references a function

Doxygen records call edges from function bodies by name. A function whose address is taken inside another body appears as a callee of that body, which is the registration site rather than the eventual caller. A call through a function-pointer field reaches every function any table assigns to the field, so dispatch edges are candidates; a field filled by code rather than by a table initializer has no dispatch edge.
