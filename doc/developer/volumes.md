# Volume Lifecycle and Access Policy

An AFP volume combines a configured share with session-owned runtime state.
Loading its definition, advertising it, authorizing a user, and opening it
are separate operations. A controller's view of configured volumes does not
prove that a particular user can mount them.

## Key modules

| Location | Responsibility |
| --- | --- |
| `libatalk/util/netatalk_conf.c` | Definition loading, authorization filtering, Homes, UUIDs, and substitutions |
| `include/atalk/volume.h` | Configuration and runtime fields of `struct vol` |
| `etc/afpd/volume.c` | Open, parameters, flush, close, and reset handling |
| `libatalk/vfs/vfs.c` | Selection of built-in storage operations |
| `etc/afpd/unix.c`, `etc/afpd/acls.c`, `libatalk/acl/` | Unix/AFP permissions and ACL identity mapping |

```mermaid
flowchart LR
    Section[Configuration section] --> Definition[Loaded volume definition]
    Definition --> Authorized[Session-authorized volume list]
    Authorized --> Open[Open volume and runtime handles]
    Open --> Close[Close and purge runtime state]
```

## Definition and open state

load_afp_conf_vols() constructs definitions through creatvol(). Static
sections name paths and policies; presets supply common settings. Homes
definitions use the authenticated user's resolved home path, a base-directory
regular expression, and an optional subdirectory inside that home. They do
not discover arbitrary directories through filesystem globbing.

Construction can skip duplicate/nested paths, unauthorized definitions, or
volumes whose initialization fails. A missing share therefore needs parser
and access-policy diagnostics before transport tuning. Substitution depends
on the session context; see [Configuration Internals](configuration.md).

afp_openvol() initializes runtime resources such as the CNID handle and
root directory and selects optional search/icon state. Each worker owns its
`struct vol` and handles. Multiple workers can access the same files and
persistent records without sharing these objects in memory.

## Access and filename policy

User/group and host restrictions filter availability. Read-only policy,
per-user write lists, Unix permissions, and ACLs govern operations after
authentication. Share policy does not grant filesystem access that the
effective serving identity lacks. Forced user/group and admin identity
controls have process-level implications; they are not arbitrary local
overrides on an individual request.

Unix mode/ACL translation is handled in the AFP and shared ACL code. Keep
advertised protocol capabilities distinct from the permissions of an actual
object and from whether the volume is open.

Volume and Mac charsets, case policy, and mangling affect client-visible
names. Changes must preserve path and persistent-ID resolution, including
older clients whose names are not UTF-8.

## Storage and persistent state

initvol_vfs() selects built-in VFS operations for the volume's storage
policy. Generic EA operations and AppleDouble metadata/resource-fork storage
are related but different interfaces. Changing `ea` on a populated volume
does not migrate its existing metadata. See
[Mac Metadata and Fork Storage](appledouble.md).

The resolved database path and volume UUID identify persistent CNID state.
A Global database base path and a per-volume directory have different
interpretations. UUID generation uses a registry; an unavailable registry
can prevent a volume's database opening. See [Persistent File Identity](cnid.md).

`multi protocol` adjusts coherency defaults for external writers. Some
effects are process-wide, while resource-fork admission is per volume.
Storage compatibility through `ea = samba` does not itself enable those
defaults or create cross-host locking/hints. See [Caching and Consistency](caching.md).

## Optional volume services

Time Machine affects capability reporting and discovery records. A reported
volume-size limit is not a filesystem quota. Search binds a compiled
backend to the volume; its query lifetime must end before volume state is
released. Virtual icons add a synthetic root entry until a real icon takes
precedence. Details belong in [Indexed Spotlight Search](indexing.md) and
[AFP Icons](icons.md).

## Reload and close

On reload, matching loaded volumes retain their options. Removed volumes
are marked or removed according to open state and `close vol` policy. A
reloaded dictionary does not imply that an existing session uses new volume
settings; fresh construction can require reconnect or restart.

Close releases runtime handles and cache state, including deferred cleanup
and parent-directory descriptors. Review open forks, query ownership,
cached directory pointers, and CNID connections together when changing
teardown order. A handle that outlives its volume can otherwise retain
pointers into freed state.
