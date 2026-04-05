# Setting up Netatalk

## Setting up the AFP file server

AFP (the Apple Filing Protocol) is a protocol originally created for Apple Macintosh file services.
The final revision of the protocol, AFP 3.4, was introduced with OS X Lion (10.7).

Netatalk's **afpd** daemon provides AFP file services to clients,
including Macs, Apple IIs, and other AFP clients.
Configuration is managed through the *afp.conf* file,
which uses an ini-style syntax.

Netatalk provides remote backup functionality for macOS Time Machine over AFP.
To make a volume a Time Machine target,
set the volume option **time machine = yes**.

Spotlight compatible indexed search on AFP volumes is also supported.
Spotlight is the search technology built into macOS.
Indexing is disabled by default, but you can enable it globally or on a per-volume basis.
To enable Spotlight indexing globally or for a specific volume,
set the option **spotlight = yes** where appropriate.

Starting with Netatalk 2.1, UNIX symlinks are supported on the server.
The semantics are the same as for NFS:
symlinks are not resolved on the server side but are instead passed to the client for resolution,
resulting in links that point somewhere inside the client's filesystem view.

### afp.conf

*afp.conf* is the configuration file used by **afpd** to determine
the behavior and configuration of the AFP file server and the AFP volumes that it provides.

*afp.conf* is divided into several sections:

[Global]

> The global section defines general server options.

[Homes]

> The homes section defines user home volumes.

Any section not named **Global** or **Homes** is interpreted as an AFP volume.

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

### Stale fork cleanup on file deletion

When a client deletes a file via AFP (FPDelete), the server checks for
any open forks associated with the file (forks are internal tracking
objects created when the client opens a file for reading and/or writing).
Netatalk automatically cleans up any forks that are stale (client did not
send a close for each open) — meaning they are not dirty (no
pending metadata flush) and hold no locks. This allows file deletion to
succeed even when a client has neglected to close before deleting the file.

By default, forks holding locks of any type (read or write) are considered
active and will prevent the delete from proceeding (the client receives
a busy error). However, if **close stale rlocks** is set to **yes** in
the Global section of *afp.conf*, forks that only hold read locks (and
no write locks) will also be force-closed.

Forks holding write locks always block deletion.

Example:

    [Global]
    close stale rlocks = yes
