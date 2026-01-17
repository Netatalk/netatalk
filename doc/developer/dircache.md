# Directory Cache Optimization

*The Hidden Cost of Validation: How Netatalk's Directory Cache Optimization Reduces Storage Stack Pressure*

## Introduction: The Invisible Performance Tax

When you access a file on a network share, you're triggering a cascade of operations through multiple cache layers.
Each layer tries to optimize performance, but they can work against each other in surprising ways.
This article explores how Netatalk's probabilistic directory cache validation reduces I/O
by working with the storage page cache.

---

## Part 1: The Storage Stack

Modern file servers orchestrate multiple cache layers, each with its own purpose and behavior:

```mermaid
graph TB
    subgraph "Application Layer"
        AFP[AFP Client Request]
        NT[Netatalk Server]
        DC[Directory Cache<br/>LRU]
        CNID[CNID Database]
    end
    
    subgraph "Kernel Layer"
        PC[Page Cache<br/>LRU]
        VFS[VFS Layer]
    end
    
    subgraph "Storage Layer"
        FS[Filesystem<br/>ZFS/ext4/XFS]
        ARC[ZFS ARC Cache<br/>Optional]
        DISK[Physical Storage]
    end
    
    AFP --> NT
    NT --> DC
    DC --> CNID
    NT --> VFS
    VFS --> PC
    PC --> FS
    FS --> ARC
    ARC --> DISK
    
    style PC fill:\#ffcccc
    style DC fill:\#ccffcc
    style ARC fill:\#ffcccc
```

Each cache layer uses an **LRU (Least Recently Used)** algorithm to decide what to keep in memory
(ZFS uses ARC instead of LRU).
This works well when access patterns match LRU assumptions, but breaks down with certain workloads
(especially scan workloads)—and that's where our story begins.

---

## Part 2: The Page Cache Problem

The Linux page cache is incredibly fast and nearly invisible.
It automatically caches file data and metadata in RAM, but here's the catch:
**you can't directly control what stays in cache**.

LRU Performance Issues:

Degeneration Under Common Patterns: LRU can perform poorly with certain access patterns.
For example, if an application loops over an array larger than the number of pages in the cache,
it will cause a page fault for every access, leading to inefficiency
(the cache churns and does not hold data long enough to achieve a good hit rate).
Netatalk currently has a small maximum dircache value (dircachesize = 131072)
as the implementation performance degrades above this limit.

Cache Pollution: LRU may evict pages that are frequently accessed but not necessarily the most relevant,
which can lead to less optimal cache performance.
For example when enumerating paths to move around a directory tree,
Netatalk historically (and still does today by default,
until you apply the new `afp.conf` tuning options shown below)
performed an IO stat operation every time it reads a dircache entry to validate the file/directory still exists.
These IO stat validations allow Netatalk to detect external filesystem changes,
but keep unneeded data warm in the page cache,
pushing other more important objects off the page cache and increasing disk IO -
even though Netatalk has the dircache containing everything needed.

### How Directory Enumeration Works

When a client browses a Netatalk network folder, here's the actual sequence:

```mermaid
sequenceDiagram
    participant Client as AFP Client
    participant Netatalk
    participant Disk as Filesystem
    participant PageCache as Page Cache
    participant DirCache as Directory Cache
    
    Client->>Netatalk: List folder contents (FPEnumerate)
    Netatalk->>Disk: readdir() - get entry names
    Disk->>Netatalk: Return entry names
    
    loop For EACH entry
        Netatalk->>PageCache: stat() each entry
        alt Cache miss
            PageCache->>Disk: Read inode metadata
            Disk->>PageCache: Load metadata
            Note over PageCache: Evicts LRU data
        end
        PageCache->>Netatalk: Return stat data
        
        alt Entry is directory
            Netatalk->>DirCache: Check if cached
            alt Not in cache
                Netatalk->>DirCache: Add directory to cache
            end
        else Entry is file
            Netatalk->>DirCache: Check if cached (if file caching enabled)
            alt Not in cache
                Netatalk->>DirCache: Add file to cache (if enabled)
            end
        end
    end
    
    Netatalk->>Client: Return file list with metadata
```

**Key insights from enumerate.c**:

1. All entries are read from filesystem via `readdir()` and `stat()` all - high page-cache/disk demand
2. **Directories**: Cache checked before adding, avoiding duplicate entries
3. **Files**: Cache is NOT checked during enumeration - always processed via `getfilparams()`
which may add them to cache internally

**Current enumeration inefficiency:**

- **Every file** is `stat()`'ed on every enumeration
- **Every directory** is `stat()`'ed, then cache is checked to avoid duplicate entries
- Repeatedly browsing a folder with 10,000 files = 10,000 stat calls each time (LRU scanning problem)

**Future optimization opportunity - Cache-First Enumeration:**

Optimal approach:

1. **readdir()**: Get list of entry names from filesystem (unavoidable - must know what exists)
2. **For each entry**: Check `dircache_search_by_name()` first
   - **Cache HIT**: Use cached metadata
   - **Cache MISS**: Fall back to `stat()` and add to cache
3. **Result**: Second enumeration of same folder could be ~99% cache-served (only 1% stat for probabilistic validation)

Would potentially transform repeated directory browsing from O(n) stat calls to O(n/100) stat calls,
eliminating most filesystem I/O.

### The LRU Scanning Problem

LRU caches have a fundamental weakness called the **scanning problem**:

When Netatalk validates thousands of entries, each `stat()` call:

1. Checks the page cache
2. Can causes a cache miss (directories and directory trees have many entries)
3. Loads the inode from disk / or reads from page cache (either adds to page cache or refreshes timer to keep in page cache)
4. **Evicts something potentially more valuable**

Parent folder recursion:

Each directory entry, stores its parent ID, and each parent ID has its own directory cache entry,
providing a recursive chain (path) to root.
When a new directory is accessed/added,
dirlookup() recursively calls itself until it finds a cached ancestor or reaches volume root.
Every recursive lookup also results in many more stat calls.
So even opening a small folder directly,
still requires stat'ing every level of the whole path to be pushed into the page cache.

If the dircache max size is small (by default just 8192 entries), as you move around your file share,
old entries are pushed off (evicted) as new ones are added.
This high entry rotation is known as "scan eviction" and means by the time you want to go back to a previous directory
and read a cached entry, it has likely already been evicted which can cause a cascade effect of recursive lookups
and stats calls to restore the broken cached paths if parent entries are evicted.
So unless your whole file server has less than 8192 file and directories,
it is recommended to increase the `dircachesize` value in [afp.conf](https://netatalk.io/manual/en/afp.conf.5).

Future releases will increase the maximum size of the dircache once existing performance issues are addressed.
We are also considering retaining directories over files during eviction
to reduce recursive path discoveries after parent folders are evicted.

---

## Part 3: Why This Goes Unnoticed

The page cache operates at memory speed when it hits, making overhead hard to measure:

The real cost isn't the individual `stat()` calls—it's the **cascade effect**:

- Your working set gets evicted from page cache
- Filesystem queries slow down
- Application data must be re-read from disk
- Overall system responsiveness degrades

However, there is only a single Page Cache per kernel.
So this new dircache change in Netatalk to validate dircache periodically
means less interference with other services on the same instance.

---

## Part 4: How Files Are Found - The Lookup Hierarchy

Before understanding the probabilistic solution,
let's see how Netatalk finds files and the precedence between cache layers:

### Initial File Discovery Flow

```mermaid
flowchart TD
    Request[Client requests file/folder]
    
    Request --> DC_Check{In Directory<br/>Cache?}
    
    DC_Check -->|Cache Hit| DC_Valid{Validate?}
    DC_Check -->|Cache Miss| CNID_Query[Query CNID Database]
    
    DC_Valid -->|Every Nth request| Validate[stat filesystem]
    DC_Valid -->|Other requests| Use_Cached[Use cached entry]
    
    Validate -->|Still valid| Use_Cached
    Validate -->|Invalid| Remove_Entry[Remove from cache]
    
    Remove_Entry --> CNID_Query
    
    CNID_Query -->|Found| FS_Check[stat filesystem]
    CNID_Query -->|Not found| Create_New[Create new CNID]
    
    FS_Check -->|Exists| Add_Cache[Add to dircache]
    FS_Check -->|Missing| Error[Return error]
    
    Create_New --> Add_Cache
    Add_Cache --> Return[Return to client]
    Use_Cached --> Return
    
    style DC_Check fill:\#e6f3ff
    style CNID_Query fill:\#ffe6e6
    style FS_Check fill:\#ffffe6
```

### The Precedence Hierarchy

```mermaid
graph TB
    subgraph "Lookup Precedence"
        L1[Directory Cache - Fastest - Memory]
        L2[CNID Database - Fast - BerkeleyDB]
        L3[Filesystem - Slow - Disk I/O]
        
        L1 -->|Miss or Invalid| L2
        L2 -->|Verify exists| L3
    end
    
    subgraph "Data Location"
        D1[Dircache - Paths and Metadata]
        D2[CNID - ID to Path mappings]
        D3[Page Cache - File data and inodes]
        D4[Disk - Authoritative source]
    end
    
    L1 -.-> D1
    L2 -.-> D2
    L3 -.-> D3
    D3 -.-> D4
```

### Key Points

1. **Directory Cache** is checked first (fastest, in-memory)
2. **CNID Database** provides persistent ID→path mappings
3. **Filesystem** is the ultimate authority (via stat calls)
4. **Page Cache** transparently accelerates filesystem access

## Part 5: The Probabilistic Solution

With the lookup hierarchy understood, here's how probabilistic validation optimizes the flow:

```mermaid
stateDiagram-v2
    [*] --> CacheLookup: Directory Request
    
    state ValidationDecision {
        [*] --> CheckCounter
        CheckCounter --> IncrementCounter
        IncrementCounter --> Modulo: counter % freq
        Modulo --> DoValidate: == 0 (validate)
        Modulo --> SkipValidate: != 0 (skip)
    }
    
    CacheLookup --> Found: Cache Hit
    CacheLookup --> NotFound: Cache Miss
    
    Found --> ValidationDecision
    
    DoValidate --> StatCall: Filesystem stat
    SkipValidate --> TrustCache: Use cached data
    
    StatCall --> Valid: Still exists
    StatCall --> Invalid: Gone/moved
    
    Valid --> UpdateMetadata: Refresh metadata
    Invalid --> RemoveEntry: Evict from cache
    
    RemoveEntry --> QueryCNID: Rebuild from CNID
    UpdateMetadata --> ServeClient
    TrustCache --> ServeClient
    NotFound --> QueryCNID
    QueryCNID --> ServeClient
    
    ServeClient --> [*]: Return to client
    
    note right of SkipValidate: 99% take this path<br/>No filesystem I/O
```

The key insight: **We don't skip the cache hierarchy, we skip the validation step**.
Files are still properly discovered through CNID when not cached,
but we avoid repeatedly verifying cached entries still exist.

### Configuration Example

```ini
# afp.conf
dircache validation freq = 100  # Validate 1 in 100 lookups
```

With this setting:

- **99% of stat() calls eliminated**
- Page cache remains focused on hot data
- Any Invalid entries (files/dirs/paths changed outside of Netatalk) are caught and corrected on use
- Self-healing through CNID re-query on failures

---

## Part 6: Understanding Cache Lifetimes

Different cache layers have different retention strategies:

```mermaid
graph TB
    subgraph "Netatalk Dircache"
        DC1[Entry Added]
        DC1 --> DC2[LRU Queue]
        DC2 --> DC3[Validated 1/N times]
        DC3 --> DC4[LRU Evicted when full]
    end
```

```mermaid
graph TB
    subgraph "Page Cache"
        PC1[Page Loaded]
        PC1 --> PC2[Active List]
        PC2 --> PC3[Inactive List]
        PC3 --> PC4[LRU Reclaimed under pressure]
    end
```

```mermaid
graph TB
    subgraph "ZFS ARC"
        ARC1[Block Cached]
        ARC1 --> ARC2[MRU List]
        ARC2 --> ARC3[MFU List]
        ARC3 --> ARC4[Ghost Lists]
        ARC4 --> ARC5[ARC Evicted by ARC sizing]
    end
```

By trusting and reducing validation calls at the fastest layer (dircache),
we reduce pressure on the subsequent cache layers:

- **Dircache**: Keeps frequently accessed directory entries
- **Page Cache**: Retains genuinely hot data
- **ZFS ARC**: Maintains frequently/recently used blocks

---

## Part 7: Monitoring the Improvement

You can observe the benefits of dircache validation optimization using these metrics:

- **Dircache statistics via logs**: Monitor `invalid_on_use` counter (should be low, indicating cached entries remain valid)
- **vmstat page cache activity**: Look for lower page scan rates (less cache churn)
- **iostat disk IOPS**: Measure reduced read IOPS (fewer stat calls to disk)
- **ZFS ARC stats**: Check for higher hit ratios (more efficient cache utilization)

### Sample Statistics

The actual log format from dircache.c:

```txt
dircache statistics: (user: john)
entries: 4096, max_entries: 8192, config_max: 131072,
lookups: 100000, hits: 95000 (95.0%), misses: 5000,
validations: ~950 (1.0%),
added: 5000, removed: 900, expunged: 10, invalid_on_use: 2, evicted: 256,
validation_freq: 100
```

**Key metrics to monitor:**

- **invalid_on_use**: Should remain low - indicates cached entries stay valid
- **validations ratio**: Shows validation frequency working as configured (1.0% = every 100th)
- **hit_ratio**: High percentage (95%) indicates good cache effectiveness
- **max_entries**: Peak/high-water cache usage this session - helps tune `dircachesize`

---

## Conclusion

The real benefit of this optimization isn't just the eliminated stat() calls—
it's the compound effect across the entire storage stack:

By understanding how cache layers interact and compete,
we can make intelligent optimizations that benefit the entire system.
The probabilistic validation approach shows that sometimes the best optimization isn't doing something faster—
it's not doing it at all.

### Key Takeaways

1. **Page cache pollution is invisible but expensive**
2. **LRU caches suffer from scanning patterns**
3. **Probabilistic validation maintains correctness while improving performance**
4. **Reducing I/O helps every layer of the storage stack**
5. **Small changes can have compound effects**

**By default nothing changes, as the default value for `dircache validation freq = 1`.**

### File Caching Implementation

In directory.c, the `dirlookup_internal()` function
controls file caching behavior through the `AFPobj->options.dircache_files` configuration setting:

- **Cache lookup**: When a file is found in cache, checks `AFPobj->options.dircache_files`
  before returning it — if disabled, returns AFPERR_BADTYPE to reject the cached file entry
- **CNID lookup**: When a file is resolved via CNID, sets the `DIRF_ISFILE` flag
  which will cause cache rejection if file caching is disabled
- **Parent recursion**: Uses `strict` parameter to control whether parents must be directories,
ensuring parent-chain rebuilds work correctly (database design guarantee)

This configuration-driven control separates caching policy from core lookup logic, allowing
administrators to tune memory vs. performance tradeoffs based on workload characteristics.

---

*Directory cache optimizations available in Netatalk 4.4+ with configurable validation frequency
and file caching control via `dircache validation freq` and `dircache files` parameters.*

Developed and Authored by Andy Lemin, with Contributions from the Netatalk team.
