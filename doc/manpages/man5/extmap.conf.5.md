# Name

extmap.conf — Configuration file used by afpd to specify file name extension mappings

# Description

**extmap.conf** is the configuration file used by **afpd** to specify file
name extension mappings.

The configuration lines are composed like:

    .extension [ type [ creator ] ]

Any line beginning with a hash (“#”) character is ignored. The
leading-dot lines specify file name extension mappings. The extension
'.' sets the default creator and type for otherwise untyped Unix files.

# Examples

## Example: Extension is jpg, type is "JPEG", creator is "ogle"

    .jpg "JPEG" "ogle"

## Example: Extension is lzh, type is "LHA", creator is not defined

    .lzh "LHA"

# See Also

afp.conf(5), afpd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
