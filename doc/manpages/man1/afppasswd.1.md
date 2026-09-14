# Name

afppasswd — AFP password maintenance utility

# Synopsis

**afppasswd** [-cdfmnr] [-a *username* | -d *username*] [-p *directory*] [-u *minimum uid*] [-w *password*]

**afppasswd** [-n]

# Description

**afppasswd** creates and maintains the credential directory used by the SRP
("Secure Remote Password") UAM and, with **-r**, the legacy *afppasswd*
file used by the "Randnum exchange" and "2-Way Randnum exchange" UAMs.

By default **afppasswd** operates in **SRP mode** and stores per-user
salts and verifiers in separate per-user files under the SRP verifier
directory (default *afppasswd.srp* under the netatalk configuration
directory). Pass **-r** as root to operate on the legacy Randnum file
(default *afppasswd* under the netatalk configuration directory) instead.

There are two invocation styles:

- *As root*, **afppasswd** manages credentials for any system user.
  The user is named with **-a** *username* (which both adds new entries
  and updates existing ones) or **-d** *username* (which disables SRP for that
  user), or the credential store is initialized at once with **-c**.
- *As a regular user*, **afppasswd** takes no positional arguments and
  changes the calling user's own SRP password. The caller may use **-p** to
  select the verifier directory. Randnum mode remains root-only.

The named user must already exist as a local system user.

When upgrading from the flat-file layout, stop **afpd** and run
**afppasswd -m** as root. The migration validates every record and local user,
builds and synchronizes a temporary verifier directory, retains the flat file
as *afppasswd.srp.legacy* (or a numbered sibling if that name exists), and only
then installs the completed directory. Salts and verifiers are copied without
cryptographic conversion, so users keep their existing SRP passwords. Use
**-p** with **-m** when migrating a store at a non-default path.

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
writable by group or other. Its files are named by numeric uid, have mode
0600, and contain one *username:hex_salt:hex_verifier* record. Disabled
placeholders created by **-c** or preserved by **-m** are root-owned. Root's
**-a** operation transfers ownership to the corresponding user only after
writing and synchronizing a real verifier. User ownership enables SRP login
and lets enrolled users update their own verifier without elevated privileges.
If a restore changes an enrolled verifier's metadata, stop **afpd**, restore
the file's ownership to its local user and its mode to 0600, then restart
**afpd**. For example, to repair alice's verifier in */path/to/verifiers*, run
**chown** *alice* */path/to/verifiers/$(id -u alice)* and **chmod 0600** on
that file as root. Do not change a root-owned disabled placeholder this way;
use **afppasswd -a** *username* to set a password and re-enable it. A verifier
with more than one hard link must be replaced or have its extra links removed.

Once enrolled, users can also replace their verifier directly. The old-password
proof and optional CrackLib password-quality checks in **afppasswd** are
utility checks, not server-enforced password policy. Root can use **-d** to
disable an SRP account: it writes a root-owned disabled placeholder, creating
one when the uid file is absent. Use **-a** to set a new password and re-enable
the account. Disabling does not terminate existing sessions or disable other
UAMs.

Existing Randnum credential and key files must be single-link regular files
owned by root and inaccessible to group or other.
Symbolic links are not followed.

The compiled-in defaults cover packaged installations. For a non-default
installation, pass **-p** the same directory configured by "**srp verifier
path**" in **afp.conf**(5), or, with **-r**, the same file configured by
"**passwd file**". **afppasswd** does not read *afp.conf* to resolve this
path. The kernel enforces access to an explicitly selected path; a user who
selects another user's credential store simply receives an access error.

> ***NOTE:*** The legacy Randnum and 2-Way Randnum UAMs only provide
weak password protection and are discouraged. They should only be enabled
to support very old AFP clients that cannot use SRP, DHX, or DHX2.
Randnum requires a file named *afppasswd.key* at the same path as the
*afppasswd* file. The key file contains a hex-encoded 8-byte DES key that
Randnum uses to encrypt the stored password. **afppasswd -r -c** creates
this key file if it is missing and validates an existing one. Randnum
password updates refuse to proceed unless the key file is present and valid.
Randnum passwords can be reset only by root with **afppasswd -r**.
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

Administrator managing credentials at a non-default path:

    example% sudo afppasswd -r -c -p /usr/local/etc/afppasswd
    example% sudo afppasswd -r -a olduser -p /usr/local/etc/afppasswd
    Enter NEW AFP password: (hidden)
    Enter NEW AFP password again: (hidden)
    afppasswd: updated Randnum password.

# Options

**-a** *username*

> Add or update the named user. Required when an administrator operates
on a specific user. Non-root invocations always operate on the calling
user and do not accept this option. In SRP mode, a successful password set
also enables the user's verifier by transferring file ownership to that user.
Existing SRP verifiers may have any permission mode; permissions are reset to
0600 before processing the verifier. Files must still be owned by the named
user or root and be regular files with exactly one hard link.

**-d** *username*

> As root, disable the named user's SRP credential. The uid verifier is
replaced with a root-owned, mode-0600 disabled placeholder; a missing verifier
is created with that state. This is idempotent and is useful when cleaning up a
reused uid. Use **-a** to set a new password and re-enable the user. This
option accepts only **-p** and cannot be used with **-r**.
Existing verifiers may have any permission mode; permissions are reset to 0600.
The same ownership and file-type requirements as **-a** apply.

**-c**

> Create and initialize the password file or verifier directory. In SRP mode,
a root-owned mode-0600 verifier file for each local system user with a uid at
or above the **-u** threshold is populated with a disabled placeholder.
Passwords must be set individually by root with **-a** before users can log
in over SRP or change their own SRP password. SRP initialization refuses an
existing credential path; migrate a legacy flat file with **-m** or move it
aside before intentionally starting with an empty store. With **-r**, also create or validate
the companion *afppasswd.key* file for the Randnum UAM.

**-f**

> With **-r -c**, replace an existing Randnum credential file and, if needed,
an invalid Randnum key file. This legacy option is not available in SRP mode.

**-m**

> As root, migrate the legacy flat SRP verifier file at the selected path to
the per-uid verifier directory layout without resetting passwords. No option
other than **-p** may be combined with **-m**. **afpd** must remain stopped for
the entire operation.

**-r**

> As root, operate on the legacy Randnum *afppasswd* file instead of the SRP
verifier directory. Affects which object is read or created and which
compiled-in default is used. Regular users may not use this option.

**-p** *path*

> Override the compiled-in credential path. In SRP mode, *path* is a verifier
directory. With **-r**, *path* is a Randnum password file.

**-n**

> If cracklib support is built into *netatalk*, this option disables
password strength validation for this invocation.

**-u** *minimum uid*

> The minimum *user id* (uid) that **afppasswd** considers when **-c**
walks the local user database to populate the file. Defaults to 100.

**-w** *password*

> Use *string* as the password instead of typing it interactively. Use
this option only when absolutely necessary, since the password may
remain in the terminal history in plain text.

# Files

*afppasswd.srp*

> Default SRP verifier directory, located under the netatalk configuration
directory. Each numeric-uid file contains one
*username:hex_salt:hex_verifier* record. Override with **-p**.

*afppasswd.srp.legacy*, *afppasswd.srp.legacy.N*

> Collision-safe backup names used by **-m** for the original flat verifier
file.

*afppasswd*

> Default legacy Randnum file, located under the netatalk configuration
directory. Used only with **-r**. Override with **-p**.

*afppasswd.key*

> Companion key file for the legacy Randnum file. It must contain exactly
16 hexadecimal characters with an optional trailing newline.

# See Also

afpd(8), afp.conf(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
