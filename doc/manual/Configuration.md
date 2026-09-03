# Setting up Netatalk

## Setting up the AFP file server

Netatalk's **afpd** daemon provides AFP file services to clients.
You usually launch the AFP file service daemon through the **netatalk** controller daemon.
The controller daemon manages the lifecycle of the AFP file service daemon,
including Zeroconf service registration, and housekeeping for certain CNID backends
and Spotlight (Finder search) indexing.

Configuration of both the daemons and the AFP volumes are managed through the *afp.conf* file,
which uses an ini-style syntax.

### afp.conf

*afp.conf* is the configuration file used by **afpd** to determine
the behavior and configuration of the AFP file server and the AFP volumes that it provides.

*afp.conf* is divided into several sections:

[Global]

> The global section defines general server options.

[Homes]

> The homes section defines user home volumes.

Any section not named **Global** or **Homes** is interpreted as an AFP volume.

> **Note:** Nested volumes (where one volume's path is a subdirectory of another's) are not allowed.
When Netatalk detects a nested volume configuration, it will skip the nested volume and log a warning.
This is because nested volumes cause CNID database inconsistencies that lead to crashes or data corruption.
If you need to share both a parent directory and a subdirectory,
use the **path** option in the **Homes** section to limit the home volume to a subdirectory,
or reorganize your shares so that no volume path is contained within another.

To share user homes,
define a **Homes** section and specify the **basedir regex** option.
This can be a simple path to the parent directory of all user homes,
or a regular expression.

Example:

    [Homes]
    basedir regex = /home

Any user logging into the AFP server will now have a user volume
available at the path */home/NAME*.

A more complex setup involves a server with many user homes
split across two different filesystems:

- /RAID1/homes

- /RAID2/morehomes

The following configuration handles this:

    [Homes]
    basedir regex = /RAID./.*homes

If **basedir regex** contains a symlink, set the canonicalized absolute path.
For example, when */home* links to */usr/home*:

    [Homes]
    basedir regex = /usr/home

For a detailed explanation of all available options,
refer to the [afp.conf](afp.conf.5.html) man page.

### Backup Volumes

Netatalk provides remote backup functionality for macOS Time Machine over AFP.
To make a volume a Time Machine target,
set the volume option **time machine = yes**.

When used together with Zeroconf,
the backup volume will be automatically advertised via Bonjour when the server is running,
allowing macOS clients to discover and select it as a Time Machine backup destination.

### Symlinks

By default, symlinks are not followed on the server side but are instead passed to the client for
resolution, resulting in links that point somewhere inside the client's filesystem view.
This is the same behaviour as the historical Mac OS X AFP server.

Server-side symlink following can be enabled per volume with **follow symlinks = yes**.
When enabled, symlink targets created via AFP are validated: absolute paths, paths containing
**..** components, targets that resolve outside the volume root, and targets on a different
filesystem device are rejected.
Note that pre-existing symlinks created outside of AFP are not subject to this validation.

Be aware that enabling server-side symlink following can lead to security issues if not used carefully.

## Interoperability with Other Network File Systems

Netatalk and Samba can share the same volume and store Extended Attribute metadata
in a mutually compatible format,
enabling access via AFP or SMB.

> ***WARNING:*** The **ea = samba** and **ea = sys** (Netatalk default)
> metadata formats are **not** compatible,
> and there is currently no automated way to convert between them.
> If you have an existing Netatalk volume using the default settings and want to add Samba sharing,
> the recommended migration path is to copy files from one volume to another using an AFP client.

### Locking

AFP and SMB clients only respect each other's locks when both servers place
real POSIX byte-range locks on the shared files. Two settings make that happen:

- Netatalk's **strict locking** takes a POSIX
  read lock for every AFP read and a write lock for every AFP write, making
  AFP file access conflict with Samba's locks and vice versa. It is off by
  default; setting **multi protocol = yes** on any volume changes the default
  to on globally (an explicit *strict locking* setting still wins, with a
  logged warning).
- Samba's own **strict locking** option is the same concept on the SMB side.
  Set *strict locking = yes* in smb.conf: the default *Auto* skips the check
  for files under an SMB lease. Samba's *posix locking* must stay at its
  default *yes*, otherwise neither server sees the other's locks at all.

Samba's **fruit:locking = netatalk** module option (off by default in Samba)
additionally maps SMB open and deny modes onto the same on-disk convention
Netatalk uses for AFP deny modes, so conflicting opens are refused across
protocols and a file with open AFP forks is protected from deletion over SMB
when deny modes are in effect.

SMB client-side caching (oplocks and leases) is not aware of AFP activity:
with Samba's defaults, an SMB client may cache stale data and overwrite
AFP-side changes. On a volume shared by both servers, disable it in smb.conf
(*oplocks = no*, *level2 oplocks = no*, *smb2 leases = no*) or, on Linux,
set *kernel oplocks = yes* instead (real kernel leases that Netatalk's file
access breaks; this disables SMB2 leases and durable handles).

> ***WARNING:*** Do not export a Time Machine share over both protocols.
> Samba's *fruit:time machine = yes* preset disables *posix locking* and
> kernel oplocks, removing every cross-protocol safety described here.
> Time Machine shares should be served by one protocol only.

### Settings side by side

Each row pairs the afp.conf and smb.conf settings that serve the same purpose.
An empty cell means that side needs no setting for that row — either the
behavior is built in, or the other server needs several settings to match one
setting on this side. Settings marked *default* are listed so you know not to
change them.

| Netatalk (afp.conf)                  | Samba (smb.conf)                     | What it does |
| ------------------------------------ | ------------------------------------ | ------------ |
| **ea = samba**                       | **vfs objects = catia fruit streams_xattr** | Apple metadata (Finder info, resource forks) stored in a format both servers read and write. |
|                                      | **fruit:metadata = netatalk** *(Samba default)* | Finder metadata in the same extended attribute Netatalk uses. |
|                                      | **fruit:resource = file** *(Samba default)* | Resource forks in the same *._* AppleDouble files Netatalk uses. |
|                                      | **fruit:encoding = native**          | Same on-disk names for characters that are illegal on Windows. |
|                                      | **streams_xattr:prefix = user.**     | An AFP extended attribute and an SMB alternate data stream become the same object. Requires **ea = samba** on the Netatalk side (its trailing-byte format matches). |
|                                      | **streams_xattr:store_stream_type = no** | Second half of the row above — strips the *:$DATA* suffix so the names match. |
| **multi protocol = yes**             | | Declares that Samba modifies the volume too; defaults every Netatalk coherency setting in this table accordingly. |
| **strict locking = yes** *(the default when multi protocol = yes)* | **strict locking = yes** | Every read and write is checked against the other server's byte-range locks. Samba's default *Auto* skips the check for leased files — set *yes* explicitly. |
|                                      | **posix locking = yes** *(Samba default — keep)* | Samba mirrors its locks into the kernel where Netatalk can see them. With this off, no lock crosses between the servers at all. |
| *(built in — always on)*             | **fruit:locking = netatalk**         | AFP and SMB open/deny modes conflict across protocols: a deny-mode open on one side refuses a conflicting open on the other. |
| **dircache validation freq = 1** *(the default when multi protocol = yes)* | | Netatalk notices files that Samba created, renamed, or deleted. |
| *(built in when multi protocol = yes)* | | The volume's resource forks are not cached in afpd memory, so Samba-side changes are always re-read. |
|                                      | **kernel change notify = yes** *(Samba default — keep)* | Samba notices files that Netatalk changed, and notifies SMB clients. |
|                                      | **oplocks = no**                     | SMB clients must not cache file data on a volume Netatalk also serves — a cached write could overwrite an AFP-side change. |
|                                      | **level2 oplocks = no**              | Second half of the row above. |
|                                      | **smb2 leases = no**                 | Third half of the row above. On Linux, **kernel oplocks = yes** may replace all three (real kernel leases that Netatalk's file access breaks). |
| **solaris share reservations = yes** *(default; Solaris/illumos only)* | | F_SHARE reservations — the one deny-mode layer Samba can see on illumos without *fruit:locking*. |
| **umask = 0002**                     | **create mask = 0664**               | New files get the same permissions no matter which protocol created them. |
|                                      | **directory mask = 0775**            | Same, for directories. |
|                                      | **map archive = no**                 | Stops Samba from setting a phantom execute bit on every new file (the DOS archive flag mapped to a mode bit). |

### Caching

When sharing a volume with other processes (Samba, NFS, local applications),
set *multi protocol = yes*, which defaults *dircache validation freq* to **1**
so Netatalk detects external changes on every access.
When Netatalk is the **only** process on the volume (the default posture),
*dircache validation freq* defaults to 100 for maximum performance.

### Netatalk configuration

Set *multi protocol = yes* on volumes Samba also serves. It changes the
defaults of the settings safe concurrent access requires: *strict locking*
defaults to yes, *dircache validation freq* defaults to 1, and the volume is
excluded from the resource-fork data cache.
Explicit settings always win — an explicit value that weakens coherency
is honored and logged with a verbose warning. Pair it with *ea = samba*
to store Extended Attributes in the Samba-compatible format. The example
lists the defaulted settings explicitly anyway, so the intended
configuration is visible at a glance.

    [Global]
        multi protocol = yes
        ea = samba
        umask = 0002
        strict locking = yes            ; the default when multi protocol = yes
        dircache validation freq = 1    ; the default when multi protocol = yes
        solaris share reservations = yes ; already the default (Solaris/illumos only)

    [Homes]
        basedir regex = /home

    [Test Volume]
        path = /export/test1

    [My Time Machine Volume]
        path = /export/timemachine
        time machine = yes
        ; Time Machine over AFP only -- do not also export this path via SMB

### Samba configuration

The Samba **catia**, **fruit**, and **streams_xattr** VFS modules
provide compatibility for Apple SMB clients and interoperability with Netatalk.
Use **hide files** (not **veto files**) to hide Netatalk's invisible files from Windows clients.

    [global]
        ea support = yes
        vfs objects = catia fruit streams_xattr

        fruit:encoding = native
        fruit:locking = netatalk
        streams_xattr:prefix = user.
        streams_xattr:store_stream_type = no

        strict locking = yes
        ; posix locking = yes is the Samba default; do not disable it

        ; no SMB client caching on a volume Netatalk also serves
        oplocks = no
        level2 oplocks = no
        smb2 leases = no

        ; same permissions from both protocols
        create mask = 0664
        directory mask = 0775
        map archive = no

        hide files = /.DS_Store/Network Trash Folder/TheFindByContentFolder/TheVolumeSettingsFolder/Temporary Items/.TemporaryItems/.VolumeIcon.icns/Icon?/.FBCIndex/.FBCLockFolder/

        read only = no

    [homes]

    [Test Volume]
        path = /export/test1
