# ACL Support

ACL support for AFP is implemented for ZFS ACLs on Solaris and derived platforms,
and for POSIX 1e ACLs on Linux.

## Configuration

For basic operation, no additional configuration is needed.
Netatalk reads ACLs on the fly and calculates effective permissions,
which are then sent to the AFP client via the so-called UARights permission bits.
On a Mac, the Finder uses these bits to adjust permissions in its windows.
For example, a folder whose UNIX mode is read-only
but whose ACL grants the user write access will display the effective read-write permission.
Without this permission mapping,
the Finder would display a read-only icon and the user would not be able to write to the folder.

By default,
the authenticated user's effective permissions are only mapped to the UARights permission structure,
not the UNIX mode.
You can adjust this behavior with the configuration option [map acls](afp.conf.html#options-for-acl-handling).

However, neither in Finder "Get Info" windows nor in the Terminal will you be able to see the ACLs,
because of how macOS ACLs are designed.
To display ACLs on the client,
you must set up both client and server as part of an authentication domain
(directory service, e.g. LDAP, OpenDirectory).
The reason is that macOS ACLs are bound to UUIDs, not just UIDs or GIDs.
Therefore **afpd** must be able to map every filesystem UID and GID to a UUID
so that it can return server-side ACLs —
which are bound to UNIX UID and GID —
mapped to macOS UUIDs.

Netatalk can query a directory server using LDAP.
Either the directory server already provides a UUID attribute for users and groups
(Active Directory, Open Directory),
or you can repurpose an unused attribute (or add a new one) in your directory server
(e.g. OpenLDAP).

In detail:

1. For Solaris/ZFS: ZFS Volumes

    You should configure the following ZFS ACL properties for any volume you want to use
    with Netatalk:

        aclinherit = passthrough
        aclmode = passthrough

    For an explanation of what these properties do and how to apply them,
    check your host's ZFS documentation (e.g. *man zfs*).

2. Authentication Domain

    Your server and clients must be part of a security association
    where identity data comes from a common source.
    ACLs in Darwin are based on UUIDs, as is the ACL specification in AFP 3.2.
    Your source of identity data must provide an attribute
    for every user and group containing a UUID stored as an ASCII string.
    In other words:

    - You need an Open Directory server or an LDAP server where you
      store UUIDs in an attribute.

    - Your clients must be configured to use this server.

    - Your server should be configured to use this server via nsswitch
      and PAM.

    - Configure Netatalk via the [LDAP options for ACLs](afp.conf.html#options-for-acl-handling)
      in *afp.conf* so that Netatalk can retrieve UUIDs
      for users and groups via LDAP search queries.

## macOS ACLs

With Access Control Lists (ACLs),
macOS offers a powerful extension of the traditional UNIX permissions model.
An ACL is an ordered list of Access Control Entries (ACEs)
that explicitly grant or deny a set of permissions to a given user or group.

Unlike UNIX permissions, which are bound to user or group IDs,
ACLs are tied to UUIDs.
Accessing an object's ACL therefore requires
that server and client use a common directory service
to translate between UUIDs and user/group IDs.

ACLs and UNIX permissions interact straightforwardly.
Because ACLs are optional,
UNIX permissions act as a default mechanism for access control.
Changing an object's UNIX permissions leaves its ACL intact,
and modifying an ACL never changes the object's UNIX permissions.
During access checks, macOS first examines an object's ACL,
evaluating ACEs in order until all requested rights have been granted,
a requested right has been explicitly denied by an ACE,
or the end of the list has been reached.
If there is no ACL or the permissions granted by the ACL are insufficient,
macOS next evaluates the object's UNIX permissions.
ACLs therefore always take precedence over UNIX permissions.

## ZFS ACLs

ZFS ACLs closely match macOS ACLs.
Both offer mostly identical fine-grained permissions and inheritance settings.

## POSIX ACLs

### Overview

Compared to macOS or NFSv4 ACLs,
POSIX ACLs represent a different,
less versatile approach to overcoming the limitations of traditional UNIX permissions.
Implementations are based on the withdrawn POSIX 1003.1e standard.

The standard defines two types of ACLs.
Files and directories can have access ACLs,
which are consulted for access checks.
Directories can also have default ACLs,
which are not used in access checks.
When a new object is created inside a directory with a default ACL,
the default ACL is applied to the new object as its access ACL.
Subdirectories inherit default ACLs from their parent.
There are no further mechanisms of inheritance control.

Architectural differences between POSIX ACLs and macOS ACLs include:

- No fine-grained permissions model.
Like UNIX permissions,
POSIX ACLs only differentiate between read, write, and execute permissions.

- Entries within an ACL are unordered.

- POSIX ACLs can only grant rights. There is no way to explicitly deny rights.

- UNIX permissions are integrated into an ACL as special entries.

POSIX 1003.1e defines six different types of ACL entries.
The first three types integrate standard UNIX permissions.
They form a minimal ACL;
their presence is mandatory and only one entry of each type is allowed per ACL.

- ACL_USER_OBJ: the owner's access rights.

- ACL_GROUP_OBJ: the owning group's access rights.

- ACL_OTHER: everybody's access rights.

The remaining entry types extend the traditional permissions model:

- ACL_USER: grants access rights to a specific user.

- ACL_GROUP: grants access rights to a specific group.

- ACL_MASK: limits the maximum access rights that can be granted
by entries of type ACL_GROUP_OBJ, ACL_USER, and ACL_GROUP.
As the name suggests, this entry acts as a mask.
Only one ACL_MASK entry is allowed per ACL.
If an ACL contains ACL_USER or ACL_GROUP entries,
an ACL_MASK entry must be present;
otherwise it is optional.

In order to maintain compatibility with applications not aware of ACLs,
POSIX 1003.1e changes the semantics of system calls and utilities
that retrieve or manipulate an object's UNIX permissions.
If an object only has a minimal ACL,
the group permission bits of the UNIX permissions correspond
to the value of the ACL_GROUP_OBJ entry.

However, if the ACL also contains an ACL_MASK entry, the behavior differs.
The group permission bits of the UNIX permissions correspond
to the value of the ACL_MASK entry —
i.e. calling *chmod g-w* will not only revoke write access for the group,
but for all entities granted write access by ACL_USER or ACL_GROUP entries.

### Mapping POSIX ACLs to macOS ACLs

When a client reads an object's ACL,
**afpd** maps its POSIX ACL onto an equivalent macOS ACL.
Writing an object's ACL requires **afpd** to
map a macOS ACL onto a POSIX ACL.
Due to architectural limitations of POSIX ACLs,
an exact mapping is usually impossible,
so the result is an approximation of the original ACL's semantics.

- **afpd** silently discards entries that deny permissions,
because they cannot be represented in the POSIX architecture.

- Because entries within POSIX ACLs are unordered,
it is impossible to preserve order.

- Inheritance control is subject to severe limitations:

  - Entries with the only_inherit flag set become part of
    the directory's default ACL only.

  - Entries with at least one of the flags file_inherit,
    directory_inherit,
    or limit_inherit set become part of the directory's access and default ACL,
    but the restrictions they impose on inheritance are ignored.

- The lack of a fine-grained permission model on the POSIX side
normally results in an increase of granted permissions.

Because macOS clients are unaware of the POSIX 1003.1e-specific relationship
between UNIX permissions and ACL_MASK,
**afpd** does not expose this feature to the client.
This avoids compatibility issues and handles UNIX permissions and ACLs
the same way as Apple's reference implementation of AFP.
When an object's UNIX permissions are requested,
**afpd** calculates proper group rights and returns the result together with
the owner's and everybody's access rights to the caller via the "permissions"
and "ua_permissions" members of the FPUnixPrivs structure
(see Apple Filing Protocol Reference, page 181).
When changing an object's permissions,
**afpd** always updates ACL_USER_OBJ, ACL_GROUP_OBJ, and ACL_OTHER.
If an ACL_MASK entry is present,
**afpd** recalculates its value so that the new group rights become effective
while existing ACL_USER and ACL_GROUP entries remain intact.
