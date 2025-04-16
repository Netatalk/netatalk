# Name

macipgw — MacIP Gateway Daemon

# Synopsis

**macipgw** [-d debugclass] [-f configfile] [-n nameserver] [-u unprivileged-user] [-z zone] [network] [netmask]

**macipgw** [ -v | -V ]

# Description

**macipgw** provides IP connectivity for devices connected through an
AppleTalk-only network, i.e. LocalTalk or Apple Remote Access (ARA).
**macipgw** is normally started out of */etc/rc*.

MacIP (formerly known as KIP) is a protocol that allows the
encapsulation of IP packets in AppleTalk packets. **macipgw** registers
itself as a MacIP gateway on the AppleTalk network and configures and
brings up a tunnel interface (see **tun**(4)). It then forwards IP packets
received from the AppleTalk network to the kernel through the tunnel
interface. Likewise, IP packets received through the tunnel are
forwarded to the AppleTalk device registered for the destination IP
address.

Unlike other MacIP gateways, **macipgw** needs its own IP (sub-)net to
operate, and does not use proxy-ARP for the addresses registered. The
gateway will always use the first address in the network for the local
address, i.e. 192.168.1.1 for the network 192.168.1.0/24.

If present, **macipgw** reads configuration options from
*/usr/etc/macipgw.conf* (or equivalent pkgconf path.) Command line
options will take precedence over configuration file options. See below
for an example.

**macipgw** will log operational messages through **syslog**(3) under the
facility *LOG_DAEMON*.

# Options

**-d** <debugclass\>

> Specifies that the daemon should not fork, and that a trace of all
actions be written to *stdout*. See the source code for useful values of
debugclass.

**-f** <configfile\>

> Consult <configfile\> instead of *macipgw.conf* for the configuration
information.

**-n** <nameserver\>

> Specifies the IP address of a DNS name server the AppleTalk devices
connected through the gateway should use.

**-u** <unprivileged-user\>

> Drop root privileges and change to user unprivileged-user after the
server has started.

**-z** <zone\>

> Specifies that **macipgw** should register in zone instead of the default
zone.

**-v** | **-V**

> Show version information and exit.

<network\>

> Specifies the network number to use for the clients.

<netmask\>

> Specifies the netmask for the network.

# Examples

## Example: macipgw invocation

    /usr/local/libexec/macipgw -n 192.168.1.1 -z "Remote Users" 192.168.1.0 255.255.255.0

Starts **macipgw**, assigning the Class C network 192.168.1.0 for devices
connected through the gateway, specifying that the system **macipgw** is
running on can be used as a name server, and that it should register in
the zone Remote Users.

## Example: macipgw.conf configuration file

    [Global]
    network = 192.168.151.0
    netmask = 255.255.255.0
    nameserver = 8.8.8.8
    zone = My Zone
    unprivileged user = foobar

# See Also

tun(4), ip(4), atalkd(8), syslog(3), syslogd(8)

# Bugs

No information besides the log messages is available on which AppleTalk
devices are using the gateway.

# Author

Stefan Bethke <Stefan.Bethke@Hanse.DE\>
