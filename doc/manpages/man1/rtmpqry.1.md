# Name

rtmpqry — query AppleTalk routing information

# Synopsis

**rtmpqry** [-a | -r] [ -A local_address ] [remote_address]

# Description

**rtmpqry** is used to query and display the routing information that an
AppleTalk router is advertising using the RTMP protocol.  By default,
it queries the local atalkd instance.

# Options

**-a**

> Query all routes, do not suppress those that would be dropped by split-
horizon processing.  Generally (and if this option is omitted), routers
do not advertise routes where, if you sent it a packet, it would send that
packet straight back out the same interface to another router, as this would
be pointless.  However, for visibility into the structure of the internetwork,
it is sometimes useful to see all the routes.

**-r**

> Request just the RTMP network metadata, not any routes.

**-A**

> Bind to the local AppleTalk address given.

# Examples

Get network range and seed router for the current network:

    example$ rtmpqry -r 0.255
    Router: 9.239
    Network(s): 3 - 10
    example$

Get all routes from the router at 12.1:

    example$ rtmpqry -a 12.1
    3 - 10 distance 0 via 12.1
    11 distance 0 via 12.1
    12 distance 0 via 12.1

    example$

# See Also

atalk_aton(3), atalkd(8), getzones(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
