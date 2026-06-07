# Introduction to Netatalk

Netatalk is an Open Source software package that can be used to turn a
\*NIX machine into a performant and light-weight AFP file server.

AFP (Apple Filing Protocol) is a network file system protocol originally
developed by Apple for use in their Macintosh computers. It was the
primary file sharing protocol for Mac OS for the better part of 30 years.
AFP 3.4 is the latest version of the protocol, introduced with Mac OS X 10.8
Mountain Lion. Netatalk can speak AFP 2 and AFP 3, allowing it to support
a wide range of clients from the latest macOS to very old Classic Mac OS
and Apple II machines.

A number of 3rd party AFP client implementations exist for other operating systems,
including the _Netatalk Client_, which is an improved fork of _afpfs-ng_ that
is available for Linux, FreeBSD, OpenBSD, NetBSD, and macOS.

Using Netatalk's AFP 3.4 compliant file server leads to noticeably
higher transmission speeds for macOS clients compared to Samba/NFS,
while providing users with the best possible Mac-like user experience.
It can read and write Mac metadata - macOS extended file attributes as
well as Classic Mac OS resource forks - facilitating mixed environments
of new and old Macs.

Netatalk ships with range of capabilities to accommodate most deployment
environments, including Kerberos, ACLs and LDAP. Modern macOS features
such as Zeroconf (Bonjour) service discovery, Time Machine backups,
and Spotlight indexed search are provided.

## AppleTalk Services

Netatalk provides a modern TCP/IP transport layer, as well as a traditional
AppleTalk transport layer for very old Mac and Apple II clients.

For AppleTalk networks with legacy Macs and Apple IIs, Netatalk provides
a print server, time server, and Apple II network boot server. The print
server is fully integrated with CUPS, allowing old Macs to discover and
print to a modern CUPS/AirPrint compatible printer on the network.
Netatalk can also provide a MacIP gateway, allowing AppleTalk-only
Macintosh clients to reach TCP/IP networks.

Additionally, Netatalk can be configured as an AppleTalk router through
its AppleTalk Network Management daemon, providing both segmentation and
zone names in traditional Macintosh networks.
