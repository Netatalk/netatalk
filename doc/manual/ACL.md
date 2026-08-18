# ACL Support

Netatalk maps ACLs on a supported server filesystem to AFP ACLs and effective
AFP permissions. It does not provide one ACL implementation for every server
platform or make ACLs override the host filesystem's access checks.

## Support matrix

| Server platform and ACL backend | Support | AFP behavior |
| --- | --- | --- |
| Solaris/illumos with NFSv4/ZFS ACLs | Supported | Maps NFSv4 ACEs to and from the AFP/macOS ACL format. |
| FreeBSD with `libsunacl`/ZFS ACLs | Supported | Uses the NFSv4/ZFS ACL implementation. |
| Linux with POSIX 1e ACLs | Supported | Maps the less expressive POSIX ACL model to AFP; see [POSIX ACLs](#posix-acls). |
| macOS (Darwin) host ACLs | Not supported | Disabled at the build system level. |
| macOS AFP client | Supported | Can use the ACL facilities of a supported server backend. |

Other platforms may compile when a compatible POSIX ACL API is found, but are
not part of the documented support matrix.

## How Netatalk uses ACLs

After authentication, **afpd** calculates the user's effective filesystem ACL
rights and maps them to AFP UARights. Finder uses UARights to decide which
operations to offer. For example, a directory with restrictive mode bits but
an ACL granting the user write access can be shown as writable to that user.
The host filesystem remains the authority for the operation itself.

The [map acls](afp.conf.html#options-for-acl-handling) option controls this
client-facing mapping:

- `rights` (the default) maps effective ACL rights to AFP UARights.

- `mode` also adjusts the UNIX mode reported through AFP so that it reflects
  the mapped ACL rights. It does not change the filesystem object's mode.

- `none` disables effective-rights mapping.

ACL effective-rights mapping does not require LDAP, Active Directory, or any
other directory service. It uses the authenticated user's local UNIX identity
and the server filesystem's ACL.

AFP ACL entries identify users and groups by UUID, whereas the supported server
ACL backends identify named entries by UID or GID. When returning or accepting
ACL entries, Netatalk resolves the UID or GID through the server's normal
user/group lookup and translates the resulting name to or from a UUID. Without
LDAP configured, it can generate a reversible, Netatalk-local UUID from a
local UID or GID. This is sufficient for the server-side mapping, but Netatalk
advertises the volume as not supporting network identities, so macOS clients do
not use the AFP ACL-entry display and editing functions.

To enable those client-facing ACL functions, configure Netatalk's
[LDAP options for ACLs](afp.conf.html#options-for-acl-handling) with a
directory that can map each relevant user and group name to a stable UUID and
back again. LDAP is the mechanism Netatalk implements for this lookup; Active
Directory and Open Directory are examples, not requirements. An LDAP directory
with UUID attributes for users and groups can be used instead. In practice, the
directory should be shared by the server and clients so that the UUIDs refer to
the same identities.

For Solaris/ZFS volumes, configure the ZFS properties appropriate for ACL
inheritance and preservation:

```ini
aclinherit = passthrough
aclmode = passthrough
```

Consult the host's *zfs*(8) documentation for the meaning of these settings.

## macOS ACLs

AFP represents ACLs using the macOS/Darwin model: an ordered list of ACEs that
allow or deny fine-grained rights to UUID-identified users and groups. Directory
rights are distinct; for example, adding a file and adding a subdirectory are
separate permissions.

Netatalk deliberately does not support native macOS (Darwin) ACLs when
**afpd** runs on macOS. The build disables ACL support on that platform.
The POSIX ACL implementation cannot safely evaluate or preserve Darwin's
ordered allow/deny and fine-grained directory semantics.

This is a limitation of the server host, not of AFP clients. A macOS client can
use ACL support from a Linux POSIX-ACL or Solaris/FreeBSD ZFS-ACL server.

## ZFS ACLs

Netatalk uses the NFSv4 ACL APIs on Solaris/illumos and the `libsunacl` API on
FreeBSD. These ACLs are close enough to the AFP/macOS ACL model to preserve
allow/deny ACEs and more of their directory and inheritance semantics than a
POSIX ACL mapping can.

## POSIX ACLs

The Linux backend uses the POSIX 1003.1e ACL API. This is a different and less
expressive model than AFP/macOS or NFSv4 ACLs, so Netatalk necessarily
approximates ACL entries exchanged with AFP clients.

For every object, Netatalk reads the POSIX access ACL. For directories, it
also reads the default ACL when returning an AFP ACL. The POSIX model defines
two ACL types:
Files and directories can have access ACLs,
which are consulted for access checks.
Directories can also have default ACLs,
which are not used in access checks.
When a new object is created inside a directory with a default ACL,
the default ACL is applied to the new object as its access ACL.
Subdirectories inherit default ACLs from their parent.
There are no further mechanisms of inheritance control.

These differences determine the limits of Netatalk's AFP mapping:

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

### Mapping POSIX ACLs to AFP ACLs

When a client reads an object's ACL,
**afpd** maps its POSIX ACL into AFP's macOS-style ACL format. Writing an AFP
ACL maps it back to POSIX ACL entries. This is an approximation, not an exact
round trip, because the server ACL model cannot represent all AFP ACL features.

- **afpd** silently discards requested deny entries,
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

The POSIX ACL mask is not exposed as a separate client permission layer.
Instead, **afpd** calculates the effective user and group rights it reports
through AFP. When AFP changes an object's UNIX permissions, **afpd** updates
ACL_USER_OBJ, ACL_GROUP_OBJ, and ACL_OTHER. If an ACL_MASK entry is present,
it recalculates the mask so that the new group rights take effect while named
ACL_USER and ACL_GROUP entries remain intact.
