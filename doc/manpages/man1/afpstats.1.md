# Name

afpstats — List Netatalk AFP server statistics

# Synopsis

**afpstats** [-h *hostname*] [-s *socket*]

**afpstats** [-v | \-\-version]

# Description

**afpstats** lists information about users connected to a Netatalk AFP server running on the local machine,
their mounted volumes, time of login, and other details.
Sessions connected over both TCP/IP (DSI) and AppleTalk (ASP) are reported.

To enable AFP statistics in Netatalk at runtime, "**afpstats = yes**" must be set in *afp.conf*.
To allow non-root users to query the socket, set "**afpstats group = *group***" and add those users
to that group.

# Options

**-h**, **\-\-hostname** *hostname*

> Filter the output by the given hostname.

**-s**, **\-\-socket** *path*

> Connect to the afpstats Unix socket at *path*.
> Defaults to */var/lib/netatalk/afpstats.sock* (path varies with the configured *localstatedir*).

**-v**, **\-\-version**

> Show version and exit.

# Note

The socket is created by **afpd** on startup with mode 0660. By default,
only root and users permitted by the socket's group ownership can query the
session list. Use the **afpstats group** option in *afp.conf* to set the
group owner explicitly, for example to a dedicated *netatalk* group.
The socket path also depends on search permissions for the parent directory.

# Examples

List all connected users to the AFP server running on *fileserver*:

    $ afpstats --hostname fileserver
    Connected user  PID     Login time       State     Protocol  Hostname    Mounted volumes
    alice           452547  Apr 28 21:58:50  active    TCP/IP    fileserver  Data Vault, alice's Home
    bob             451969  Apr 28 21:21:32  sleeping  TCP/IP    fileserver  My AFP Volume

# See Also

afpd(8), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
