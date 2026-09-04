# Name

netatalk — Netatalk AFP server service controller daemon

# Synopsis

**netatalk** [-d] [-F *configfile*]

**netatalk** --unprivileged --pidfile *path* [-d] [-F *configfile*]

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

**-u** | **--unprivileged**

> Start a restricted single-user AFP server without root privileges. The
> daemon only accepts the UNIX identity that started it, uses that identity's
> filesystem permissions, and requires SQLite CNID plus SRP authentication.
> See the Configuration manual for all requirements and limitations.

**-P** *path* | **--pidfile** *path*

> Set the controller PID file used by unprivileged mode. This option is
> required with **--unprivileged**. The path must be absolute and located in
> private, user-owned state; it is not accepted for the normal system service.

**-v** | **-V**

> Print version information and exit.

# Signals

SIGTERM

> Stop the Netatalk AFP and CNID daemons.

SIGHUP

> Sending a *SIGHUP* will cause the Netatalk AFP and CNID daemons to reload
their configurations from *afp.conf*. Configuration reloads are disabled in
unprivileged mode; restart **netatalk** after changing *afp.conf*.

# Files

*afp.conf*

> configuration file used by **netatalk**(8), **afpd**(8) and **cnid_metad**(8)

# See Also

afpd(8), cnid_metad(8), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
