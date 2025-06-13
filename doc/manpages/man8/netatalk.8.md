# Name

netatalk — Netatalk AFP server service controller daemon

# Synopsis

**netatalk** [-F *configfile*]

**netatalk** [-v | -V]

# Description

**netatalk** is the daemon used to control the Netatalk AFP file server,
which is made up of multiple forking daemons.
For most deployments you would use **netatalk** for centralized control
rather than launching and stopping each daemon individually.
The **netatalk** daemon is normally started at boot time by an init system.

The controller daemon will launch the AFP daemon **afpd**
and the CNID meta daemon **cnid_metad**. The latter will in turn launch
the CNID database daemon **cnid_dbd**.

The configurations of all four daemons are managed in a single
configuration file called *afp.conf*.

# Options

**-d**

> Do not disassociate daemon from terminal.

**-F** *configfile*

> Specifies the configuration file to use.

**-v** | **-V**

> Print version information and exit.

# Signals

SIGTERM

> Stop the Netatalk AFP and CNID daemons.

SIGHUP

> Sending a *SIGHUP* will cause the Netatalk AFP and CNID daemons to reload
their configurations from *afp.conf*.

# Files

*afp.conf*

> configuration file used by **netatalk**(8), **afpd**(8) and **cnid_metad**(8)

# See Also

afpd(8), cnid_metad(8), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
