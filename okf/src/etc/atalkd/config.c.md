---
type: C Source File
title: "etc/atalkd/config.c"
description: "13 functions, 1 type, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/config.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Defined at lines 83 to 169.

Split an atalkd.conf line into words.

White space separates the words, and a word in double quotes keeps its spaces. The array ends with NULL, and the slot after it holds the line buffer for freeline.

Parameters:
* `line`: configuration line

Returns: the word array, or NULL for a blank line, a comment, or a failed allocation

Called by: [readconf](config.c.md#readconf), [writeconf](config.c.md#writeconf)

### freeline

```c
static void freeline(char **argv)
```

Defined at lines 177 to 189.

Free a word array from at_parseline and the line buffer stored after its NULL.

Parameters:
* `argv`: word array, or NULL

Called by: [writeconf](config.c.md#writeconf)

### writeconf

```c
int writeconf(char *)
```

Defined at lines 207 to 339. Declared in [etc/atalkd/main.c](main.c.md).

Rewrite atalkd.conf with the configuration that atalkd settled on.

The call keeps blank lines and comments, and replaces each configuration line with the state of the next interface. That state holds the name, the routing options, the phase, the net range, the address, and the zones. Lines beyond the last interface go away. Without an old file at the default path, each interface gets a line.

The call writes a temporary file beside the old one and renames it into place. The temporary file gets the mode of the old file, or 0644 without one.

Parameters:
* `cf`: configuration file, or NULL for _PATH_ATALKDCONF

Returns: 0, or -1 when the file lacks the owner write bit or an open, write, or rename fails

Calls: [at_parseline](config.c.md#at_parseline), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [freeline](config.c.md#freeline)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

### readconf

```c
int readconf(char *)
```

Defined at lines 373 to 528. Declared in [etc/atalkd/main.c](main.c.md).

Read the interfaces of atalkd.conf into the interface list.

Each line names an interface and its options. The second example sets up both phases on one interface.

```
interface [ -router ] [ -dontroute ] [ -seed ] [ -phase number ]
    [ -net net-range ] [ -addr net.node ] [ -zone zonename ]...

le0 -phase 1 -net 7938 -zone Argus

le0 -phase 2 -net 8043-8044 -zone Argus -zone "Research Systems"
le0 -phase 1 -net 7938 -zone Argus
```

The network supplies the net range, address, and zones that a line leaves out. Without -seed another router can override the configuration. With -seed, atalkd exits when a phase 2 router reports another net range or default zone. The phase defaults to 2 or to the -1 or -2 option of atalkd. The -addr option can stand in for a -net that is not a range, and the first zone of an interface is its default zone.

Each interface has to support AppleTalk and take the AppleTalk multicast address. The call refuses loopback, point-to-point, and slave interfaces, and an interface whose name and phase repeat. An error returns -1 after a message on stderr. Main then calls getifconf, which adds the other kernel interfaces to the ones read so far.

Parameters:
* `cf`: configuration file, or NULL for _PATH_ATALKDCONF

Returns: 0 with at least IFBASE interfaces, loopback included, or -1 for a missing file, an error, or fewer interfaces

Calls: [addmulti](multicast.c.md#addmulti), [at_parseline](config.c.md#at_parseline), [newiface](config.c.md#newiface), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [main](main.c.md#main)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md), `params`

### router

```c
int router(struct interface *iface, char **av)
```

Defined at lines 541 to 560.

Apply the -router option, which also makes the interface a seed.

The option has to come before -zone, and it excludes -dontroute.

Parameters:
* `iface`: interface of the line
* `av`: words after the option, unused

Returns: 1, the number of words used, or -1 when -dontroute or -zone came first

Dispatched via: [params](config.c.md#params)

### dontroute

```c
int dontroute(struct interface *iface, char **av)
```

Defined at lines 570 to 580.

Apply the -dontroute option, which excludes -router.

Parameters:
* `iface`: interface of the line
* `av`: words after the option, unused

Returns: 1, the number of words used, or -1 when -router came first

Dispatched via: [params](config.c.md#params)

### seed

```c
int seed(struct interface *iface, char **av)
```

Defined at lines 590 to 604.

Apply the -seed option, which has to come before -zone.

Parameters:
* `iface`: interface of the line
* `av`: words after the option, unused

Returns: 1, the number of words used, or -1 after a -zone

Dispatched via: [params](config.c.md#params)

### phase

```c
int phase(struct interface *iface, char **av)
```

Defined at lines 613 to 638.

Apply the -phase option, AppleTalk phase 1 or 2.

Parameters:
* `iface`: interface of the line
* `av`: words after the option, the phase first

Returns: 2, the number of words used, or -1 for a missing or unknown phase

Dispatched via: [params](config.c.md#params)

### net

```c
int net(struct interface *iface, char **av)
```

Defined at lines 652 to 728.

Apply the -net option, a network number or, in phase 2, a range like 8043-8044.

The phase has to come first, and the range has to hold a configured address. A range marks the route of the interface as extended.

Parameters:
* `iface`: interface of the line, receives its route entry
* `av`: words after the option, the network first

Returns: 2, the number of words used, or -1 for a bad network, a range in phase 1, a missing phase, or a failed allocation

Calls: [newrt](rtmp.c.md#newrt)

Dispatched via: [params](config.c.md#params)

### addr

```c
int addr(struct interface *iface, char **av)
```

Defined at lines 741 to 775.

Apply the -addr option, a net.node address of the interface.

When -net came first, the address has to lie in its range. Otherwise the route of the interface becomes the single net of the address.

Parameters:
* `iface`: interface of the line, receives the address
* `av`: words after the option, the address first

Returns: 2, the number of words used, or -1 for a missing or bad address, one outside the range, or a failed allocation

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [newrt](rtmp.c.md#newrt)

Dispatched via: [params](config.c.md#params)

### zone

```c
int zone(struct interface *iface, char **av)
```

Defined at lines 789 to 833.

Apply the -zone option, which counts only on a seed interface.

The zone name goes from the Unix charset to the Mac charset. The net range has to come first. The first zone stays the default, and each later one goes right after it.

Parameters:
* `iface`: interface of the line, receives the zone
* `av`: words after the option, the zone name first

Returns: 2, the number of words used, or -1 for a missing name, a missing net range, or a failed allocation

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [newzt](zip.c.md#newzt)

Dispatched via: [params](config.c.md#params)

Mentioned in the documentation of: [main](../../bin/nbp/nbplkup.c.md#main), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name)

### getifconf

```c
int getifconf(void)
```

Defined at lines 845 to 930. Declared in [etc/atalkd/main.c](main.c.md).

Add the AppleTalk interfaces of the kernel that the interface list lacks.

Main calls it when readconf fails. An interface joins the list when it is up and supports AppleTalk, and is not loopback, point-to-point, or slave. It also has to take the AppleTalk multicast address and be new to the list.

Returns: 0, or -1 when the socket or an allocation fails

Calls: [addmulti](multicast.c.md#addmulti), [freeifacelist](../../libatalk/util/getiface.c.md#freeifacelist), [getifacelist](../../libatalk/util/getiface.c.md#getifacelist), [newiface](config.c.md#newiface), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [main](main.c.md#main)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

### newiface

```c
struct interface * newiface(const char *name)
```

Defined at lines 939 to 956.

Allocate a zeroed interface structure with its name and the AppleTalk family on both addresses.

Parameters:
* `name`: interface name, cut to fit

Returns: the interface, or NULL when calloc fails

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [getifconf](config.c.md#getifconf), [main](main.c.md#main), [readconf](config.c.md#readconf)

# Types

### struct param

Defined at line 57.
* `char * p_name`
* `int(* p_func`

# Macros

* Undocumented: `ARGV_CHUNK_SIZE`, `IFF_SLAVE`, `MAXLINELEN`
