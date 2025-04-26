# Name

atalkd — userland AppleTalk network manager daemon

# Synopsis

**atalkd** [-f *configfile*] [-P *pidfile*] [-1] [-2] [-d] [-t]

**atalkd** [-v | -V]

# Description

**atalkd** is responsible for all user level AppleTalk network management.
This includes routing, name registration and lookup, zone lookup, and
the AppleTalk Echo Protocol (similar to **ping**(8)). Specifically, this
corresponds to the RTMP, NBP, ZIP, and AEP protocols in the AppleTalk
protocol family.

The init system of your OS will typically start the **atalkd** daemon at
bootup. The daemon first reads from its configuration file,
**atalkd.conf**. If there is no configuration file, or if no interfaces
have been defined, **atalkd** will attempt to configure all available
interfaces and will create a configuration file. See **atalkd.conf**(5)
for details on the configuration file format.

# Options

**-1**

> Forces AppleTalk Phase 1.

**-2**

> Forces AppleTalk Phase 2.

**-d**

> Do not disassociate daemon from terminal. Writes some additional
debugging information to stdout.

**-f** *configfile*

> Consult *configfile* instead of **atalkd.conf** for the configuration
information.

**-P** *pidfile*

> Specifies the file in which **atalkd** stores its process id.

**-t**

> Turns on transition routing.

**-v** | **-V**

> Print version information and exit.

# Routing

If you are connecting an **atalkd** router to an existing AppleTalk
network, you should first contact your local network administrators to
obtain appropriate network addresses.

**atalkd** can provide routing between interfaces by configuring multiple
interfaces. Each interface must be assigned a unique
net-range between 1 and 65279 (0 and
65535 are illegal, and addresses between 65280 and 65534 are reserved
for startup). It is best to choose the smallest useful net-range, i.e.
if you have three machines on a LAN, choose a net-range below 1000. Each
net-range may have an arbitrary list of zones associated with it.

Note that **atalkd** automatically acts as a router if there is more than
one interface, and no other configurations are present.

# Files

*atalkd.conf* configuration file

# See Also

atalkd.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
