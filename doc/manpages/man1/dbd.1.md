# Name

dbd — CNID database maintenance utility

# Synopsis

**dbd** [-cfistv] [-F *configfile*] [-u *username*] *volumepath*

**dbd** [-V]

# Description

**dbd** scans all files and directories of an AFP volume and updates the CNID
database when discrepancies are found between the database and the filesystem.
It must be run as root. The argument must resolve to the configured volume
root; a subdirectory is refused. Use **-F** when the volume is defined in a
non-default configuration.

Run maintenance with affected AFP clients disconnected and other writers
stopped. Back up volume metadata and catalog state before resetting or
converting them.

When run with the **-f** parameter, it will completely wipe the existing
volume table in the database and create a new one from scratch.
This can be used for example to convert a volume from one CNID scheme to another.
It can change IDs and invalidate Mac aliases or saved references. It is not
a routine first diagnostic step for contention or a missing volume.
A volume whose **cnid scheme** names a CNID backend this installation does not
have is refused with a message naming the scheme; correct *afp.conf* first.

# Options

**-c**

> convert from *AppleDouble v2* to *Extended Attributes* filesystem metadata

**-f**

> Delete and recreate the volume's CNID records. Existing IDs may change;
combine with **-i** when fresh IDs rather than stored AppleDouble hints are
required.

**-F** *configfile*

> location of the *afp.conf* config file

**-i**

> invalidate CNID hints stored in AppleDouble files,
so existing database entries are used instead of stale AD hints;
combine with **-f** to assign fresh CNIDs to all files and directories

**-s**

> scan volume: treat the volume as read only and don't perform any
filesystem modifications. Database opening and CNID reconciliation can
still create or update database state, including records. This is not a
zero-write database dry run. It does not cancel a separately requested
**-f** reset.

**-t**

> show statistics while running

**-u** *username*

> username for use with AFP volumes using user variable $u. With it,
**dbd** skips the **private** volumes of other accounts; without it, it
reaches every private volume. Either way it opens a private volume's state
as the volume's owner

**-v**

> verbose

**-V**

> display version info

# CNID usage

The CNID backends maintain filename-to-ID mappings. If you change a
filename outside **afpd**(8) (shell, samba), the CNID database will not
reflect that change. Netatalk tries to recover from such inconsistencies
as gracefully as possible.

For when you want to operate on a Netatalk volume outside of an AFP client
without sacrificing the integrity of the CNID database,
you can use the **nad**(1) tool which updates the CNID records while
modifying files.

# See also

nad(1), afpd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
