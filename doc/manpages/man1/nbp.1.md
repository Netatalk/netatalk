# Name

nbplkup, nbprgstr, nbpunrgstr — tools for accessing the NBP database

# Synopsis

**nbplkup**

**nbprgstr** [-A *address*] [-m *Mac charset*] [-p *port*] *obj:type@zone*

**nbpunrgstr** [-A *address*] [-D *address*] [-m *Mac charset*] [-s] [-f | -l] *obj:type@zone*

# Description

**nbprgstr** registers *nbpname* with **atalkd**(8), at the given
*port*. **nbpunrgstr** informs **atalkd** that *nbpname* is no longer to
be advertised.

**nbplkup** displays up to *maxresponses* (default 1000) entities
registered on the AppleTalk internet. *nbpname* is parsed by
**nbp_name**(3). An \`*=*' for the *object* or *type* matches anything,
and an \`*\**' for *zone* means the local zone. The default values are
taken from the NBPLKUP environment variable, parsed as an *nbpname*.

If -A is specified, this address is used as the local address to be used for the lookup.
If -D is specified, this address is used as the destination address to send the lookup to.
The -f and -l options allow FwdReq and LkUp operations to be used instead of the
default BrRq.

A BrRq asks a router (by default the local atalkd instance) to propagate a lookup request
across the entire AppleTalk internetwork, and is the default option because it is 
generally the most useful.  If in doubt, use a BrRq.

A LkUp asks a node just about itself; it can be used either to query what names are bound 
on a specific node, or by specifying a broadcast destination, simulate the kind of lookup 
that is done on a routerless network.  A FwdReq asks a router to propagate the lookup to 
its directly connected networks that are members of the zone specified, but not to 
propagate it any further across the internetwork; this can be useful on large 
internetworks, or to troubleshoot caches on refractory nodes.

If -s is specified, output is printed in a script-friendly format: for each
response, first the address is printed, followed by a single space, followed 
by the name and type, followed by a linefeed.

If -m is specified, strings will be interpreted in the given Macintosh character set.
If -m is not specified, nbplkup defaults to using MacRoman.

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
    
Find all devices of type *netatalk* in the local zone, providing script-friendly
output.

    example% nbplkup -s :netatalk
    5.42:4 netatalk-build:netatalk
    4.162:4 prometheus:netatalk
    8.31:4 Tiryns:netatalk
    example%

# See also

nbp_name(3), atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
