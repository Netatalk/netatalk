# Introduction to Netatalk

Netatalk is an Open Source software package that can be used to turn a
\*NIX machine into a performant and light-weight file server.
Any computer with an AFP client can connect to a Netatalk server.
AFP clients include macOS, Classic Mac OS, Apple II ProDOS or GS/OS,
and other operating systems with 3rd party AFP client software.

It provides a modern TCP/IP transport layer, as well as a traditional
AppleTalk transport layer for very old Mac and Apple II clients.

Using Netatalk's AFP 3.4 compliant file-server leads to noticeably
higher transmission speeds for macOS clients compared to Samba/NFS,
while providing users with the best possible Mac-like user experience.
It can read and write Mac metadata - macOS extended file attributes as
well as Classic Mac OS resource forks - facilitating mixed environments
of new and old Macs.

Netatalk ships with range of capabilities to accommodate most deployment
environments, including Kerberos, ACLs and LDAP. Modern macOS features
such as Zeroconf (Bonjour) service discovery, Time Machine backups,
and Spotlight indexed search are provided.

For AppleTalk networks with legacy Macs and Apple IIs, Netatalk provides
a print server, time server, and Apple II network boot server. The print
server is fully integrated with CUPS, allowing old Macs to discover and
print to a modern CUPS/AirPrint compatible printer on the network.

Additionally, Netatalk can be configured as an AppleTalk router through
its AppleTalk Network Management daemon, providing both segmentation and
zone names in traditional Macintosh networks.
