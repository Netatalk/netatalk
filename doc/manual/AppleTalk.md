# AppleTalk

## AppleTalk Transport Layer

AppleTalk is a network protocol family created by Apple which predates
TCP/IP. It contains different protocols for different uses: address
resolution, address/name mapping, service location, establishing
connections, and the like.

Netatalk implements the AppleTalk protocols to serve files over AFP and
provide other services to old Mac and Apple II clients.

A complete overview can be found inside the [developer
documentation](/appletalk.html).

### To use AppleTalk or not

You'll need the AppleTalk support built into Netatalk to provide file
services to older AFP clients not capable of using AFP over TCP. It also
enables printing services via PAP by `papd(8)`, a timeserver
`timelord(8)` for older Mac clients, and NetBoot server `a2boot(8)` for
Apple II clients.

In addition, if you are serving Classic Mac OS clients, you might
consider using AppleTalk for service propagation/location, having the
ease of use for your network clients in mind. The Apple engineers
implemented a way to easily locate an AFP server via AppleTalk but
establishing the AFP connection itself via AFP over TCP (see the
developer documentation for details on this cool feature, too).

To use the different base AppleTalk protocols with Netatalk, one has to
use `atalkd(8)`. It can also be used as an AppleTalk router to connect
different independent network segments to each other.

To use AppleTalk/atalkd, your system has to have kernel support for
AppleTalk. If missing, you will be restricted to AFP over TCP, and won't
be able to use either of the AppleTalk services described in this
chapter.

    appletalk = yes

If Netatalk has been built with AppleTalk support (pass
`-Dwith-appletalk=true` to the build system), this activates AFP over
AppleTalk.

### No AppleTalk routing

This is the most simple form, you can use AppleTalk with netatalk. In
case, you have only one network interface up and running, you haven't to
deal with atalkd's config at all: atalkd will use AppleTalk's
self-configuration features to get an AppleTalk address and to register
itself in the network automagically.

In case, you have more than one active network interface, you have to
make a decision:

#### Using only one interface

Just add the interface name (en1, le0, eth2, ... for example) to
atalkd.conf on a single line. Do only list *one* interface here.

    eth0

AppleTalk networking should be enabled on eth0 interface. All the
necessary configuration will be fetched from the network

At startup time, atalkd will add the real settings (address and network
and eventually a zone) to atalkd.conf on its own

    eth0 -phase 2 -net 0-65534 -addr 65280.166

atalkd filled in the AppleTalk settings that apply to this network
segment. A netrange of 0-65534 indicates that there is no AppleTalk
router present, so atalkd will fetch an address that matches the
following criteria: netrange from inside the so called "startup range"
65280-65533 and a node address between 142 and 255.

#### Using several interfaces

When using several interfaces you have to add them line by line
following the *-dontroute* switch in atalkd.conf.

    eth0 -dontroute eth1 -dontroute eth2 -dontroute

AppleTalk networking should be enabled on all three interfaces, but no
routing should be done between the different segments. Again, all the
necessary configuration will be fetched from the connected networks.

    eth0 -dontroute -phase 2 -net 0-65534 -addr 65280.152
    eth1 -dontroute -phase 2 -net 0-65534 -addr 65280.208
    eth2 -dontroute -phase 2 -net 1-1000 -addr 10.142 -zone "Printers"

On eth0 and eth1, there are no other routers present, so atalkd chooses
an address from within the startup range. But on eth2 there lives an
already connected AppleTalk router, publishing one zone called
"Printers" and forcing clients to assign themselves an address in a
netrange between 1 and 1000.

In this case, atalkd will handle each interface as it would be the only
active one. This can have some side effects when it comes to the point
where AFP clients want to do the magic switch from AppleTalk to TCP, so
use this with caution.

In case, you have more than one active network interface and do not take
special precautions as outlined above, then autoconfiguration of the
interfaces might fail in a situation where one of your network
interfaces is connected to a network where *no* other active AppleTalk
router is present and supplies appropriate routing settings.

For further information see `atalkd.conf(5)` and the developer
documentation.

### atalkd acting as an AppleTalk router

Several types of AppleTalk routers exist: seed, non-seed and so called
soft-seed routers.

- A seed router has its own configuration and publishes this into the
  network segments it is configured for.

- A non-seed router needs a seed router on the interface to which it is
  connected to learn the network configuration. So this type of
  AppleTalk router can work completely without manual configuration.

- A so called soft-seed router is exactly the same as a non-seed router
  except the fact, that it can also remember the configuration of a seed
  router and act as a replacement in case, the real seed router
  disappears from the net.

Netatalk's atalkd can act as both a seed and a soft-seed router, even in
a mixed mode, where it acts on one interface in this way and on the
other in another.

If you leave your atalkd.conf completely empty or simply add all active
interfaces line by line without using seed settings (atalkd will act
identically in both cases), then atalkd is forced to act as a soft-seed
router on each interface, so it will fail on the first interface, where
no seed router is accessible to fetch routing information from.

In this case, other services, that depend on atalkd, might also fail.

So you should have atalkd act as a seed router on one or all active
interfaces. A seed router has to supply information about:

- The specific netrange on this segment

- Its own AppleTalk address

- The zones (one to many) available in this segment

- The so called "default zone" for this segment

> **WARNING**

> Unless you are the network admin yourself, consider asking them before
changing anything related to AppleTalk routing, as changing these
settings might have side effects for all of your AppleTalk network
clients!

In an AppleTalk network netranges have to be unique and must not overlap
each other. Fortunately netatalk's atalkd is polite enough to check
whether your settings are in conflict with already existing ones on the
net. In such a case it simply discards your settings and tries to adapt
the already established ones on the net (if in doubt, always check
syslog for details).

Netranges, you can use, include pretty small ones, e.g. 42-42, to very
large ones, e.g. 1-65279 - the latter one representing the maximum. In
routed environments you can use any numbers in the range between 1 and
65279 unless they do not overlap with settings of other connected
subnets.

The own AppleTalk address consists of a net part and a node part (the
former 16 bit, the latter 8 bit, for example 12057.143). Apple
recommends using node addresses of 128 or above for servers, letting
client Macs assign themselves an address faster (as they will primarily
search for a node address within 1-127 in the supplied netrange). As we
don't want to get in conflict with Apple servers, we prefer using node
addresses of 142 or above.

AppleTalk zones have *nothing* to do with physical networks. They're
just a hint for your client's convenience, letting them locate network
resources in a more comfortable/faster way. You can either use one zone
name across multiple physical segments as well as more than one zone
name on a single segment (and various combinations of this).

So all you have to do is to *draw a network chart* containing the
physical segments, the netranges you want to assign to each one, the
zone names you want to publish in which segments and the default zone
per segment (this is always the first zone name, you supply with the
*-zone* switch in atalkd.conf).

If you finished the steps outlined above, you might want to edit
atalkd.conf to fit your needs.

You'll have to set the following options in atalkd.conf:

- -net (use reasonable values between 1-65279 for each interface)

  In case, this value is suppressed but -addr is present, the netrange
  from this specific address will be used

- -addr (the net part must match the -net settings if present, the node
  address should be between 142 and 255)

- -zone (can be used multiple times in one single line, the first entry
  is the default zone)

Note that you are able to set up "zone mapping", that means publishing
exactly the same zone name on all AppleTalk segments, as well as
providing more than one single zone name per interface. Dumb AppleTalk
devices, like LaserWriters, will always register themselves in the
default zone (the first zone entry you use in atalkd.conf per
interface), more intelligent ones will have the ability to choose one
specific zone via a user interface.

    eth0 -seed -phase 2 -net 1-1000 -addr 1000.142 -zone "Printers" -zone "Spoolers"
    eth1 -seed -phase 2 -net 1001-2000 -addr 2000.142 -zone "Macs" -zone "Servers"

The settings for eth0 force AppleTalk devices within the connected
network to assign themselves an address in the netrange 1-1000. Two zone
names are published into this segment, "Printers" being the so called
"standard zone", forcing dumb AppleTalk devices like Laser printers to
show up automatically into this zone. AppleTalk printer queues supplied
by netatalk's papd can be registered into the zone "Spoolers" simply by
adjusting the settings in `papd.conf(5)`. On eth1 we use the different
and non-overlapping netrange 1001-2000, set the default zone to "Macs"
and publish a fourth zone name "Servers".

    eth0 -seed -phase 2 -net 1-1000 -addr 1000.142 -zone "foo"
    lo0 -phase 1 -net 1 -addr 1.142 -zone "foo"

We use the same network settings as in the example above but let atalkd
publish the same zone name on both segments. As the same zone name will
be used on all segments of the AppleTalk network no zone names will show
up at all... but AppleTalk routing will still be active. In this case,
we connect a so called "non-extended" LocalTalk network (phase 1) to an
EtherTalk "extended" network (phase 2) transparently.

    eth0 eth1 eth2

As we have more than one interface, atalkd will try to act as an
AppleTalk router between both segments. As we don't supply any network
configuration on our own we depend on the availability of seed routers
in every connected segment. If only one segment is without such an
available seed router the whole thing will fail.

    eth0 -phase 2 -net 10-10 -addr 10.166 -zone "Parking"
    eth1 -phase 2 -net 10000-11000 -addr 10324.151 -zone "No Parking" -zone "Parking"
    eth2 -phase 2 -net 65279-65279 -addr 65279.142 -zone "Parking" -zone "No Parking"

In this case, active seed routers are present in all three connected
networks, so atalkd was able to fetch the network configuration from
them and, since the settings do not conflict, act as a soft-seed router
from now on between the segments. So even in case, all of the three seed
routers would disappear from the net, atalkd would still supply the
connected network with the network configuration once learned from them.
Only in case, atalkd would be restarted afterwards, the routing
information will be lost (as we're not acting as seed router).

    eth0
    eth1 -seed -phase 2 -net 99-100 -addr 99.200 -zone "Testing"

In case in the network connected to eth0 lives no active seed router or
one with a mismatching configuration (e.g. an overlapping netrange of
1-200) atalkd will fail. Otherwise it will fetch the configuration from
this machine and will route between eth0 and eth1, on the latter acting
as a seed router itself.

By the way: It is perfectly legal to have more than one seed router
connected to a network segment. But in this case, you should take care
that the configuration of all connected routers is exactly the same
regarding netranges, published zone names and also the "standard zone"
per segment

## Printing

Netatalk can act both as a PAP client to
access AppleTalk-capable printers, and as a PAP server. The former by
using the `pap(1)` utility and the latter by starting the `papd(8)` service.

The "Printer Access Protocol" as part of the AppleTalk protocol suite is
a fully 8 bit aware and bidirectional printing protocol, developed by
Apple in 1985. *8 bit aware* means that the whole set of bytes can be
used for printing (binary encoding). This has been a great advantage
compared with other protocols like Adobe's Standard Protocol to drive
serial and parallel PostScript printers (compare with [Adobe TechNote
5009](https://web.archive.org/web/20041022165533/http://partners.adobe.com/asn/tech/ps/specifications.jsp))
or LPR which made only use of the lower
128 bytes for printing because the 8th bit has been reserved for control
codes.

*Bidirectional* means that printing source (usually a Macintosh
computer) and destination (a printer or spooler implementation)
communicate about both destination's capabilities via feature queries
(compare with [Adobe TechNote
5133](https://web.archive.org/web/20041022123331/http://partners.adobe.com/asn/tech/ps/archive.jsp))
and device status. This allows the LaserWriter driver on the Macintosh
to generate appropriate device specific PostScript code (color or b/w,
only embedding needed fonts, and so on) on the one hand and
notifications about the printing process or problems (paper jam for
example) on the other.

### Setting up the PAP print server

Netatalk's `papd` is able to provide AppleTalk printing services for
Macintoshes or, to be more precise, PAP clients in general. Netatalk
does not contain a full-blown spooler implementation itself, papd only
handles the bidirectional communication and submittance of printjobs
from PAP clients. So normally one needs to integrate papd with a Unix
printing system like e.g. classic SysV lpd, BSD lpr, LPRng,
CUPS or the like.

Since it is so important to answer the client's feature queries
correctly, how does papd achieve this? By parsing the relevant keywords
of the assigned PPD file. When using
CUPS, papd will attempt to generate a PPD file on the fly by querying
the printer via IPP. With other spoolers, choosing the correct PPD is
important to be able to print.

Netatalk formerly had built-in support for System V lpd printing in a
way that papd saved the printed job directly into the spooldir and calls
`lpd` afterwards, to pick up the file and do the rest. Due to
incompatibilities with many lpd implementations the normal behaviour was
to print directly into a pipe instead of specifying a printer by name
and using lpd interaction. As of Netatalk 2.0, another alternative has
been implemented: direct interaction with CUPS (Note: when CUPS support
is compiled in, then the SysV lpd support doesn't work at all). Detailed
examples can be found in the `papd.conf(5)` manual page.

#### Integrating papd with SysV lpd

Unless CUPS support has been compiled in (which is default from Netatalk
2.0 on) one simply defines the lpd queue in question by setting the `pr`
parameter to the queue name. If no `pr` parameter is set, the default
printer will be used.

#### Using pipes with papd

An alternative to the technique outlined above is to direct papd's
output via a pipe into another program. Using this mechanism almost all
printing systems can be driven.

#### Using direct CUPS support

Starting with Netatalk 2.0, direct CUPS integration is available. In
this case, defining only a queue name as `pr` parameter won't invoke the
SysV lpd daemon but uses CUPS instead. Unless a specific PPD has been
assigned using the `pd` switch, the PPD configured in CUPS will be used
by `papd`, too.

There exists one special share named *cupsautoadd*. If this is present
in papd.conf, then all available CUPS queues will be served
automagically using the parameters assigned to this global share. But
subsequent printer definitions can be used to override these global
settings for individual spoolers.

    cupsautoadd:op=root:

### Using AppleTalk printers

Netatalk's `papstatus(8)` can be used to query AppleTalk printers, `pap(1)` to print
to them.

`pap` can be used stand-alone or as part of an output filter or a CUPS
backend (which is the preferred method
since one does not have to deal with all the options).

    pap -p"ColorLaserWriter 16/600@*" /usr/share/doc/gs/examples/tiger.ps

The file `/usr/share/doc/gs/examples/tiger.ps` is sent to a printer
called "ColorLaserWriter 16/600" in the standard zone "\*". The device
type is "LaserWriter" (can be suppressed since it is the default).

    gs -q -dNOPAUSE -sDEVICE=cdjcolor -sOutputFile=test.ps | pap -E

GhostScript is used to convert a PostScript job to PCL3 output suitable
for a Color DeskWriter. Since no file has been supplied on the command
line, `pap` reads the data from stdin. The printer's address will be
read from the `.paprc` file in the same directory, `pap` will be called
(in our example simply containing "Color
DeskWriter:DeskWriter@Printers"). The `-E` switch forces `pap` to not
wait for an EOF from the printer.

## Time Services

### Timelord

`timelord`, an AppleTalk based time
server, is useful for automatically synchronizing the system time on
older Macintosh or Apple II clients that do not support
NTP.

Netatalk's `timelord` is compatible with the tardis client for Macintosh
developed at the [University of
Melbourne.](https://web.archive.org/web/20010303220117/http://www.cs.mu.oz.au/appletalk/readmes/TMLD.README.html)

For further information please have a look at the `timelord(8)` manual
page.

## NetBoot Services

### Apple 2 NetBooting

`a2boot` is a service that allows you to
boot an Apple //e or Apple IIGS into ProDOS or GS/OS through an AFP
volume served by Netatalk.

You need to supply the appropriate boot blocks and system files provided
by Apple yourself.

For further information please have a look at the `a2boot(8)` manual
page.
