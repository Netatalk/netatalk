# Interoperability with Other Network File Systems

Netatalk and Samba can share the same volume and store Extended Attribute metadata
in a mutually compatible format,
enabling access via AFP or SMB.

> ***WARNING:*** The **ea = samba** and **ea = sys** (Netatalk default)
> metadata formats are **not** compatible,
> and there is currently no automated way to convert between them.
> If you have an existing Netatalk volume using the default settings and want to add Samba sharing,
> the recommended migration path is to copy files from one volume to another using an AFP client.

When sharing a volume with other processes (Samba, NFS, local applications),
keep *dircache validation freq* at **1** (the default)
so Netatalk detects external changes on every access.
If Netatalk is the **only** process on the volume,
you can set *dircache validation freq* = 100 for maximum performance.

## Netatalk configuration

Use *ea = samba* to store Extended Attributes in the Samba-compatible format.

    [Global]
        vol preset = my default values
        dircache validation freq = 1
        ea = samba

    [Homes]
        basedir regex = /home

    [Test Volume]
        path = /export/test1

    [My Time Machine Volume]
        path = /export/timemachine
        time machine = yes

## Samba configuration

The Samba **catia**, **fruit**, and **streams_xattr** VFS modules
provide compatibility for Apple SMB clients and interoperability with Netatalk.
Use **hide files** (not **veto files**) to hide Netatalk's invisible files from Windows clients.

    [global]
        ea support = yes
        vfs objects = catia fruit streams_xattr

        fruit:encoding = native
        streams_xattr:prefix = user.
        streams_xattr:store_stream_type = no

        hide files = /.DS_Store/Network Trash Folder/TheFindByContentFolder/TheVolumeSettingsFolder/Temporary Items/.TemporaryItems/.VolumeIcon.icns/Icon?/.FBCIndex/.FBCLockFolder/

        read only = no

    [homes]

    [Test Volume]
        path = /export/test1

    [My Time Machine Volume]
        path = /export/timemachine
