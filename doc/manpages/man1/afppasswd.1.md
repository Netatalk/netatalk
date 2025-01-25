# Name

afppasswd — AFP password maintenance utility

# Synopsis

`afppasswd [-acfn] [-p afppasswd file] [-u minimum uid] [-w password string]`

# Description

`afppasswd` creates and maintains an afppasswd file, which supplies the
user credentials for the "Randnum exchange" and "2-Way Randnum exchange"
User Authentication Modules.

`afppasswd` can either be called by root with parameters to manage all
user credentials, or by local system users with no parameters to change
their own AFP passwords.

> **NOTE**

> With this utility you can only change the passwords used by these two
Random Number UAMs. As they provide only weak password encryption, their
use is discouraged unless one has to support very old AFP clients that
can not deal with the more secure "DHX" or "DHX2" UAMs.

# Examples

Local user changing their own password:

    example% afppasswd
    Enter NEW AFP password: (hidden)
    Enter NEW AFP password again: (hidden)
    afppasswd: updated password.

# Options

`-a`

> Add a new user to the `afppasswd` file.

`-c`

> Create and/or initialize `afppasswd` file or specific user.

`-f`

> Force the current action.

`-p` <path\>

> Path to `afppasswd` file.

`-n`

> If cracklib support is built into *netatalk* this option will cause
cracklib checking to be disabled, if the superuser does not want to have
the password run against the cracklib dictionary.

`-u` <minimum uid\>

> This is the minimum *user id* (uid) that `afppasswd` will use when
creating users.

`-w` <password string\>

> Use string as password, instead of typing it interactively. Please use
this option only if absolutely necessary, since the password may remain
in the terminal history in plain text.

# See Also

`afpd(8)`, `afp.conf(5)`.

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
