# Name

timelord — Macintosh time server daemon

# Synopsis

**timelord** [-d] [-l] [-n *nbpname*]

**timelord** [-v | -V]

# Description

**timelord** is a simple time server for Macintosh computers that use the
*tardis* client. It has the same capabilities as the "Timelord" CDEV for
Macintosh.

**tardis** is implemented as a Chooser extension. In the Chooser, choose
the **timelord** instance to synchronize the Mac's system time with. Once
synchronized, **tardis** will automatically synchronize with the server at
boot, or scheduled at regular intervals while the Mac is running (the
latter requires tardis 1.4).

# Options

**-d**

> Debug mode, i.e. don't disassociate from controlling TTY.

**-l**

> Return the time zone adjusted localtime of the server. The default
behavior without this option is GMT.

**-n** *nbpname*

> Register this server as *nbpname*. This defaults to the hostname.

**-v** | **-V**

> Print version information and exit.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
