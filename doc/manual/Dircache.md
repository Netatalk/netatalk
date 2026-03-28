# Directory Cache Tuning

The dircache (directory cache) in Netatalk is a sophisticated multi-layer cache for files and directories,
including filesystem stat information, AppleDouble metadata, and resource forks.

Once populated during an initial directory enumeration,
subsequent operations return cached data directly from memory.
This enables fast operations even with very large working sets.
When any AFP user renames or deletes files and directories,
dircache entries are updated or removed for all users.
While the dircache is always kept in sync with AFP operations,
it does not detect changes made outside of Netatalk
(e.g. local filesystem changes, or modifications by other file sharing services such as Samba).

By default (with *dircache validation freq = 1*),
Netatalk performs a *stat()* operation on every access to validate
that the cache entry still matches the storage layer.
While the cache still avoids most I/O operations,
this ctime check adds some overhead.
When Netatalk is the only process accessing a storage volume,
you can set a high *dircache validation freq* value (e.g. 100) to skip most of these extra *stat()* calls,
reducing load on the storage and page-cache layers.

If a dircache entry is found to be stale when accessed,
Netatalk gracefully detects the issue,
rebuilds the entry with the latest state from storage,
and increments the *invalid_on_use* counter.
You can use the *invalid_on_use* counter to tune *dircache validation freq*
in environments where external changes are infrequent but maximum performance is still desired.

dircache size = *number* **(G)**

Maximum number of entries in the cache.
The given value is rounded up to the nearest power of 2.
Each entry takes about 192 bytes (on 64-bit systems),
and each **afpd** child process for every connected user has its own cache.
Consider the number of users when configuring *dircache size*.

Default: 65536, Maximum: 1048576.

dircache validation freq = *number* **(G)**

Directory cache validation frequency for external change detection.
Controls how often cached entries are validated against the filesystem
to detect changes made outside of Netatalk
(e.g. direct filesystem modifications by other processes).
A value of 1 means validate on every access
(the default, for backward compatibility);
higher values validate less frequently.
For example, 5 means validate cache entries every 5th access.
Higher values improve performance and reduce storage I/O and page-cache stress,
but may delay detection of external changes.

Default: 1, Range: 1–100.

Internal Netatalk operations (file/directory create, delete, rename)
always update dircache entries immediately regardless of this setting.
If Netatalk is the only process accessing the volume,
you can safely set a value of 100 for maximum performance.

dircache mode = *lru* | *arc* (default: *lru*) **(G)**

Cache replacement algorithm. Netatalk supports two cache eviction policies:

- **LRU (Least Recently Used)** — the legacy algorithm and the default.
  Maintains a single list ordered by access time (FIFO);
  the least recently accessed entry is evicted when the cache is full.
  Simple and memory-efficient.

- **ARC (Adaptive Replacement Cache)** — a modern self-tuning algorithm
  that dynamically balances *recency* and *frequency* to achieve better cache hit rates.
  Based on
  ["Outperforming LRU with an Adaptive Replacement Cache Algorithm"](https://theory.stanford.edu/~megiddo/pdf/IEEE_COMPUTER_0404.pdf)
  (Megiddo & Modha, IBM, 2004).

ARC adapts to the observed access pattern,
making it effective for workloads that mix temporal locality with frequency-based reuse —
for example, when users alternate between recently and frequently accessed files or directories.
It maintains four lists: two for cached entries (T1 recent, T2 frequent)
and two *ghost* lists (B1, B2) that track recently evicted entries.
When a ghost entry is re-accessed, ARC learns from the miss and adjusts its policy.
This ghost learning and segmentation is what distinguishes ARC from LRU,
and also makes ARC resistant to sequential scans and backups that can flush an LRU cache.

**Netatalk's ARC uses complete ghost entries** (retaining all content)
rather than the metadata-only ghosts in the IBM paper,
because cache entries are small.
A ghost hit is promoted back to the cache instantly without any filesystem lookup,
so ARC should always perform at least as well as LRU in edge cases and better in all others.

**Memory:** ARC uses approximately **twice the memory** of LRU
because it tracks up to *c* ghost entries alongside *c* cached entries
(where *c* is *dircache size*).
Each entry is ~192 bytes on 64-bit systems.
Even so, ARC still outperforms an LRU cache of the same total memory.

| Mode | Entries tracked | Memory (1000-entry cache) |
| ---- | --------------- | ------------------------- |
| LRU  | 1000 cached     | ~192 KB                   |
| ARC  | 1000 cached + up to 1000 ghosts | ~384 KB   |

For the default *dircache size* of 65536: LRU ≈ 12 MB, ARC ≈ 24 MB per connected user.

**Recommendation**: Use **arc** for servers with 4 GB or more RAM.
Use **lru** for memory-constrained systems.

Default: lru.

## Resource Fork Caching

Netatalk can optionally cache resource fork data
(AppleDouble *._* data in classic Mac OS,
or extended attributes in modern macOS/Linux/UNIX systems) in a tier-2 dircache layer.
This avoids repeated storage I/O when AFP clients enumerate directories
with FPGetFileDirParams or FPEnumerate and request resource fork data.
Resource forks store many macOS data structures such as folder cover art (icns data).
Users who set custom directory icons, for example,
will observe significant performance gains from the resource fork cache layer.

Resource fork caching is disabled by default and is controlled by two settings:

dircache rfork budget = *number* (default: *0*) **(G)**

Total memory budget in KB for caching resource fork data per connected user.
When set to 0 (the default), resource fork caching is disabled.

Maximum: 10485760 (10 GB in KB).

dircache rfork maxsize = *number* (default: *1024*) **(G)**

Maximum size in KB of a single resource fork entry that will be cached.
Resource forks larger than this value are not cached even if the total budget has space remaining.

Maximum: 10240 (10 MB in KB).

Cached entries are automatically invalidated when the file's ctime or inode changes,
or when an AFP client modifies the resource fork.
Entries are evicted in LRU/ARC order when the budget is exceeded.

**Example** (100 MB rfork budget, max 5 MB per entry):

    dircache rfork budget = 102400
    dircache rfork maxsize = 5120

**Example** (Netatalk-only access to volume):

    dircache validation freq = 100

**Example** (file-heavy workload with large cache):

    dircache size = 262144
    dircache validation freq = 100
    dircache mode = arc

**Note**: Monitor dircache effectiveness by checking Netatalk log files
for "dircache statistics:" lines when **afpd** shuts down gracefully
(on user disconnect) or gets the SIGHUP signal.
