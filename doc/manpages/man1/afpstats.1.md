# Name

afpstats — List AFP server statistics

# Synopsis

`afpstats`

# Description

`afpstats` list statistics about users connected to the AFP server,
their mounted volumes, etc.

# Note

`afpd` must be built with D-Bus support. Check this with "`afpd -V`".

Additionally, "`afpstats = yes`" must be set in `afp.conf`.

# Examples

    $ afpstats
    Connected user   PID      Login time        State          Mounted volumes
    sandy            452547   Apr 28 21:58:50   active         Test Volume, sandy's Home
    charlie          451969   Apr 28 21:21:32   active         My AFP Volume

# See Also

`afpd(8)`, `afp.conf(5)`, `dbus-daemon(1)`

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
