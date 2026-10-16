# CNID Persistent File Identity

Catalog Node Identifiers (CNIDs) let AFP clients refer to files and
directories independently of their current names. Netatalk reconciles
parent/name and device/inode information with persistent records. Supported
AFP renames and moves should preserve identity; a database reset or arbitrary
external file replacement cannot promise the same IDs.

## Key modules

| Location | Responsibility |
| --- | --- |
| `libatalk/cnid/cnid.c`, `libatalk/cnid/cnid_init.c` | Input validation, backend registry, and dispatch |
| `include/atalk/cnid.h` | Backend operations and public error contract |
| `libatalk/cnid/cnid_sqlite.c`, `libatalk/cnid/cnid_mysql.c` | Embedded SQLite and optional MySQL implementations |
| `etc/afpd/directory.c`, `etc/afpd/volume.c` | AFP identity resolution and volume reset handling |
| `bin/dbd/` | Offline filesystem/catalog reconciliation |

## Backend and handle ownership

SQLite is always built and is the default. MySQL/MariaDB is optional.
cnid_init() registers statically linked implementations, and cnid_open()
returns a backend handle for an open volume. Each worker has its own
connection and backend-private state, even when workers use the same
persistent database.

The command named `dbd` is a maintenance utility. It is distinct from the
historical *Database Daemon* backend that used Berkeley DB.

The dispatch layer separates lookup by parent/name, reconciliation against
stat identity, ID-to-name resolution, allocation, update, and deletion.
Choosing the correct operation matters: an unavailable database does not
authorize creating a replacement identity for an existing file.

CNIDs and parent DIDs at the AFP-facing API use network byte order. SQL IDs
are host-order numeric values. Preserve that boundary through comparisons,
serialization, and backend dispatch. Reserved IDs below `CNID_START` are
not ordinary database allocations.

## Storage, UUIDs, and transactions

SQLite opens a database in the resolved volume database directory. The
database filename derives from the volume name, and the per-volume table
name derives from its UUID. A volume registry stores path, stamp, and
depletion information. The UUID therefore participates in persistent
identity as well as discovery; changing it is not a cosmetic rename.

Rows associate an ID with name, parent DID, device, and inode. Uniqueness
constraints on parent/name and device/inode require mutation code to
reconcile conflicting records. Current SQLite connections use WAL,
`synchronous=NORMAL`, a bounded busy timeout, and per-connection prepared
statements. Readers can overlap, while write contention still needs handling.

Transaction ownership must remain explicit. Error cleanup should roll back
only the transaction it owns and preserve the failure being reported.
Retries must be bounded; contention or an unusable row must not cause an
endless lookup/insert cycle. Backend code validates rows, UUIDs, and metadata
hints before using them.

The optional MySQL backend uses an external server and UUID-based volume
tables. Centralizing CNID records does not provide distributed file locking
or cross-host cache invalidation. User configuration and backend setup are
documented in [CNID Backends](https://netatalk.io/manual/en/CNID.html).

## Metadata hints and error contracts

AppleDouble can cache an ID hint and database stamp. A hint accelerates
reconciliation only when it belongs to the current identity space and
passes validation. User-writable metadata is not authoritative database
state. See [Mac Metadata and Fork Storage](appledouble.md).

`CNID_ERR_*` values occupy an unsigned range stored through `errno`. Use
`CNID_ERRNO()` to compare them, and preserve the following distinctions:

| Result | Caller implication |
| --- | --- |
| `CNID_ERR_NOTFOUND` | The lookup found no matching record |
| `CNID_ERR_BUSY`, `CNID_ERR_CORRUPT`, `CNID_ERR_DB` | The backend could not establish existence; treat as a backend failure |
| `CNID_ERR_RESET` | The identity space was reset; existing client IDs are stale |
| Parameter/path/closed-handle errors | Check arguments, path policy, and lifetime |

`CNID_ERRNO_IS_BACKEND_FAILURE()` groups unusable backend answers. AFP
callers use retryable `AFPERR_MISC` for those answers instead of permanent
`AFPERR_NOOBJ` or `AFPERR_NOID`. This distinction must survive path lookup,
catalog search, and parameter handling.

Name search also has a completeness contract. cnid_find() and
cnid_find_scoped() return a `more_available` indication and require a minimum
result-buffer capacity. A filled buffer is not proof that the search is
complete. See the API declarations rather than copying its signature into
new callers.

## Reset and maintenance boundaries

A table reset invalidates every old ID in that table. cnid_volume_reset()
in `etc/afpd/volume.c` emits a UUID-tagged reset hint and ends the originating
session. Sibling sessions with matching open volumes are also asked to end.
The client must obtain new IDs; clearing a worker cache alone cannot repair
client-held references. See [Caching and Consistency](caching.md).

The offline `dbd` utility scans filesystem entries and reconciles catalog
records and AppleDouble hints. Resetting records can invalidate aliases and
stored references. Its read-only-filesystem mode does not promise that
opening a backend has no database side effects. Operational commands and
preconditions belong in [dbd(1)](https://netatalk.io/manual/en/dbd.1.html).
