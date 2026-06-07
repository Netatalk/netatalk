# Setting up Netatalk

## Setting up the AFP file server

Netatalk's **afpd** daemon provides AFP file services to clients.
You usually launch the AFP file service daemon through the **netatalk** controller daemon.
The controller daemon manages the lifecycle of the AFP file service daemon,
including Zeroconf service registration, and housekeeping for certain CNID backends
and Spotlight indexing.

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
