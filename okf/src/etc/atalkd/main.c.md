---
type: C Source File
title: "etc/atalkd/main.c"
description: "11 functions, includes 16 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/main.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/rtmp.h](../../include/atalk/rtmp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/zip.h](../../include/atalk/zip.h.md)
* [atserv.h](atserv.h.md)
* [gate.h](gate.h.md)
* [interface.h](interface.h.md)
* [list.h](list.h.md)
* [main.h](main.h.md)
* [nbp.h](nbp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [rtmp.h](rtmp.h.md)
* [zip.h](zip.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `fcntl.h`, `net/if.h`, `net/route.h`, `netdb.h`, `netinet/in.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/ioctl.h`, `sys/param.h`, `sys/resource.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/wait.h`, `unistd.h`

# Function tables

### atserv

Initialized at line 73. Dispatches to: [aep_packet](aep.c.md#aep_packet), [nbp_packet](nbp.c.md#nbp_packet), [rtmp_packet](rtmp.c.md#rtmp_packet), [zip_packet](zip.c.md#zip_packet)

# Functions

### atalkd_exit

```c
static void atalkd_exit(const int i)
```

Defined at lines 114 to 137.

Delete the AppleTalk addresses of the interfaces and the pid file, then exit.

The error paths and as_down exit through here. On Linux a failed SIOCDIFADDR falls back to SIOCATALKDIFADDR, and systems without SIOCDIFADDR keep the addresses.

Parameters:
* `i`: exit status

Calls: [ifconfig](main.c.md#ifconfig)

Called by: [as_down](main.c.md#as_down), [as_timer](main.c.md#as_timer), [bootaddr](main.c.md#bootaddr), [consistency](main.c.md#consistency), [main](main.c.md#main), [setaddr](main.c.md#setaddr)

Uses file-scope variables: `interfaces`, `pidfile`

### sendto_iface

```c
static ssize_t sendto_iface(struct interface *iface, int sockfd, const void *buf, size_t len, const struct sockaddr_at *dest_addr)
```

Defined at lines 155 to 177.

Send a packet out of an interface, and log only the first failure of a run.

IFACE_ERROR marks an interface whose last send failed and keeps the log quiet until a send succeeds. An interface that has gone away stays in the list until atalkd restarts.

Parameters:
* `iface`: interface to send on, its IFACE_ERROR flag follows the result
* `sockfd`: socket of the port
* `buf`: packet
* `len`: packet length
* `dest_addr`: destination address

Returns: the sendto() result

Called by: [as_timer](main.c.md#as_timer)

### as_timer

```c
static void as_timer(int sig)
```

Defined at lines 212 to 740.

Run the routing work of atalkd on each SIGALRM, every 10 seconds.

An interface with an address but no configuration sends an RTMP request in phase 1, or a ZIP GetNetInfo in phase 2, for 3 ticks. After that it counts as having no router. A router interface without seed information starts over and waits for a router. A seed router takes its configured zones. Any other interface runs without a router, in phase 2 on the startup net range. In both of those cases the next interface boots.

Each tick ages the gateways and their routes. A gateway that reaches RTMPTAB_BAD goes down, and the call frees or poisons its routes. When the last gateway of a non-seed interface goes down, the interface drops its zones and returns to the no-router state. In phase 2 its net range becomes 0-fffe.

Each gateway gets a ZIP query that asks for the interface zone and for each route in use that lacks zones. After 3 queries for a net, the call logs that the zone is unknown. With ziptimeout set, it stops asking.

A router interface announces its net range in RTMP data. The same packets carry the routes of the configured router interfaces with known zones. Split horizon leaves out the routes learned on the interface, unless it has -router.

At STABLE with no new RTMP data and no ZIP queries, atalkd is stable and writes its configuration with writeconf. At STABLEANYWAY it stops waiting without writing. The first time atalkd is stable outside debug mode, it stops itself with SIGSTOP. The parent waits for that stop, continues the child with SIGCONT, and exits, so the start returns once AppleTalk is up.

Parameters:
* `sig`: signal number, unused

Calls: [addzone](zip.c.md#addzone), [atalkd_exit](main.c.md#atalkd_exit), [bootaddr](main.c.md#bootaddr), [consistency](main.c.md#consistency), [looproute](rtmp.c.md#looproute), [rtmp_delzonemap](rtmp.c.md#rtmp_delzonemap), [rtmp_free](rtmp.c.md#rtmp_free), [rtmp_packet](rtmp.c.md#rtmp_packet), [rtmp_replace](rtmp.c.md#rtmp_replace), [rtmp_request](rtmp.c.md#rtmp_request), [sendto_iface](main.c.md#sendto_iface), [setaddr](main.c.md#setaddr), [writeconf](config.c.md#writeconf), [zip_getnetinfo](zip.c.md#zip_getnetinfo), [zip_packet](zip.c.md#zip_packet)

Called by: [main](main.c.md#main)

Uses file-scope variables: `ciface`, `configfile`, `debug`, `interfaces`, `newrtmpdata`, `noparent`, `stable`, `stabletimer`, `ziptimeout`

### consistency

```c
static void consistency()
```

Defined at lines 750 to 782.

Check the zone table against the routes in debug mode, and exit on a mismatch.

Each route of a zone has to be in use or have no gateway, and its zone list has to hold that zone. A mismatch logs the route and calls atalkd_exit(1).

Calls: [atalkd_exit](main.c.md#atalkd_exit)

Called by: [as_timer](main.c.md#as_timer), [main](main.c.md#main)

### as_debug

```c
static void as_debug(int sig)
```

Defined at lines 798 to 923.

Dump the interfaces, gateways, and routes to a debug file on SIGUSR1.

The file is atalkd.PID.debug in [tmpdir()](../../libatalk/util/unix.c.md#tmpdir). It lists each interface with its address, flags, net range, and zones, then each gateway with its state and routes. The flag letters are:

* 1, 2: phase 1, phase 2
* R, S, D: -router, -seed, -dontroute
* A, C, N, L: address set, configured, no router, loopback route
* q, z, x, i on routes: ZIP query pending, zones known, extended, in use

Parameters:
* `sig`: signal number, unused

Calls: [tmpdir](../../libatalk/util/unix.c.md#tmpdir)

Called by: [main](main.c.md#main)

Uses file-scope variables: `interfaces`

### as_down

```c
static void as_down(int sig)
```

Defined at lines 935 to 967.

Delete the routes of atalkd and exit on SIGTERM.

The call deletes the kernel route of each route in use and the loopback route of each interface. Then atalkd_exit(0) deletes the interface addresses and the pid file.

Parameters:
* `sig`: signal number, unused

Calls: [atalkd_exit](main.c.md#atalkd_exit), [gateroute](rtmp.c.md#gateroute), [looproute](rtmp.c.md#looproute)

Called by: [main](main.c.md#main)

Uses file-scope variables: `interfaces`

### main

```c
int main(int ac, char **av)
```

Defined at lines 995 to 1382.

Start atalkd: read the configuration, set up the interfaces, and serve AppleTalk.

The options are -1 or -2 for the default phase, -d for debug mode, -f for the configuration file, and -P for the pid file. The -t option turns on phase 1 transition mode, and -v or -V prints the version. The loopback interface comes first, then the interfaces of readconf, or of getifconf when readconf fails.

Before the fork, main applies the default phase, marks the router interfaces, and fills in default net ranges. It also deletes the old interface addresses. A single seed interface needs -router, and a list without a real interface exits. The parent waits until the child stops itself once AppleTalk is stable, continues it with SIGCONT, and exits 0. Debug mode stays in the foreground.

The child boots the first interface and installs as_down for SIGTERM, as_debug for SIGUSR1, and as_timer for a 10-second SIGALRM. It then reads packets from every port and passes them to the handler of the port, with SIGALRM and SIGUSR1 blocked.

Parameters:
* `ac`: argument count
* `av`: argument vector

Returns: does not return, and a startup failure exits with 1

Calls: [as_debug](main.c.md#as_debug), [as_down](main.c.md#as_down), [as_timer](main.c.md#as_timer), [atalkd_exit](main.c.md#atalkd_exit), [bootaddr](main.c.md#bootaddr), [bprint](../../libatalk/util/bprint.c.md#bprint), [consistency](main.c.md#consistency), [dumpconfig](main.c.md#dumpconfig), [getifconf](config.c.md#getifconf), [ifconfig](main.c.md#ifconfig), [newiface](config.c.md#newiface), [newrt](rtmp.c.md#newrt), [readconf](config.c.md#readconf), [server_lock](../../libatalk/util/server_lock.c.md#server_lock), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name), [set_processname](../../libatalk/util/logger.c.md#set_processname), [syslog_setup](../../libatalk/util/logger.c.md#syslog_setup)

Calls through [`atport::ap_packet`](atserv.h.md#struct-atport): no table assigns this field

Uses file-scope variables: `Packet`, `ciface`, `configfile`, `debug`, `defphase`, `fds`, `interfaces`, `nfds`, `ninterfaces`, `pidfile`, `rtfd`, `transition`, `version`

### bootaddr

```c
void bootaddr(struct interface *iface)
```

Defined at lines 1398 to 1453.

Give an interface its startup address and ask the network for its configuration.

A phase 1 interface takes net 0, or its configured net on NetBSD, and sends an RTMP request. A phase 2 interface takes its configured address and net range and sends a ZIP GetNetInfo. The loopback interface counts as configured at once and passes the boot on to the next interface.

The call marks the interface as addressed and sets stabletimer to UNSTABLE. Booting an interface that already has an address exits atalkd.

Parameters:
* `iface`: interface to boot, or NULL to do nothing

Calls: [atalkd_exit](main.c.md#atalkd_exit), [rtmp_request](rtmp.c.md#rtmp_request), [setaddr](main.c.md#setaddr), [zip_getnetinfo](zip.c.md#zip_getnetinfo)

Called by: [as_timer](main.c.md#as_timer), [main](main.c.md#main), [rtmp_config](rtmp.c.md#rtmp_config), [zip_packet](zip.c.md#zip_packet)

Uses file-scope variables: `ciface`, `stabletimer`

### setaddr

```c
void setaddr(struct interface *iface, uint8_t phase, uint16_t net, uint8_t node, uint16_t first, uint16_t last)
```

Defined at lines 1477 to 1596.

Set the AppleTalk address and net range of an interface and reopen its ports.

The first call creates the RTMP, NBP, echo, and ZIP ports of the interface, with the port numbers of the ddp services where those exist. Later calls close the open ports first. The kernel gets the address through SIOCSIFADDR and reports the final one through SIOCGIFADDR. Each port then gets a DDP socket bound to that address with SO_BROADCAST set, and the select set starts over.

Any failure exits atalkd. A failed bind removes the addresses of every interface first.

Parameters:
* `iface`: interface, receives the address and the open ports
* `phase`: IFACE_PHASE1 or IFACE_PHASE2
* `net`: network number in network byte order
* `node`: node number to ask for
* `first`: first net of the range in network byte order
* `last`: last net of the range in network byte order

Calls: [atalkd_exit](main.c.md#atalkd_exit), [ifconfig](main.c.md#ifconfig)

Called by: [as_timer](main.c.md#as_timer), [bootaddr](main.c.md#bootaddr), [rtmp_config](rtmp.c.md#rtmp_config), [zip_packet](zip.c.md#zip_packet)

Uses file-scope variables: `atservNATSERV`, `fds`, `interfaces`, `nfds`

### ifconfig

```c
int ifconfig(const char *iname, unsigned long cmd, struct sockaddr_at *sa)
```

Defined at lines 1611 to 1635.

Run an interface ioctl with an AppleTalk address.

The call opens an AppleTalk socket for the request. For SIOCGIFADDR it stores the address of the interface in sa.

Parameters:
* `iname`: interface name
* `cmd`: ioctl request, such as SIOCSIFADDR, SIOCGIFADDR, SIOCDIFADDR, or SIOCADDMULTI
* `sa`: address for the request, receives the address for SIOCGIFADDR

Returns: 0, or 1 when the socket or the ioctl fails

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [addmulti](multicast.c.md#addmulti), [atalkd_exit](main.c.md#atalkd_exit), [main](main.c.md#main), [setaddr](main.c.md#setaddr)

### dumpconfig

```c
void dumpconfig(struct interface *iface)
```

Defined at lines 1646 to 1686.

Print the configuration of an interface as an atalkd.conf line in debug mode.

The line adds the configured address as -caddr, which readconf does not accept, and leaves zone names unquoted.

Parameters:
* `iface`: interface to print

Called by: [main](main.c.md#main)

# Macros

* Undocumented: `MACCHARSET`, `PKTSZ`, `elements`

# File-scope variables

`Packet`, `atservNATSERV`, `ciface`, `configfile`, `debug`, `defphase`, `fds`, `interfaces`, `newrtmpdata`, `nfds`, `ninterfaces`, `noparent`, `pidfile`, `rtfd`, `stable`, `stabletimer`, `transition`, `version`, `ziptimeout`
