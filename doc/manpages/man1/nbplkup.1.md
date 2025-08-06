# Name

nbplkup — look up registered AppleTalk entities on the network

# Synopsis

**nbplkup** [-A *address*] [-D *address*] [-m *Mac charset*] [-r *responses*] [-s] [-f | -l] [*nbpname*]

# Description

**nbplkup** displays up to *responses* (default 1000) entities registered on the AppleTalk network.

By default, it looks up all entities in the local zone.
If *nbpname* is specified, the lookup can be filtered to only return entities matching that name.

This uses the Name-Binding Protocol (NBP) to interact with the AppleTalk network.
*nbpname* is parsed by **nbp_name**(3).

# Options

**-A** *address*
> Use *address* as the local address to be used for the lookup.

**-D** *address*
> Use *address* as the destination address to send the lookup to.

**-m** *Mac charset*
> Interpret strings in the given Macintosh character set.
> If not specified, nbplkup defaults to using MacRoman.

**-s**
> Print output in a script-friendly format:
> for each response, first the address is printed, followed by a single space,
> followed by the name and type, followed by a linefeed.

**-f**
> Use a *FwdReq* operation for the lookup request instead of the default *BrRq*.
>
> Cannot be used with -l.

**-l**
> Use a *LkUp* operation for the lookup request instead of the default *BrRq*.
>
> Cannot be used with -f.

**nbpname**
> The NBP name to use.
>
> The *nbpname* is a string of the form *obj:type@zone*, where *obj* is the object name,
> *type* is the type of the object, and *zone* is the zone name.
> The *obj* and *type* can be arbitrary strings, but the *zone* must be a valid zone name.
> An \`*=*' for the *object* or *type* matches anything, and an \`*\**' for *zone* means the local zone.

## Notes

A *BrRq* asks a router (by default the local atalkd instance) to propagate a lookup request
across the entire AppleTalk internetwork.  This is the default option because it is
generally the most useful.  If in doubt, use a *BrRq*.

A *LkUp* asks a node just about itself.  It can be used either to query what names are bound
on a specific node, or by specifying a broadcast destination, simulate the kind of lookup
that is done on a routerless network.

A *FwdReq* asks a router to propagate the lookup to its directly connected networks
that are members of the zone specified, but not to propagate it any further across the internetwork.
This can be useful on large internetworks, or to troubleshoot caches on refractory nodes.

# Environment Variables

NBPLKUP

> default nbpname for nbplkup in format *obj:type@zone*

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

nbp_name(3), nbprgstr(1), nbpunrgstr(1), atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
