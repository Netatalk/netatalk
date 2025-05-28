# Name

afp_signature.conf — Configuration file used by afpd to specify server signature

# Description

**afp_signature.conf** is the configuration file used by **afpd** to store
a server signature automagically. The configuration lines are composed
like:

    "server-name" hexa-string

The first field is the server name. Server names must be quoted if they
contain spaces. The second field is the hexadecimal string of 32
characters for a 16-byte server signature.

All leading spaces and tabs are ignored. Blank lines are ignored. The
lines prefixed with \# are ignored. Any illegal lines are ignored.

> ***NOTE:*** The server signature is a unique 16-byte identifier used to prevent
users from logging on to the same server twice.
>
> Netatalk 2.0 and earlier generated a server signature by using
*gethostid()*. This led to a problem where multiple servers could end up with
the same signature because the hostid is not unique enough.
>
> Current netatalk generates the signature from random numbers and saves
it into afp_signature.conf upon initial startup. For subsequent starts, it is
read from this file.
>
> This file should not be thoughtlessly edited or copied onto another
server.
>
> If you want to set the signature intentionally, use the option
"signature =" in afp.conf. In this case, afp_signature.conf is not used.

# Examples

## Example: afp_signature.conf

    # This is a comment.
    "My Server" 74A0BB94EC8C13988B2E75042347E528

# See also

afpd(8), afp.conf(5), asip-status(1)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
