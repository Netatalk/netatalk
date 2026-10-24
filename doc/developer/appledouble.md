# Mac Metadata and Fork Storage

AFP exposes a data fork, a resource fork, Finder information, dates, AFP
attributes, and generic extended attributes. Netatalk maps these concepts
onto host files, xattrs, and sidecar files. Storage policy and platform
capabilities determine the mapping; the AFP interface alone does not.

## Key modules

| Location | Responsibility |
| --- | --- |
| `include/atalk/adouble.h`, `libatalk/adouble/` | Parsed metadata, fork descriptors, storage, and locking |
| `include/atalk/vfs.h`, `libatalk/vfs/` | Built-in volume operations and generic EAs |
| `etc/afpd/fork.c`, `etc/afpd/ofork.c`, `etc/afpd/fork.h` | AFP fork commands and shared-object ownership |
| `etc/afpd/ad_cache.c` | Read-only metadata reconstruction and resource caching |
| `etc/afpd/extattrs.c`, `etc/afpd/file.c` | AFP extended attributes and file parameters |

## Storage boundaries

| Information | Representation |
| --- | --- |
| Data fork | Ordinary host filesystem file |
| Legacy AppleDouble v2 metadata/resource fork | `.AppleDouble/<name>` storage |
| EA-mode metadata | Extended attributes using the selected conventions |
| EA-mode resource fork | Platform-dependent resource-fork access or `._<name>` storage |
| Generic AFP EAs | Selected VFS EA operations |
| Persistent CNID records | Separate database, with validated hints in metadata |

On macOS, EA mode can use native resource-fork access. Other platforms use
the available sidecar/storage path. A macOS-created `._` file and a legacy
Netatalk `.AppleDouble` file are not interchangeable merely because both
use AppleDouble concepts. Conversion code checks the layout it receives.

initvol_vfs() selects a table of built-in operations according to volume
policy. It is static dispatch, not a dynamic VFS plugin ABI. Generic EA
storage is also distinct from the metadata fields interpreted by
`struct adouble`. Selecting Samba-compatible conventions does not itself
establish multi-protocol cache/locking policy or migrate existing metadata.

## Object and descriptor ownership

ad_init() initializes an AppleDouble object from the volume's settings.
ad_open() opens selected components; ad_close() releases those opens.
Data, metadata, and resource descriptors have their own open-reference
counts. The shared object's `ad_refcount` is a different lifetime mechanism:
ad_ref()/ad_unref() must not substitute for closing descriptors.

The session's open-fork table owns shared-object references separately from
these descriptor opens. Its allocation and close/deallocation contracts
are described in [AFP Daemon](afpd.md).

The selected fork need not have all metadata components open. Check the
specific descriptor/open state before using it. Open descriptors can also
outlive a pathname or directory entry, so operations on an open fork must
distinguish the held object from a later path lookup.

### Read-only access

`ADFLAGS_RDONLY` expresses read intent while allowing internal write access.
With `ADFLAGS_SETSHRMD`, including opens that specify `ADFLAGS_CHECK_OF`,
data and resource descriptors request `O_RDWR` to support write locks.
Version 2 metadata also requests `O_RDWR`: its descriptor is shared with
the resource fork, which may later be opened for writing. An existing empty
v2 metadata file can receive a fresh header during the open, even without
`ADFLAGS_CREATE`. Callers must not use this flag alone to promise that
metadata will remain unchanged.

For isolated metadata inspection, such as `dbd -s`, use
`ADFLAGS_HF | ADFLAGS_RDONLY_STRICT`, adding `ADFLAGS_DIR` for a directory.
Strict mode implies `ADFLAGS_RDONLY`, bypasses descriptor promotion, and
rejects an empty v2 header with `EIO` before it can be repaired. It accepts
only these flags and requires a private, freshly initialized `struct adouble`.
Conflicting flags or an object previously used by ad_open() cause `EINVAL`.
Close a successful open with `ad_close(&ad, ADFLAGS_HF)` and reinitialize
the object before another open. A retry after a failed strict open also
requires reinitialization.

Normal AFP fork operations need shared descriptors and lock handling.
Replacing their opens with strict mode requires a separate ownership review:
closing an additional descriptor for the same file can release existing
process-owned POSIX locks. See ad_open() for the API contract.

## Parsed entries and cached metadata

ad_entry() returns a pointer only when the requested entry's parsed bounds
are valid. Code that reads or writes a particular byte count must also
respect that count, using ad_entry_fits() where appropriate. An entry ID
or a non-NULL object is not proof that a complete expected payload exists.

Metadata can be absent while a resource fork exists. Negative cache states
preserve this distinction; treating every metadata ENOENT as an empty fork
can lose content. ad_metadata_cached() provides read-only reconstructed
metadata and closes storage it opened internally. A caller that intends
to write must acquire the appropriate writable opens separately.
See [Caching and Consistency](caching.md).

ad_flush() persists modified metadata. Check its result and close results
according to the operation's error contract. A multi-step metadata/fork
copy is not automatically transactional. The [libatalk example](libatalk.md)
shows explicit ownership and destination-identity handling.

## Locking and I/O

AFP deny modes, explicit byte-range locks, and temporary POSIX data-I/O
locks have different roles. `strict locking` controls the temporary locks;
it does not remove explicit AFP locking commands or deny-mode checks.
Lock placement and descriptor selection depend on storage and platform,
so changes must preserve both data-fork and resource-fork behavior.

ad_read()/ad_write() access the selected fork. Eligible transfers may use
sendfile or recvfile support. On write failure, socket consumption and
file persistence can differ; the caller must still drain the transport
payload correctly. See [AFP Request Processing](request-flow.md).

Metadata contains CNID hints, while authoritative identity belongs to the
[CNID backend](cnid.md). Copying a file's metadata to another inode must
not silently duplicate its persistent identity. Reset or validate hints
as required by the operation.
