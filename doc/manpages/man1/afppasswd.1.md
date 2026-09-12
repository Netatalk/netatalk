# Name

afppasswd — AFP password maintenance utility

# Synopsis

**afppasswd** [-cfmnr] [-a *username*] [-F *afp.conf*] [-u *minimum uid*] [-w *password string*]

# Description

**afppasswd** creates and maintains the credential directory used by the SRP
("Secure Remote Password") UAM and, with **-r**, the legacy *afppasswd*
file used by the "Randnum exchange" and "2-Way Randnum exchange" UAMs.

By default **afppasswd** operates in **SRP mode** and stores per-user
salts and verifiers in separate, user-owned files under the SRP verifier
directory (default *afppasswd.srp* under the netatalk configuration
directory, or whatever is configured via the "**srp verifier path**"
option in the standard **afp.conf**(5)). Pass **-r** as root
to operate on the legacy Randnum file (default *afppasswd* under the
netatalk configuration directory, or whatever is configured by the
"**passwd file**" option) instead.

There are two invocation styles:

- *As root*, **afppasswd** manages credentials for any system user.
  The user is named with **-a** *username* (which both adds new entries
  and updates existing ones), or the credential store is initialized at once
  with **-c**.
- *As a regular user*, **afppasswd** takes no positional arguments and
  changes the calling user's own SRP password in the verifier directory from
  the administrator-controlled standard *afp.conf*. Randnum mode and **-F**
  are not available to regular users.

The named user must already exist as a local system user.

When upgrading from the flat-file layout, stop **afpd** and run
**afppasswd -m** as root. The migration validates every record and local user,
builds and synchronizes a temporary verifier directory, retains the flat file
as *afppasswd.srp.legacy* (or a numbered sibling if that name exists), and only
then installs the completed directory. Salts and verifiers are copied without
cryptographic conversion, so users keep their existing SRP passwords. Use
**-F** with **-m** when migrating a store configured outside the standard
*afp.conf*.

Migration rejects malformed records, duplicate users, numeric-uid collisions,
unknown local users, unsafe source metadata, an existing verifier directory,
a partial migration sibling, and a source that changes during the operation.
Before publication, failures remove the temporary directory and leave the
source untouched. Publication failures trigger an automatic rollback; if
rollback itself cannot complete, **afppasswd** reports the exact backup name
containing the original. Do not start **afpd** until that condition has been
resolved.

For manual recovery, keep **afpd** stopped, move any incomplete verifier
directory at the configured path aside, and move the reported *.legacy*
backup back to the configured path. After verifying that the restored object
is a root-owned flat file inaccessible to group and other, retry
**afppasswd -m**. If the automatic migration cannot be used, the
password-reset fallback is to move the flat file aside, run **afppasswd -c**,
and reset each affected password with **afppasswd -a** *username*.

The SRP verifier directory must be root-owned, searchable by users, and not
writable by group or other. Its files are named by numeric uid, are owned by
the corresponding user, have mode 0600, and contain one
*username:hex_salt:hex_verifier* record. This lets users update only their own
verifier without elevated privileges. Existing Randnum credential and key
files must be single-link regular files owned by root and inaccessible to
group or other. Symbolic links are not followed.

**afppasswd** reads credential paths from the standard *afp.conf* installed
under the netatalk configuration directory, or from the root-only **-F**
override. The selected configuration file must be a single-link regular file
owned by root and not writable by group or other; if the standard file is
absent, compiled-in credential paths are used. An explicitly named **-F** file
must exist. Regular users cannot select a
configuration file, so they remain bound to the administrator-selected
standard configuration. If **afpd** is started with **-F** and a nonstandard
configuration file, either mirror its "**srp verifier path**" in the standard
*afp.conf* for local-user password changes or have an administrator run
**afppasswd -F** *afp.conf*. The deprecated "**srp passwd file**" setting is
also recognized when no "**srp verifier path**" is set.

> ***NOTE:*** The legacy Randnum and 2-Way Randnum UAMs only provide
weak password protection and are discouraged. They should only be enabled
to support very old AFP clients that cannot use SRP, DHX, or DHX2.
Randnum requires a file named *afppasswd.key* at the same path as the
*afppasswd* file. The key file contains a hex-encoded 8-byte DES key that
Randnum uses to encrypt the stored password. **afppasswd -r -c** creates
this key file if it is missing and validates an existing one. Randnum
password updates refuse to proceed unless the key file is present and valid.
Randnum passwords can be reset only by root with **afppasswd -r**. The Randnum
UAM no longer accepts password changes over AFP; it remains available for
authentication.
The Randnum UAM logs a warning at startup when the key file is missing or
invalid, but authentication fails until it is fixed.

The key file must contain exactly 16 hexadecimal characters, such as
`0123456789ABCDEF`, with an optional trailing newline. Generate a fresh random
key for each server instead of reusing this example value.

# Examples

Administrator initializing the SRP verifier directory and adding a new user:

    example% sudo afppasswd -c
    example% sudo afppasswd -a newuser
    Enter NEW AFP password: (hidden)
    Enter NEW AFP password again: (hidden)
    afppasswd: updated SRP verifier.

Administrator updating an existing user's SRP password:

    example% sudo afppasswd -a someuser

Administrator migrating the default legacy flat SRP file after stopping
**afpd**:

    example% sudo afppasswd -m
    afppasswd: migrated 2 SRP verifiers; original retained as /etc/netatalk/afppasswd.srp.legacy

Local user changing their own SRP password:

    example% afppasswd

Administrator managing credentials selected by a nonstandard configuration:

    example% sudo afppasswd -r -c -F /usr/local/etc/netatalk/afp.conf
    example% sudo afppasswd -r -a olduser -F /usr/local/etc/netatalk/afp.conf
    Enter NEW AFP password: (hidden)
    Enter NEW AFP password again: (hidden)
    afppasswd: updated Randnum password.

# Options

**-a** *username*

> Add or update the named user. Required when an administrator operates
on a specific user. Non-root invocations always operate on the calling
user and do not accept this option.

**-c**

> Create and initialize the password file or verifier directory. In SRP mode,
a mode-0600 verifier file owned by each local system user with a uid at or
above the **-u** threshold is populated with a disabled placeholder; passwords
still need to be set individually with **-a**. With **-r**, also create or
validate the companion
*afppasswd.key* file for the Randnum UAM.

**-f**

> Force the action. With **-c**, allows overwriting an existing
password/verifier file.

**-m**

> As root, migrate the legacy flat SRP verifier file at the selected path to
the per-uid verifier directory layout without resetting passwords. No option
other than **-F** may be combined with **-m**. **afpd** must remain stopped for
the entire operation.

**-r**

> As root, operate on the legacy Randnum *afppasswd* file instead of the SRP
verifier directory. Affects which object is read or created and which default
configuration option is used. Regular users may not use this option.

**-F** *afp.conf*

> As root, read credential settings from the specified **afp.conf** instead of
the installed standard file. The selected file must be a single-link regular
file owned by root and not writable by group or other. In SRP mode,
**afppasswd** uses "**srp verifier path**" (or the deprecated "**srp passwd
file**" alias); with **-r**, it uses "**passwd file**". Regular users may not
use this option.

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

> Default SRP verifier directory, located under the netatalk configuration
directory. Each numeric-uid file contains one
*username:hex_salt:hex_verifier* record. Override with the "**srp verifier
path**" option in **afp.conf**(5), selected with **-F** by root if needed.

*afppasswd.srp.legacy*, *afppasswd.srp.legacy.N*

> Collision-safe backup names used by **-m** for the original flat verifier
file.

*afppasswd*

> Default legacy Randnum file, located under the netatalk configuration
directory. Used only with **-r**. Override with the "**passwd file**" option
in **afp.conf**(5), selected with **-F** by root if needed.

*afppasswd.key*

> Companion key file for the legacy Randnum file. It must contain exactly
16 hexadecimal characters with an optional trailing newline.

# See Also

afpd(8), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
