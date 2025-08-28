# Name

afpstats — List netatalk AFP server statistics

# Synopsis

**afpstats**

# Description

**afpstats** list information about users connected to the netatalk AFP server,
their mounted volumes, and time of login.

# Note

To make use of this feature, **afpd** must be built with *AFP statistics* support,
which can be checked with "**afpd -V**".
Furthermore, the host must be running D-Bus configured with a policy that defines the *org.netatalk.AFPStats* service,
typically enabled through a configuration file called *netatalk-dbus.conf* which is distributed with netatalk.

Finally, to enable AFP statistics in netatalk at runtime, "**afpstats = yes**" must be set in *afp.conf*.

# Examples

    $ afpstats
    Connected user   PID      Login time        State          Mounted volumes
    alice            452547   Apr 28 21:58:50   active         Test Volume, alice's Home
    charlie          451969   Apr 28 21:21:32   active         My AFP Volume

# See Also

afpd(8), afp.conf(5), dbus-daemon(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
