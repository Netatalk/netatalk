# Name

timelord — AppleTalk time server daemon

# Synopsis

**timelord** [-d] [-l] [-n *nbpname*]

**timelord** [-v | -V]

# Description

**timelord** is a simple time server for Macintosh and Apple II computers that use
a client called *tardis*. It has the same capabilities as the *Timelord* CDEV for
Macintosh, created at the University of Melbourne.

The **tardis** client for Macintosh is implemented as a Chooser extension.
In the Chooser, pick the **timelord** instance to synchronize the Mac's system time with.
Once synchronized, **tardis** will automatically synchronize with the server at boot,
or scheduled at regular intervals while the Mac is running (the latter requires tardis 1.4).

A **tardis** client for Apple II computers is also available,
as part of the MG's DaveX Utilities suite.

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
