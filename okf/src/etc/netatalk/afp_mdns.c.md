---
type: C Source File
title: "etc/netatalk/afp_mdns.c"
description: "mDNS based Zeroconf support"
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/netatalk/afp_mdns.c"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-27T17:55:55+01:00 }
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

Defined at lines 52 to 72.

Set a TXT record entry to a value formatted with vasprintf.

Parameters:
* `rec`: TXT record
* `key`: entry name
* `fmt`: printf format of the value
* `...`: format arguments

Returns: 0, or -1 when the format or TXTRecordSetValue fails

Calls: [vasprintf](../../libatalk/compat/misc.c.md#vasprintf)

Called by: [register_stuff](afp_mdns.c.md#register_stuff)

### TXTRecordKeyPrintf

```c
int TXTRecordKeyPrintf(TXTRecordRef *rec, const char *key_fmt, int key_var, const char *fmt,...)
```

Defined at lines 87 to 124.

Set a TXT record entry, named with an int, to a value formatted with vasprintf.

The adisk record uses it for the dk0, dk1, and later entries of the volumes.

Parameters:
* `rec`: TXT record
* `key_fmt`: printf format of the entry name, with one int
* `key_var`: int for key_fmt
* `fmt`: printf format of the value
* `...`: format arguments

Returns: 0, or -1 when a format or TXTRecordSetValue fails

Calls: [asprintf](../../libatalk/compat/misc.c.md#asprintf), [vasprintf](../../libatalk/compat/misc.c.md#vasprintf)

Called by: [register_stuff](afp_mdns.c.md#register_stuff)

### polling_thread

```c
static void * polling_thread(void *arg)
```

Defined at lines 137 to 161.

Poll the sockets of the registered services and run their callbacks.

The thread watches every service socket for input and runs DNSServiceProcessResult on it. It ends when poll fails, EINTR included.

Parameters:
* `arg`: unused

Returns: NULL

Called by: [register_stuff](afp_mdns.c.md#register_stuff)

Uses file-scope variables: `svc_ref_count`, `svc_refs`

### RegisterReply

```c
static void RegisterReply(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context)
```

Defined at lines 176 to 184.

Log a failed registration, as the DNSServiceRegister callback.

A failure leaves nothing to retry, so the call only logs it.

Parameters:
* `sdRef`: service reference, unused
* `flags`: flags, unused
* `errorCode`: result of the registration
* `name`: service name
* `regtype`: service type
* `domain`: domain
* `context`: unused

Called by: [register_stuff](afp_mdns.c.md#register_stuff)

### unregister_stuff

```c
static void unregister_stuff(void)
```

Defined at lines 190 to 210.

This function unregisters anything we have already registered and frees associated memory.

Called by: [md_zeroconf_unregister](afp_mdns.c.md#md_zeroconf_unregister), [register_stuff](afp_mdns.c.md#register_stuff)

Uses file-scope variables: `poller`, `svc_ref_count`, `svc_refs`

### register_stuff

```c
static void register_stuff(const AFPObj *obj)
```

Defined at lines 224 to 426.

Register the AFP service, the Time Machine volumes, and the device model with mDNSResponder.

An earlier registration goes first. The AFP service goes out under the server name, or the host name without one. Time Machine volumes go into the TXT record of an adisk service, each with its volume UUID when it has one. The mimic model adds a device-info service on port 9. A thread then polls the service sockets.

Parameters:
* `obj`: AFP object with the options

Calls: [RegisterReply](afp_mdns.c.md#registerreply), [TXTRecordKeyPrintf](afp_mdns.c.md#txtrecordkeyprintf), [TXTRecordPrintf](afp_mdns.c.md#txtrecordprintf), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [polling_thread](afp_mdns.c.md#polling_thread), [unregister_stuff](afp_mdns.c.md#unregister_stuff)

Called by: [md_zeroconf_register](afp_mdns.c.md#md_zeroconf_register)

Uses file-scope variables: `poller`, `svc_ref_count`, `svc_refs`

### md_zeroconf_register

```c
void md_zeroconf_register(const AFPObj *obj)
```

Defined at lines 438 to 443.

Register the AFP services with mDNSResponder and start the polling thread.

Parameters:
* `obj`: AFP object with the options

Calls: [register_stuff](afp_mdns.c.md#register_stuff)

Called by: [zeroconf_register](afp_zeroconf.c.md#zeroconf_register)

### md_zeroconf_unregister

```c
int md_zeroconf_unregister(void)
```

Defined at lines 452 to 456.

Cancel the polling thread and withdraw the mDNS services.

The call cancels the polling thread, so the caller has to run outside it.

Returns: 0

Calls: [unregister_stuff](afp_mdns.c.md#unregister_stuff)

Called by: [zeroconf_deregister](afp_zeroconf.c.md#zeroconf_deregister)

# File-scope variables

`fds`, `poller`, `svc_ref_count`, `svc_refs`
