# Name

netatalk — Netatalk AFP server service controller daemon

# Synopsis

**netatalk** [-F *configfile*]

**netatalk** [-v | -V]

# Description

**netatalk** is the service controller daemon responsible for starting and
restarting the AFP daemon **afpd** and the CNID daemon **cnid_metad**. It is
normally started at boot time by an init system.

# Options

**-d**

> Do not disassociate daemon from terminal.

**-F** *configfile*

> Specifies the configuration file to use.

**-v** | **-V**

> Print version information and exit.

# Signals

SIGTERM

> Stop Netatalk service, AFP and CNID daemons

SIGHUP

> Sending a *SIGHUP* will cause the AFP daemon reload its configuration
file.

# Files

*afp.conf*

> configuration file used by **netatalk**(8), **afpd**(8) and **cnid_metad**(8)

# See Also

afpd(8), cnid_metad(8), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
