# Name

afpd — Apple Filing Protocol daemon

# Synopsis

`afpd [-d] [-F configfile]`

`afpd [ -v | -V | -h ]`

# Description

`afpd` provides an Apple Filing Protocol (AFP) interface to the Unix
file system. It is normally started at boot time by `netatalk`(8).

`afp.conf` is the configuration file used by `afpd` to determine the
behavior and configuration of a file server.

# Options

-d

> Do not disassociate daemon from terminal.

-v

> Print version information and exit.

-V

> Print verbose information and exit.

-h

> Print help and exit.

-F <configfile\>

> Specifies the configuration file to use.

# Signals

To shut down a user's `afpd` process it is recommended that
`SIGKILL (-9)` *NOT* be used, except as a last resort, as this may leave
the CNID database in an inconsistent state. The safe way to terminate an
`afpd` is to send it a `SIGTERM (-15)` signal and wait for it to die on
its own.

SIGTERM and SIGUSR1 signals that are sent to the main `afpd` process are
propagated to the children, so all will be affected.

SIGTERM

> Clean exit. Propagates from master to childs.

SIGQUIT

> Send this to the master `afpd`, it will exit leaving all children
running! Can be used to implement AFP service without downtime.

SIGHUP

> Sending a `SIGHUP` to afpd will cause it to reload its configuration
files.

SIGINT

> Sending a `SIGINT` to a child `afpd` enables *max_debug* logging for
this process. The log is sent to the file `/tmp/afpd.PID.XXXXXX`.
Sending another `SIGINT` will revert to the original log settings.

SIGUSR1

> The `afpd` process will send the message "The server is going down for
maintenance." to the client and shut itself down in 5 minutes. New
connections are not allowed. If this is sent to a child afpd, the other
children are not affected. However, the main process will still exit,
disabling all new connections.

SIGUSR2

> The `afpd` process will look in the message directory configured at
build time for a file named message.pid. For each one found, a the
contents will be sent as a message to the associated AFP client. The
file is removed after the message is sent. This should only be sent to a
child `afpd`.

# Files

`afp.conf`

> configuration file used by afpd

`afp_signature.conf`

> list of server signature

`afp_voluuid.conf`

> list of UUID for Time Machine volume

`extmap.conf`

> file name extension mapping

`message.pid`

> contains messages to be sent to users.

# See Also

`netatalk(8)`, `hosts_access(5)`, `afp.conf(5)`,
`afp_signature.conf(5)`, `afp_voluuid.conf(5)`, `extmap.conf(5)`,
`dbd(1)`.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
