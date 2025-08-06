# Name

nbprgstr, nbpunrgstr — manage AppleTalk entry registration on the network

# Synopsis

**nbprgstr** [-A *address*] [-m *Mac charset*] [-p *port*] *nbpname*

**nbpunrgstr** [-A *address*] [-m *Mac charset*] *nbpname*

# Description

**nbprgstr** registers *nbpname* with **atalkd**(8), at the given *port*.

**nbpunrgstr** informs **atalkd** that *nbpname* is no longer to be advertised.

This uses the Name-Binding Protocol (NBP) to interact with the AppleTalk network.
*nbpname* is parsed by **nbp_name**(3).

# Options

**-A** *address*
> Use *address* as the local address to be used for the lookup.

**-m** *Mac charset*
> Interpret strings in the given Macintosh character set.
> If not specified, nbplkup defaults to using MacRoman.

**-p** *port*
> Use *port* as the port to register the entity on.

**nbpname**
> The NBP name to use.
>
> The *nbpname* is a string of the form *obj:type@zone*, where *obj* is the object name,
> *type* is the type of the object, and *zone* is the zone name.
> The *obj* and *type* can be arbitrary strings, but the *zone* must be a valid zone name.
> An \`*=*' for the *object* or *type* matches anything, and an \`*\**' for *zone* means the local zone.

# Examples

Register an entity "myhost:Foobar" on the local zone, with DDP address 65280.68:

    example% nbprgstr -A 65280.68 myhost:Foobar
    example% nbplkup
                            myhost:Foobar                             65280.68:128
    example%

Revoke the registration of the entity "myhost:Foobar":

    example% nbpunrgstr myhost:Foobar
    example% nbplkup
    example%

# See also

nbp_name(3), nbplkup(1), atalkd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
