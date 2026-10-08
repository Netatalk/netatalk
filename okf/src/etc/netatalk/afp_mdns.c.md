---
type: C Source File
title: "etc/netatalk/afp_mdns.c"
description: "mDNS based Zeroconf support"
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/netatalk/afp_mdns.c"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_mdns.h](afp_mdns.h.md)
* [afp_zeroconf.h](afp_zeroconf.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `poll.h`, `pthread.h`, `time.h`, `unistd.h`

# Functions

### TXTRecordPrintf

```c
int TXTRecordPrintf(TXTRecordRef *rec, const char *key, const char *fmt,...)
```

Defined at lines 39 to 59.

Calls: [vasprintf](../../libatalk/compat/misc.c.md#vasprintf)

Called by: [register_stuff](afp_mdns.c.md#register_stuff)

### TXTRecordKeyPrintf

```c
int TXTRecordKeyPrintf(TXTRecordRef *rec, const char *key_fmt, int key_var, const char *fmt,...)
```

Defined at lines 61 to 98.

Calls: [asprintf](../../libatalk/compat/misc.c.md#asprintf), [vasprintf](../../libatalk/compat/misc.c.md#vasprintf)

Called by: [register_stuff](afp_mdns.c.md#register_stuff)

### polling_thread

```c
static void * polling_thread(void *arg)
```

Defined at lines 105 to 129.

This is the thread that polls the filehandles

Called by: [register_stuff](afp_mdns.c.md#register_stuff)

Uses file-scope variables: `fds`, `svc_ref_count`, `svc_refs`

### RegisterReply

```c
static void RegisterReply(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context)
```

Defined at lines 136 to 144.

This is the callback for the service register function.

Note: actually there isn't a lot we can do if we get problems, so we don't really need to do anything other than report the issue.

Called by: [register_stuff](afp_mdns.c.md#register_stuff)

### unregister_stuff

```c
static void unregister_stuff(void)
```

Defined at lines 150 to 170.

This function unregisters anything we have already registered and frees associated memory

Called by: [md_zeroconf_unregister](afp_mdns.c.md#md_zeroconf_unregister), [register_stuff](afp_mdns.c.md#register_stuff)

Uses file-scope variables: `fds`, `poller`, `svc_ref_count`, `svc_refs`

### register_stuff

```c
static void register_stuff(const AFPObj *obj)
```

Defined at lines 176 to 378.

This function tries to register the AFP DNS SRV service type.

Calls: [RegisterReply](afp_mdns.c.md#registerreply), [TXTRecordKeyPrintf](afp_mdns.c.md#txtrecordkeyprintf), [TXTRecordPrintf](afp_mdns.c.md#txtrecordprintf), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [polling_thread](afp_mdns.c.md#polling_thread), [unregister_stuff](afp_mdns.c.md#unregister_stuff)

Called by: [md_zeroconf_register](afp_mdns.c.md#md_zeroconf_register)

Uses file-scope variables: `poller`, `svc_ref_count`, `svc_refs`

### md_zeroconf_register

```c
void md_zeroconf_register(const AFPObj *obj)
```

Defined at lines 388 to 393.

Tries to setup the Zeroconf thread and any neccessary config setting.

Calls: [register_stuff](afp_mdns.c.md#register_stuff)

Called by: [zeroconf_register](afp_zeroconf.c.md#zeroconf_register)

### md_zeroconf_unregister

```c
int md_zeroconf_unregister(void)
```

Defined at lines 399 to 403.

Tries to shutdown this loop impl.

Note: Call this function from inside this thread.

Calls: [unregister_stuff](afp_mdns.c.md#unregister_stuff)

Called by: [zeroconf_deregister](afp_zeroconf.c.md#zeroconf_deregister)

# File-scope variables

`fds`, `poller`, `svc_ref_count`, `svc_refs`
