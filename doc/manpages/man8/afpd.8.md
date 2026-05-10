# Name

afpd — Apple Filing Protocol daemon

# Synopsis

**afpd** [-d] [-F *configfile*]

**afpd** [-v | -V | -h]

# Description

**afpd** provides an Apple Filing Protocol (AFP) interface to the Unix
file system. It is normally started at boot time by **netatalk**(8).

*afp.conf* is the configuration file used by **afpd** to determine the
behavior and configuration of a file server.

# Options

**-d**

> Do not disassociate daemon from terminal.

**-v**

> Print version information and exit.

**-V**

> Print verbose information and exit.

**-h**

> Print help and exit.

**-F** *configfile*

> Specifies the configuration file to use.

# Signals

To shut down a user's **afpd** process it is recommended that
*SIGKILL (-9)* *NOT* be used, except as a last resort, as this may leave
the CNID database in an inconsistent state. The safe way to terminate an
**afpd** is to send it a *SIGTERM (-15)* signal and wait for it to die on
its own.

SIGTERM and SIGUSR1 signals that are sent to the parent **afpd** process
are propagated to the children, so all processes will be affected.

SIGTERM

> Clean exit. Propagates from parent to children.

SIGQUIT

> Sending this to the parent **afpd** will cause it to exit,
but leaving all children running!
This can be used to implement an AFP service without downtime.

SIGHUP

> Sending a *SIGHUP* to afpd will cause it to reload its configuration
files.

SIGINT

> Sending a *SIGINT* to a child **afpd** enables *max_debug* logging and dumps
caches to the system tmp directory:
>
> - Debug log: *afpd.PID.XXXXXX*
> - Directory cache: *dircache.PID*
>
> **Directory cache dump** (see **dircache mode** in **afp.conf(5)**):
>
> - **LRU mode**: Single queue of cached entries
> - **ARC mode**: T1/T2 (cached), B1/B2 (ghost) lists, parameter p, statistics
>
> Sending another *SIGINT* reverts to original log settings.

SIGUSR1

> The **afpd** process will send the message "The server is going down for
maintenance." to the client and shut itself down in 5 minutes. New
connections are not allowed. If this is sent to a child afpd, the other
children are not affected. However, the main process will still exit,
disabling all new connections.

SIGUSR2

> Send a server message to the AFP client of a child **afpd** process.
When the signal is received, **afpd** looks in the message directory
(default: *LOCALSTATEDIR/netatalk/msg/*; the compiled-in path is
printed by **afpd -V**) for a message file in the following order:
>
> 1. *message.PID* — a file whose name ends with the child process ID
> 2. *message* — a fallback used when no PID-specific file is found
>
> The contents of the file are read and forwarded to the connected AFP
client as a server message.  The file is deleted after it is read.
This signal must only be sent to a **child** afpd process.  The parent
afpd does not install a SIGUSR2 handler, so sending it there will
terminate the parent process and prevent new connections.
>
> **Example usage:**
>
>     # Find the child afpd PID for the connected user, then:
>     echo "Scheduled maintenance tonight at 22:00" \
>         > LOCALSTATEDIR/netatalk/msg/message.PID
>     kill -USR2 PID

# Files

*afp.conf*

> configuration file used by afpd

*extmap.conf*

> file name extension mapping

*afp_signature.conf*

> list of server signatures

*afp_voluuid.conf*

> list of UUIDs for Time Machine volumes

*LOCALSTATEDIR/netatalk/msg/message.PID*, *LOCALSTATEDIR/netatalk/msg/message*

> Server message files read on SIGUSR2.  The PID-specific form
(*message.PID*) targets a single connected client; the plain *message*
form is a fallback when no PID-specific file exists.  Both are deleted
after being read.  The compiled-in directory path is printed by
**afpd -V**.

# See Also

netatalk(8), hosts_access(5), afp.conf(5),
afp_signature.conf(5), afp_voluuid.conf(5), extmap.conf(5),
dbd(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
