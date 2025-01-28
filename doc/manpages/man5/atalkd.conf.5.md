# Name

atalkd.conf — Configuration file used by atalkd to configure the interfaces used by AppleTalk

# Description

**atalkd.conf** is the configuration file used by atalkd to configure the
Appletalk interfaces and their behavior

Any line not prefixed with *\#* is interpreted. Each interface has be
configured on an uninterrupted line, with no support for split lines.
The configuration line format is:

`interface [ -seed ] [ -phase <number> ] [ -net <net-range> ] [ -addr <address> ] [ -zone <zonename> ] ...`

The simplest case is to have either no atalkd.conf, or to have one that
has no active lines. In this case, atalkd will auto-discover the local
interfaces on the machine and write to the atalkd.conf file, creating it
if one does not exist.

The interface is the network interface that this to work over, such as
*eth0* for Linux, or *le0* for Solaris.

Note that all fields except the interface are optional. The loopback
interface is configured automatically. If **-seed** is specified, all
other fields must be present. Also, **atalkd** will exit during startup if
a router disagrees with its seed information. If **-seed** is not given,
all other information may be overridden during auto-configuration. If no
**-phase** option is given, the default phase as given on the command line
is used (the default is 2). If **-addr** is given and **-net** is not, a
net-range of one is assumed.

The first -zone directive for each interface is the "default" zone.
Under Phase 1, there is only one zone. Under Phase 2, all routers on the
network are configured with the default zone and must agree. **atalkd**
maps "*" to the default zone of the first interface. Note: The
default zone for a machine is determined by the configuration of the
local routers; to appear in a non-default zone, each service, e.g.
**afpd**, must individually specify the desired zone. See also
**nbp_name**(3).

The possible options and their meanings are:

**-addr** net.node

> Allows specification of the net and node numbers for this interface,
specified in AppleTalk numbering format (example: **-addr 66.6**).

**-dontroute**

> Disables AppleTalk routing. It is the inverse of **-router**.

**-net** first[-last]

> Allows the available net to be set, optionally as a range.

**-phase** ( 1 | 2 )

> Specifies the AppleTalk phase that this interface is to use (either
Phase 1 or Phase 2).

**-router**

> Seed an AppleTalk router on a single interface. The inverse option is
**-dontroute**. Akin to **-seed**, but allows single interface routing.

**-seed**

> Seed an AppleTalk router. This requires two or more interfaces to be
configured. If you have a single network interface, use **-router**
instead. It also causes all missing arguments to be automagically
configured from the network.

**-zone** zonename

> Specifies a specific zone that this interface should appear on (example:
**-zone "Parking Lot"**). Please note that zones with spaces and other
special characters should be enclosed in quotation marks.

# Examples

Single interface on Solaris with auto-detected parameters.

       le0

The same on Linux.

       eth0

Below is an example configuration file from a Sun 4/40. The machine has
two interfaces, "le0" and "le1". The "le0" interface is
configured automatically from other routers on the network. The machine
is the only router for the "le1" interface.

       le0
       le1 -seed -net 9461-9471 -zone netatalk -zone Argus

# See Also

atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
