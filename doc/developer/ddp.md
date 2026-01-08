AppleTalk Protocol Family
=========================

AppleTalk is a network protocol family for Macintosh and Apple II computers,
created by Apple in 1985 at a time when TCP/IP was not yet widely adopted.
It contains different protocols for different uses:
address resolution, address/name mapping, service location,
establishing connections, and the like.

The AppleTalk protocols are layered above the Datagram Delivery Protocol (DDP),
and using AppleTalk address format.
The AppleTalk family may provide SOCK_STREAM (ADSP), SOCK_DGRAM (DDP),
SOCK_RDM (ATP), and SOCK_SEQPACKET (ASP).

Netatalk implements the AppleTalk protocols to serve files over AFP and
provide other services to very old Mac and Apple II clients.

It supports EtherTalk Phase I and II, RTMP, NBP, ZIP, AEP, ATP,
PAP, and ASP, while expecting the host OS kernel to supply DDP.

* DDP is a socket to socket protocol that all other AppleTalk protocols
  are built on top of.
* ATP, ASP, and NBP are implemented as statically linked libraries
  in Netatalk's *libatalk* shared library.
* The **atalkd** daemon implements RTMP, ZIP, and AEP.
* The **papd** daemon implements PAP, allowing Mac clients to spool
  to a Unix print spooler or CUPS on the netatalk host computer.

Addressing
----------

AppleTalk addresses are three byte quantities, stored in network byte
order. The include file <netatalk/at.h\> defines the AppleTalk
address format.

Sockets in the AppleTalk protocol family use the following address
structure:

    struct sockaddr_at {
        short sat_family;
        unsigned char sat_port;
        struct at_addr sat_addr;
        char sat_zero[8];
    };

The port of a socket may be set with **bind**. The node for *bind* must
always be ATADDR_ANYNODE: "this node." The net may be
ATADDR_ANYNET or ATADDR_LATENET. ATADDR_ANYNET corresponds to the
machine's "primary" address (the first configured). ATADDR_LATENET
causes the address in outgoing packets to be determined when a packet is
sent, i.e. determined late. ATADDR_LATENET is equivalent to opening
one socket for each network interface. The port of a socket and either
the primary address or ATADDR_LATENET are returned with
**getsockname**.

AppleTalk address parsing
-------------------------

The **atalk_aton()** routine converts an ASCII representation of an
AppleTalk address to a format appropriate for system calls. Acceptable
ASCII representations include both hex and base 10, in triples or
doubles. For instance, the address \`0x1f6b.77' has a network part of
\`8043' and a node part of \`119'. This same address could be written
\`8043.119', \`31.107.119', or \`0x1f.6b.77'. If the address is in hex
and the first digit is one of \`A-F', a leading \`0x' is redundant.

NBP name parsing
----------------

**nbp_name()** parses user supplied names into their component object,
type, and zone. *obj*, *type*, and *zone* should be passed by reference,
and should point to the caller's default values. **nbp_name()** will
change the pointers to the parsed-out values. *name* is of the form
object:type\@zone, where each of *object*, *:type*, and *\@zone*
replace *obj*, *type*, and *zone*, respectively. *type* must be
proceeded by \`:', and *zone* must be preceded by \`\@'.

DDP Implementations
===================

Linux
-----

The Linux kernel has had DDP support since version 1.3.0, released in June 1995.
See the [Linux 1.3.0 Changes file](https://www.kernel.org/pub/linux/kernel/v1.3/).

The source code for the *appletalk* driver lives under
[net/appletalk](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/net/appletalk)
in the Linux kernel source tree.

NetBSD
------

Kernel support for DDP appeared in NetBSD 1.3 in April 1997.
See the [NetBSD 1.3 release notes](http://www.netbsd.org/changes/changes-1.3.html)

The NetBSD kernel module is confusingly named *netatalk* and is located under
[sys/netatalk](https://cvsweb.netbsd.org/bsdweb.cgi/src/sys/netatalk/) in the NetBSD source tree.

Solaris
-------

Historical versions of Netatalk (until v2.3) distributed code for a Solaris STREAMS module that implements DDP.

The source code is located under
[sys/solaris](https://github.com/Netatalk/netatalk/tree/branch-netatalk-2-3/sys/solaris)
in the netatalk source tree.
It is written for the SPARC architecture.

Generic
-------

If you would like DDP support on another operating system,
you will need either need a kernel module for your operating system,
or a userland AppleTalk stack.

Look at the above Solaris STREAMS module if your operating system supports that framework.
Otherwise, look at the DDP code in NetBSD if your operating system is BSDish in nature.

If your operating system looks different than these two cases,
you'll have to roll your own implementation.
