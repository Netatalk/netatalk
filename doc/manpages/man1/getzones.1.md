# Name

getzones — list AppleTalk zone names

# Synopsis

**getzones** [-m | -l] [-c *Mac charset*] [*address*]

# Description

*Getzones* is used to obtain a list of AppleTalk zone names using the
Zone Information Protocol (ZIP). It sends a GetZoneList request to an
AppleTalk router. By default, it sends the request to the locally
running **atalkd**(8).

# Options

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

# See Also

atalk_aton(3), atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
