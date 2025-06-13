# Name

dbd — CNID database maintenance utility

# Synopsis

**dbd** [-cfFstuv] [*volumepath*]

**dbd** [-V]

# Description

**dbd** scans all file and directories of AFP volumes, updating the CNID
database of the volume. It must be run with appropriate permissions i.e.
as root.

When run with the **-f** parameter, it will completely wipe the existing
volume table in the database and create a new one from scratch.
This can be used to for example convert from one CNID scheme to another.

# Options

**-c**

> convert from *AppleDouble v2* to *Extended Attributes* filesystem metadata

**-f**

> delete and recreate CNID database

**-F**

> location of the *afp.conf* config file

**-s**

> scan volume: treat the volume as read only and don't perform any
filesystem modifications

**-t**

> show statistics while running

**-u**

> username for use with AFP volumes using user variable $u

**-v**

> verbose

**-V**

> display version info

# CNID usage

The CNID backends maintains filename to ID mappings. If you change a
filename outside **afpd**(8) (shell, samba), the CNID database will not
reflect that change. Netatalk tries to recover from such inconsistencies
as gracefully as possible.

For when you want to operate on a Netatalk volume outside of an AFP client
without sacrificing the integrity of the CNID database,
you can use the **ad**(1) tool which updates the CNID records while
modifying files.

# See also

ad(1), cnid_metad(8), cnid_dbd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
