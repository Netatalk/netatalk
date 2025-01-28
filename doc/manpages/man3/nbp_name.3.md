# Name

nbp_name — NBP name parsing

# Synopsis

    int nbp_name(name,
        obj,
        type,
        zone);
    char *name;
    char **obj;
    char **type;
    char **zone;

# Description

**nbp_name()** parses user supplied names into their component object,
type, and zone. *obj*, *type*, and *zone* should be passed by reference,
and should point to the caller's default values. **nbp_name()** will
change the pointers to the parsed-out values. *name* is of the form
*object:type@zone*, where each of *object*, *:type*, and *@zone*
replace *obj*, *type*, and *zone*, respectively. *type* must be
proceeded by \`*:*', and *zone* must be preceded by \`*@*'.

# Examples

The argument of **afpd**(8)'s **-n** option is parsed with **nbp_name()**. The
default value of *obj* is the first component of the machine's hostname
(as returned by **gethostbyname**(3)). The default value of *type* is
"AFPServer", and of *zone* is "\*", the default zone. To cause
*afpd* to register itself in some zone other than the default, one would
invoke it as below where *obj* and *type* would retain their default values.

    afpd -n @some-other-zone

# Bugs

*obj*, *type*, and *zone* return pointers into static area which may be
over-written on each call.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
