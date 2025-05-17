# Name

getzones — list AppleTalk zone names

# Synopsis

**getzones** [-g | -m | -l] [-c *Mac charset*] [*address*]

# Description

*Getzones* is used to obtain a list of AppleTalk zone names using the
Zone Information Protocol (ZIP). It sends a GetZoneList request to an
AppleTalk router. By default, it sends the request to the locally
running **atalkd**(8).

# Options

**-g**

> Get the current default zone and the valid network range for the current network.
This is accomplished by sending a ZIP GetNetInfo request.  Note that only seed routers
respond to GetNetInfo, so the usual idiom is to broadcast it.

**-m**

> List the name of the local zone only; this is accomplished by sending a
ZIP GetMyZone request.

**-l**

> List the local zones; this is accomplished by sending a GetLocalZones
request.

**-c**

> Sets the Macintosh character set to use when interpreting zone names.  If not specified,
defaults to MacRoman.

*address*

> Contact the AppleTalk router at *address.* *address* is parsed by
**atalk_aton**(3).

# Examples

Show all zones on the AppleTalk internetwork:

	example$ getzones
	Ethernet
	LocalTalk
	AirTalk
	example$

Get default zone and network configuration for current network from whichever router
is seeding this network;

	example$ getzones -g 0.255
	Network range: 3-10
	Flags (0xa0): requested-zone-invalid only-one-zone
	Requested zone: 
	Zone multicast address: 09:00:07:00:00:a8
	Default zone: Ethernet
	example$

# See Also

atalk_aton(3), atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
