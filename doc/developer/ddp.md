AppleTalk Transport Layer
=========================

AppleTalk is a network protocol family created by Apple which predates
TCP/IP. It contains different protocols for different uses: address
resolution, address/name mapping, service location, establishing
connections, and the like.

The protocol family is also often collectively referred to as _DDP_,
which stands for Datagram Delivery Protocol.
DDP is the core transport layer protocol of AppleTalk.

Netatalk implements the AppleTalk protocols to serve files over AFP and
provide other services to old Mac and Apple II clients.

It supports EtherTalk Phase I and II, RTMP, NBP, ZIP, AEP, ATP,
PAP, and ASP, while expecting the host OS kernel to supply DDP.

* DDP is a socket to socket protocol that all other AppleTalk protocols
  are built on top of.

* ATP, ASP, and NBP are implemented as statically linked libraries
  in Netatalk's _libatalk_ shared library.

* The **atalkd** daemon implements RTMP, ZIP, and AEP.

* The **papd** daemon implements PAP, allowing Mac clients to spool
  to a Unix print spooler on the netatalk host computer.

Linux
-----

The Linux kernel has had AppleTalk support since version 1.3.0, released in June 1995.
See the [Linux 1.3.0 Changes file](https://www.kernel.org/pub/linux/kernel/v1.3/).

The source code for the AppleTalk driver lives under
[net/appletalk](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/net/appletalk)
in the Linux kernel source tree.

NetBSD
------

Kernel support for AppleTalk appeared in NetBSD 1.3 in April 1997.
See the [NetBSD 1.3 release notes](http://www.netbsd.org/changes/changes-1.3.html)

The NetBSD kernel comes with an AppleTalk kernel module, confusingly named _netatalk_,
located under [sys/netatalk](https://cvsweb.netbsd.org/bsdweb.cgi/src/sys/netatalk/) in the kernel source tree.

Other BSDs
----------

Kernel support for AppleTalk appears in FreeBSD 2.2-current
dated after 12 September 1996, as well as OpenBSD 2.2,
or openbsd-current dated after Aug 1, 1997.

However, both distributions removed the AppleTalk kernel modules in the 2010s.

Solaris
-------

Historical versions of Netatalk (until v2.3) distributed code for a Solaris STREAMS module that implements AppleTalk.

The source code is located under
[sys/solaris](https://github.com/Netatalk/netatalk/tree/branch-netatalk-2-3/sys/solaris)
in the netatalk source tree.
It is written for the SPARC architecture.

Generic
-------

If you would like AppleTalk support on another operating system,
you will need either need a kernel module for your operating system,
or a userland AppleTalk stack.

Look at the above Solaris STREAMS module if your operating system supports that framework.

Otherwise, look at the ddp code in NetBSD if your operating system is BSDish in nature.

If your operating system looks different than these two cases,
you'll have to roll your own implementation.
