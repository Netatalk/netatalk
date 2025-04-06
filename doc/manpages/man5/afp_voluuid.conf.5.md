# Name

afp_voluuid.conf — Configuration file used by afpd to specify UUID for AFP volumes

# Description

**afp_voluuid.conf** is the configuration file used by **afpd** to store
UUID of all AFP volumes. The configuration lines are composed like:

    "volume-name" "uuid-string"

The first field is the volume name. Volume names must be quoted if they
contain spaces. The second field is the 36 character hexadecimal ASCII
string representation of a UUID.

All leading spaces and tabs are ignored. Blank lines are ignored. The
lines prefixed with \# are ignored. Any illegal lines are ignored.

> **NOTE**

> This UUID is advertised by Zeroconf in order to provide robust
disambiguation of Time Machine volumes.

> It is also used by the MySQL CNID backend.

> This file should not be thoughtlessly edited or copied onto another
server.

# Examples

## Example: afp_voluuid.conf with three volumes

    # This is a comment.
    "Backup for John Smith" 1573974F-0ABD-69CC-C40A-8519B681A0E1
    "bob" 39A487F4-55AA-8240-E584-69AA01800FE9
    mary 6331E2D1-446C-B68C-3066-D685AADBE911

# See also

afpd(8), afp.conf(5), avahi-daemon(8), mDNSResponder(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
