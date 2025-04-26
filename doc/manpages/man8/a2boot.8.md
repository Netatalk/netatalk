# Name

a2boot — Apple II netboot server daemon

# Synopsis

**a2boot** [-d] [-n *nbpname*]

**a2boot** [-v | -V]

# Description

**a2boot** is a netboot server for Apple IIe and IIGS computers. It allows
compatible clients to boot into ProDOS or GS/OS over an AppleTalk
network. This is functionally comparable to the Apple II netboot
software included in early versions of AppleShare File Server for
Macintosh.

When running, the daemon will create the following hard-coded AFP
volumes.

- Apple //e Boot

- Apple //gs

- ProDOS16 Image

These volumes need to be populated with the appropriate ProDOS or GS/OS
system files in order to function as boot volumes. For more information,
see the documentation for AppleShare File Server for Macintosh.

# Options

**-d**

> Debug mode, i.e. don't disassociate from controlling TTY.

**-n** *nbpname*

> Register this server as *nbpname*. This defaults to the hostname.

**-v** | **-V**

> Print version information and exit.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
