# cnid_dbd Wire Protocol

Private TCP/IP protocol between **cnid_dbd** and the libatalk client linked into **afpd** / **nad**.

Valid as Netatalk release 4.5

## Transport

- TCP/IP, `SOCK_STREAM`, IPv4/IPv6 via `getaddrinfo`. Default `localhost:4700`
  (`DEFAULTHOST` / `DEFAULTPORT` in `etc/cnid_dbd/cnid_metad.c`).
  `setsockopt(SOL_TCP, TCP_NODELAY)`.
- Per-volume **afp.conf** options `cnid server` / `cnid port` allow remote-host deployments.
- One TCP connection per afpd worker; reconnect-with-retry handled by `transmit()` /
  `transmit_locked()` in `libatalk/cnid/dbd/cnid_dbd.c`.
- The AF_UNIX `socketpair` in `etc/cnid_dbd/cnid_metad.c` is **internal to cnid_metad** for
  fd-passing to forked children; it is **not** the request/reply channel.

## Framing

Each request/reply is a fixed-size struct write followed optionally by `namelen` bytes of variable
payload. Receiver reads `sizeof(struct cnid_dbd_{rqst,rply})` via `readt()`, then `namelen` bytes
if non-zero.

## Byte order

All struct fields are written in **native byte order**. Heterogeneous-endian remote-`cnid server`
deployments are not supported. CNIDs are stored as host-encoded network-byte-order `uint32_t` in
the BDB primary table and travel network-byte-order across the wire by virtue of how CNIDs are
constructed.

## Op codes

| Mnemonic | Value | Description |
|---|---|---|
| `CNID_DBD_OP_OPEN` | 0x01 | Volume-attach handshake |
| `CNID_DBD_OP_CLOSE` | 0x02 | Reserved |
| `CNID_DBD_OP_ADD` | 0x03 | Allocate or look up CNID for path |
| `CNID_DBD_OP_GET` | 0x04 | Look up CNID by (DID, name) |
| `CNID_DBD_OP_RESOLVE` | 0x05 | Resolve CNID → (parent DID, name) |
| `CNID_DBD_OP_LOOKUP` | 0x06 | Look up CNID by (DID, name, dev/ino) |
| `CNID_DBD_OP_UPDATE` | 0x07 | Update entry metadata |
| `CNID_DBD_OP_DELETE` | 0x08 | Delete a CNID |
| `CNID_DBD_OP_MANGLE_ADD` | 0x09 | Add a mangled-name mapping |
| `CNID_DBD_OP_MANGLE_GET` | 0x0a | Look up a mangled-name mapping |
| `CNID_DBD_OP_GETSTAMP` | 0x0b | Return database stamp |
| `CNID_DBD_OP_REBUILD_ADD` | 0x0c | Rebuilder-only add with forced CNID |
| `CNID_DBD_OP_SEARCH` | 0x0d | Filename substring search (paginated) |
| `CNID_DBD_OP_WIPE` | 0x0e | Drop the database |

## Reply codes

| Mnemonic | Value | Meaning |
|---|---|---|
| `CNID_DBD_RES_OK` | 0x00 | Success |
| `CNID_DBD_RES_NOTFOUND` | 0x01 | Lookup miss |
| `CNID_DBD_RES_ERR_DB` | 0x02 | Database / request error |
| `CNID_DBD_RES_ERR_MAX` | 0x03 | CNID space exhausted |
| `CNID_DBD_RES_ERR_DUPLCNID` | 0x04 | Duplicate CNID during rebuild |
| `CNID_DBD_RES_SRCH_CNT` | 0x05 | SEARCH: batch full; more may exist |
| `CNID_DBD_RES_SRCH_DONE` | 0x06 | SEARCH: final batch |

## SEARCH pagination

For `op == CNID_DBD_OP_SEARCH` the variable-length payload is:

```text
+---------------------+----------------------+
| srch_offset (4 B,   | search-name bytes    |
|  native order)      | (namelen - 4 bytes)  |
+---------------------+----------------------+
```

Daemon validation: `namelen >= 4`, `0 <= srch_offset <= DBD_SEARCH_MAX_OFFSET` (50000),
`1 <= name_len <= MAXPATHLEN - 4`. The wire-level `rqst.namelen <= MAXPATHLEN` cap is enforced at
`etc/cnid_dbd/comm.c` — any over-budget request is dropped before it
reaches `dbd_search`. The 4-byte offset prefix consumes part of that budget, leaving
`MAXPATHLEN - 4` for the search-name. Failures inside `dbd_search` reply `RES_ERR_DB` (return
value `1` from `dbd_search`; not fatal).

**Daemon-side reply-code semantics (`SRCH_CNT` vs `SRCH_DONE`)**: the daemon emits `SRCH_CNT` iff
`dbif_search` confirms a `(srch_offset + DBD_MAX_SRCH_RSLTS + 1)`-th matching entry exists in the
secondary index — the cursor walk performs a one-step *peek* immediately after the buffer fills,
and tests the result for `(DB_NOTFOUND ∨ prefix-mismatch ∨ engine-error)` vs `(success ∧
prefix-match)`. The legacy "answer `SRCH_CNT` whenever `count == DBD_MAX_SRCH_RSLTS`" heuristic is
**incorrect** because it cannot distinguish the boundary case where the matching range contains
exactly `srch_offset + DBD_MAX_SRCH_RSLTS` entries (no further matches; correct answer is
`SRCH_DONE`) from the case with strictly more entries (correct answer is `SRCH_CNT`). See
`etc/cnid_dbd/dbif.c::dbif_search` for the post-fill peek
implementation; `dbif_search`'s `bool *more` out-parameter carries the result up to `dbd_search`,
which keys `rply.result` on it.

Client (`cnid_dbd_find`) terminates the pagination loop on:

1. `SRCH_DONE` reply (natural completion; `more_available = false`).
2. `(max_results - total) < DBD_MAX_SRCH_RSLTS` (buffer-fit; `more_available = true`).
3. `offset >= DBD_SEARCH_MAX_OFFSET` (offset cap; `more_available = true`).
4. `time(NULL) >= deadline_ts` (10 s budget; `more_available = true`).
5. Any error path → return `-1` with `errno = CNID_ERR_DB`.

Pagination is **stateless on the daemon side**: each request re-opens a fresh BDB cursor and skips
`srch_offset` matching entries. Concurrent ADD/DELETE between batches may produce duplicates or
misses; the macOS `kMDQuery` UI deduplicates by path. Crash-resilient resume: a daemon restart
between batches re-walks from the same offset.

> **Determinism invariant for raw-count assertions**: any test fixture
> that asserts a strict `got == N` count over paginated results MUST
> guarantee no concurrent CNID ADD/DELETE on the volume during the
> query. Concurrent mutation between paginated SEARCH batches can
> produce duplicates or misses on the DBD backend (stateless cursor
> re-walk); `test530`/`test531` use static, test-private corpora and
> long unique filename prefixes for this reason.

## Single-threaded daemon, per-volume isolation

The cnid_dbd dispatch loop (`etc/cnid_dbd/main.c`) is single-threaded.
A long paginated search blocks it; other CNID ops on the same volume serialise behind it.
Per-batch BDB latency is sub-millisecond on a warm cache; ~100 batches × ~1 ms ≈ 100 ms typical
for a 10000-result search. The 10 s end-to-end deadline is the hard backstop.

`cnid_metad` forks **one cnid_dbd child per volume** (keyed on volume path; see
`maybe_start_dbd` in `cnid_metad.c`). All afpd workers attached to the same volume share that
one child.
cnid_metad respawns any dead child on the next client request.

**Wall-clock bound.** `DBD_FIND_DEADLINE_SEC` is best-effort. The deadline is checked at the top
of each batch iteration; once `transmit_locked()` is called, it may take up to `MAX_DELAY` seconds
(= 20 s) to resolve a flaky connection via its internal reconnect loop. The hard upper bound on
`cnid_dbd_find()` is therefore approximately `DBD_FIND_DEADLINE_SEC + MAX_DELAY` = 30 s, not 10 s.
The typical case is far below either bound.

## Cancellation

The Spotlight pipeline runs synchronously in the AFP worker. While `cnid_find()` is in flight the
worker is blocked in `transmit_locked()` and is not reading the DSI socket; an in-flight
`closeQueryForContext:` cannot reach `slq_cancel()` until pagination returns. Worst-case wasted
work for a cancelled query is `DBD_FIND_DEADLINE_SEC = 10` s (or up to ~30 s in the presence of a
daemon reconnect, per the wall-clock note above). Async cancellation requires architecture changes
beyond this protocol and is a future work item.

## Upgrade contract

This wire format is the private TCP/IP RPC spoken between three components: the libatalk CNID
client (linked into `afpd` and `nad`), the per-volume `cnid_dbd` backend daemon, and the `cnid_metad`
supervisor that forks `cnid_dbd` instances on demand. All three sides must agree on the
on-the-wire layout of `struct cnid_dbd_rqst`, `struct cnid_dbd_rply`, and the SEARCH-payload
prefix described above.

Wire-format changes are therefore a hard break - Netatalk does not support mixed-version
deployments: every host that runs any of the components must be upgraded together, and the
running process group must be restarted so the new binaries take effect. Same-host deployments
just restart Netatalk which restarts all three components; remote `cnid server = <host>` deployments must restart
`cnid_metad` on remote host as well.

## Changes

Netatalk 4.5 release note; wire change is **SEARCH-only** — non-SEARCH opcodes are byte-identical to the
previous release. Any unexpected SEARCH reply code from the daemon is a protocol violation and
`abort()`s the AFP worker, matching the convention of every other `cnid_dbd_*` op.
