# Name

nbplkup, nbprgstr, nbpunrgstr — tools for accessing the NBP database

# Synopsis

**nbplkup**

**nbprgstr** [-A *address*] [-m *Mac charset*] [-p *port*] *obj:type@zone*

**nbpunrgstr** [-A *address*] [-m *Mac charset*] *obj:type@zone*

# Description

**nbprgstr** registers *nbpname* with **atalkd**(8), at the given
*port*. **nbpunrgstr** informs **atalkd** that *nbpname* is no longer to
be advertised.

**nbplkup** displays up to *maxresponses* (default 1000) entities
registered on the AppleTalk internet. *nbpname* is parsed by
**nbp_name**(3). An \`*=*' for the *object* or *type* matches anything,
and an \`*\**' for *zone* means the local zone. The default values are
taken from the NBPLKUP environment variable, parsed as an *nbpname*.

# Environment Variables

NBPLKUP

> default nbpname for nbplkup

ATALK_MAC_CHARSET

> the codepage used by the clients on the Appletalk network

ATALK_UNIX_CHARSET

> the codepage used to display extended characters on this shell.

# Examples

Find all devices of type *LaserWriter* in the local zone.

    example% nbplkup :LaserWriter
                   Petoskey:LaserWriter        7942.129:218
                 Gloucester:LaserWriter        8200.188:186
                     Rahway:LaserWriter        7942.2:138
                 517 Center:LaserWriter        7942.2:132
                      ionia:LaserWriter        7942.2:136
         Evil DEC from Hell:LaserWriter        7942.2:130
                  Hamtramck:LaserWriter        7942.2:134
             Iron Mountain :LaserWriter        7942.128:250
    example%

# See also

nbp_name(3), atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
