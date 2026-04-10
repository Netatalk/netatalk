# Name

afppasswd — AFP password maintenance utility

# Synopsis

**afppasswd** [-cfnr] [-a *username*] [-p *path*] [-u *minimum uid*] [-w *password string*]

**afppasswd** [-r]

# Description

**afppasswd** creates and maintains the credential file used by the SRP
("Secure Remote Password") UAM and, with **-r**, the legacy *afppasswd*
file used by the "Randnum exchange" and "2-Way Randnum exchange" UAMs.

By default **afppasswd** operates in **SRP mode** and stores per-user
salts and verifiers in the SRP verifier file (default *afppasswd.srp*
under the netatalk configuration directory, or whatever is configured
via the "**srp passwd file**" option in **afp.conf**(5)). Pass **-r**
to operate on the legacy Randnum file (default *afppasswd* under the
netatalk configuration directory, or as configured via the
"**passwd file**" option) instead.

There are two invocation styles:

- *As root*, **afppasswd** manages credentials for any system user.
  The user is named with **-a** *username* (which both adds new entries
  and updates existing ones), or the entire file is initialised at once
  with **-c**.
- *As a regular user*, **afppasswd** takes no positional arguments and
  changes the calling user's own AFP password: for SRP by default and for
  the legacy RandNum UAM with **-r**.

The named user must already exist as a local system user.

> ***NOTE:*** The legacy Randnum and 2-Way Randnum UAMs only provide
weak password protection and are discouraged. They should only be enabled
to support very old AFP clients that cannot use SRP, DHX, or DHX2.
If a file named *afppasswd.key* exists at the same path as the *afppasswd* file,
Randnum uses its hex-encoded 8-byte DES key to encrypt the stored password;
otherwise the password is stored as raw hex.

# Examples

Administrator initializing the SRP verifier file and adding a new user:

    example% sudo afppasswd -c
    example% sudo afppasswd -a newuser
    Enter NEW AFP password: (hidden)
    Enter NEW AFP password again: (hidden)
    afppasswd: updated SRP verifier.

Administrator updating an existing user's SRP password:

    example% sudo afppasswd -a someuser

Local user changing their own SRP password:

    example% afppasswd

Administrator managing the legacy Randnum file at a non-default path:

    example% sudo afppasswd -r -c -p /usr/local/etc/afppasswd
    example% sudo afppasswd -r -a olduser -p /usr/local/etc/afppasswd

# Options

**-a** *username*

> Add or update the named user. Required when an administrator operates
on a specific user. Non-root invocations always operate on the calling
user and do not accept this option.

**-c**

> Create and initialise the password/verifier file. Existing entries are
populated as placeholders for every local system user with a uid at or
above the **-u** threshold; passwords still need to be set individually
with **-a**.

**-f**

> Force the action. With **-c**, allows overwriting an existing
password/verifier file.

**-r**

> Operate on the legacy Randnum *afppasswd* file instead of the SRP
verifier file. Affects which file is read or created and which default
path is used when **-p** is not given.

**-p** *path*

> Override the default path of the password/verifier file. The default is
the SRP verifier file in SRP mode and the Randnum *afppasswd* file with
**-r**; both defaults can also be configured globally in **afp.conf**(5)
via "**srp passwd file**" and "**passwd file**" respectively.

**-n**

> If cracklib support is built into *netatalk*, this option disables
password strength validation for this invocation.

**-u** *minimum uid*

> The minimum *user id* (uid) that **afppasswd** considers when **-c**
walks the local user database to populate the file. Defaults to 100.

**-w** *password string*

> Use *string* as the password instead of typing it interactively. Use
this option only when absolutely necessary, since the password may
remain in the terminal history in plain text.

# Files

*afppasswd.srp*

> Default SRP verifier file, located under the netatalk configuration
directory. One line per user, formatted as
*username:hex_salt:hex_verifier*. Override with **-p** or with the
"**srp passwd file**" option in **afp.conf**(5).

*afppasswd*

> Default legacy Randnum file, located under the netatalk configuration
directory. Used only with **-r**. Override with **-p** or with the
"**passwd file**" option in **afp.conf**(5).

# See Also

afpd(8), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
