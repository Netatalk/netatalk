# Name

afp_signature.conf — Configuration file used by afpd to specify server signature

# Description

**afp_signature.conf** is the configuration file used by **afpd** to specify
server signature automagically. The configuration lines are composed
like:

<"server-name"\> <hexa-string\>

The first field is server name. Server names must be quoted if they
contain spaces. The second field is the hexadecimal string of 32
characters for 16-bytes server signature.

The leading spaces and tabs are ignored. Blank lines are ignored. The
lines prefixed with \# are ignored. The illegal lines are ignored.

> **NOTE**

> Server Signature is unique 16-bytes identifier used to prevent logging
on to the same server twice.

> Netatalk 2.0 and earlier generated server signature by using
gethostid(). There was a problem that another servers have the same
signature because the hostid is not unique enough.

> Current netatalk generates the signature from random numbers and saves
it into afp_signature.conf. When starting next time, it is read from
this file.

> This file should not be thoughtlessly edited and be copied onto another
server. If it wants to set the signature intentionally, use the option
"signature =" in afp.conf. In this case, afp_signature.conf is not used.

# Examples

## Example: afp_signature.conf

    # This is a comment.
    "My Server" 74A0BB94EC8C13988B2E75042347E528

# See also

afpd(8), afp.conf(5), asip-status(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
