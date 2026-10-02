# Name

netatalk — Netatalk AFP server service controller daemon

# Synopsis

**netatalk** [-d] [-F *configfile*]

**netatalk** -u -P *path* [-d] [-F *configfile*]

**netatalk** [-v | -V]

# Description

**netatalk** is the daemon used to control the Netatalk AFP file server.
For most deployments you would use **netatalk** rather than launching
and stopping **afpd** yourself.
The **netatalk** daemon is normally started at boot time by an init system.

The controller daemon will launch the AFP daemon **afpd**.

The configurations of both daemons are managed in a single
configuration file called *afp.conf*.

# Options

**-d** | **--debug**

> Do not disassociate daemon from terminal.

**-F** *configfile* | **--config** *configfile*

> Specifies the configuration file to use.

**-u** | **--single-user**

> Start a restricted single-user AFP server without root privileges. The
> daemon only accepts the UNIX identity that started it and uses that
> identity's filesystem permissions. The server signature, the volume UUIDs
> and each volume's SQLite CNID database are kept under **vol dbpath**, owned
> by that user and readable by nobody else.
> Authentication is SRP against the verifier directory the user created with
> **afppasswd -c -p**. See the Configuration manual for all requirements and
> limitations. Without this option **netatalk** must run as root and exits
> with status 1 otherwise.

**-P** *path* | **--pidfile** *path*

> Set the controller PID file used by single-user mode. This option is
> required with **--single-user**. The path must be absolute and located in
> private, user-owned state; it is not accepted for the normal system service.

**-v** | **-V** | **--version**

> Print version information and exit.

# Signals

SIGTERM

> Stop the Netatalk AFP daemon.

SIGHUP

> Sending a *SIGHUP* will cause the Netatalk AFP daemon to reload
its configuration from *afp.conf*. Configuration reloads are disabled in
single-user mode; restart **netatalk** after changing *afp.conf*.

# Files

*afp.conf*

> configuration file used by **netatalk**(8) and **afpd**(8)

# See Also

afpd(8), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
