# Name

getzones — list AppleTalk zone names

# Synopsis

**getzones** [-g | -m | -l | -q network | -z *zone*] [-c *Mac charset*] [*address*]

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

**-q**

> List the zones available to a given network range.  This is accomplished by sending
a ZIP Query.  Note that to query the zones available to an extended network, generally
the first network number in the range must be used.

**-z**

> Verifies whether *zone* is a valid zone for this network by sending a GetNetInfo.
If *zone* is valid, exit with 0; if *zone* is not valid, exit with 2.  Also prints the
network configuration including, if the zone is invalid, the default zone to use instead
of the one requested.

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
	
Check whether OtherNet is a valid zone for the current network:

	example$ getzones -z Othernet 0.255 >/dev/null || echo "bad zone"
	bad zone
	example$

# See Also

atalk_aton(3), atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
