---
type: C Source File
title: "etc/atalkd/config.c"
description: "13 functions, 1 type, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/config.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [interface.h](interface.h.md)
* [list.h](list.h.md)
* [main.h](main.h.md)
* [multicast.h](multicast.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [rtmp.h](rtmp.h.md)
* [zip.h](zip.h.md)
* System headers: `arpa/inet.h`, `assert.h`, `ctype.h`, `errno.h`, `fcntl.h`, `net/if.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/param.h`, `sys/socket.h`, `sys/stat.h`, `sys/types.h`

# Function tables

### params

Initialized at line 68. Dispatches to: [addr](config.c.md#addr), [dontroute](config.c.md#dontroute), [net](config.c.md#net), [phase](config.c.md#phase), [router](config.c.md#router), [seed](config.c.md#seed), [zone](config.c.md#zone)

# Functions

### at_parseline

```c
static char ** at_parseline(const char *line)
```

Defined at lines 72 to 158.

Called by: [readconf](config.c.md#readconf), [writeconf](config.c.md#writeconf)

### freeline

```c
static void freeline(char **argv)
```

Defined at lines 160 to 172.

Called by: [writeconf](config.c.md#writeconf)

### writeconf

```c
int writeconf(char *)
```

Defined at lines 174 to 306. Declared in [etc/atalkd/main.c](main.c.md).

Calls: [at_parseline](config.c.md#at_parseline), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [freeline](config.c.md#freeline)

Called by: [as_timer](main.c.md#as_timer)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

### readconf

```c
int readconf(char *)
```

Defined at lines 330 to 485. Declared in [etc/atalkd/main.c](main.c.md).

Calls: [addmulti](multicast.c.md#addmulti), [at_parseline](config.c.md#at_parseline), [newiface](config.c.md#newiface), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [main](main.c.md#main)

Calls through [`param::p_func`](config.c.md#struct-param): no table assigns this field

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md), `params`

### router

```c
int router(struct interface *iface, char **av)
```

Defined at lines 488 to 507.

Dispatched via: [params](config.c.md#params)

### dontroute

```c
int dontroute(struct interface *iface, char **av)
```

Defined at lines 510 to 520.

Dispatched via: [params](config.c.md#params)

### seed

```c
int seed(struct interface *iface, char **av)
```

Defined at lines 523 to 537.

Dispatched via: [params](config.c.md#params)

### phase

```c
int phase(struct interface *iface, char **av)
```

Defined at lines 539 to 564.

Dispatched via: [params](config.c.md#params)

### net

```c
int net(struct interface *iface, char **av)
```

Defined at lines 566 to 642.

Calls: [newrt](rtmp.c.md#newrt)

Dispatched via: [params](config.c.md#params)

### addr

```c
int addr(struct interface *iface, char **av)
```

Defined at lines 644 to 678.

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [newrt](rtmp.c.md#newrt)

Dispatched via: [params](config.c.md#params)

### zone

```c
int zone(struct interface *iface, char **av)
```

Defined at lines 680 to 724.

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [newzt](zip.c.md#newzt)

Dispatched via: [params](config.c.md#params)

### getifconf

```c
int getifconf(void)
```

Defined at lines 730 to 815. Declared in [etc/atalkd/main.c](main.c.md).

Calls: [addmulti](multicast.c.md#addmulti), [freeifacelist](../../libatalk/util/getiface.c.md#freeifacelist), [getifacelist](../../libatalk/util/getiface.c.md#getifacelist), [newiface](config.c.md#newiface), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [main](main.c.md#main)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

### newiface

```c
struct interface * newiface(const char *name)
```

Defined at lines 822 to 839.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [getifconf](config.c.md#getifconf), [main](main.c.md#main), [readconf](config.c.md#readconf)

# Types

### struct param

Defined at line 57.
* `char * p_name`
* `int(* p_func`: Called through by [readconf](config.c.md#readconf).

# Macros

* Undocumented: `ARGV_CHUNK_SIZE`, `IFF_SLAVE`, `MAXLINELEN`
