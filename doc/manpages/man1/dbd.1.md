# Name

dbd — CNID database maintenance

# Synopsis

`dbd [-cfFstuv] [volumepath]`

`dbd [-V]`

# Description

**dbd** scans all file and directories of AFP volumes, updating the CNID
database of the volume. It must be run with appropriate permissions i.e.
as root..

# Options

**-c**

> convert from "**appledouble = v2**" to "**appledouble = ea**"

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

# CNID background

The CNID backends maintains name to ID mappings. If you change a
filename outside **afpd**(8) (shell, samba), the CNID database will not
reflect that change. Netatalk tries to recover from such inconsistencies
as gracefully as possible.

# See also

cnid_metad(8), cnid_dbd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
