#!/usr/bin/env python3
"""Deltas of FreeBSD kernel counters between two sysctl snapshots.

usage: counters_delta.py before.txt after.txt

Each snapshot is 'sysctl vfs kern.maxvnodes kstat.zfs.misc.arcstats' output
("name: value" lines), as collect.sh writes it. The counters are
system-wide: take the snapshots on a quiet server, or read them next to the
per-process DTrace counts.
"""

import sys

WATCH = [
    ("vfs.cache.stats.", "name cache"),
    ("vfs.numvnodes", "vnodes"),
    ("vfs.freevnodes", "vnodes"),
    ("kern.maxvnodes", "vnodes"),
    ("vfs.recycles", "vnodes"),
    ("vfs.vnode_stats.", "vnodes"),
    ("vfs.vnlru.", "vnodes"),
    ("kstat.zfs.misc.arcstats.", "ZFS ARC"),
]
LEVELS = ("vfs.numvnodes", "vfs.freevnodes", "kern.maxvnodes",
          "kstat.zfs.misc.arcstats.size", "kstat.zfs.misc.arcstats.c",
          "kstat.zfs.misc.arcstats.c_max",
          "kstat.zfs.misc.arcstats.arc_meta_used",
          "kstat.zfs.misc.arcstats.metadata_size")


def load(path):
    values = {}
    with open(path, errors="replace") as f:
        for line in f:
            if ":" not in line:
                continue
            name, _, value = line.partition(":")
            try:
                values[name.strip()] = int(value.strip())
            except ValueError:
                pass
    return values


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    before, after = load(sys.argv[1]), load(sys.argv[2])
    rows = []
    for name in sorted(set(before) & set(after)):
        group = next((g for prefix, g in WATCH if name.startswith(prefix)),
                     None)
        if group is None:
            continue
        delta = after[name] - before[name]
        if name in LEVELS:
            rows.append((group, name, "%d -> %d" % (before[name],
                                                    after[name])))
        elif delta:
            rows.append((group, name, "%+d" % delta))
    for group in dict.fromkeys(g for _, g in WATCH):
        lines = [r for r in rows if r[0] == group]
        if lines:
            print("== %s" % group)
            for _, name, text in lines:
                print("   %-48s %s" % (name, text))

    def d(name):
        return after.get(name, 0) - before.get(name, 0)

    hits = d("vfs.cache.stats.poshits")
    neg = d("vfs.cache.stats.neghits")
    miss = d("vfs.cache.stats.miss")
    if hits or neg or miss:
        print("\nname cache: %d hits, %d negative hits, %d misses, %.1f%% of "
              "lookups missed" % (hits, neg, miss,
                                  100.0 * miss / (hits + neg + miss)))
    calls = d("vfs.cache.stats.fullpathcalls")
    if calls:
        print("path builds past the lockless walk: %d, %d found, %d failed"
              % (calls, d("vfs.cache.stats.fullpathfound"),
                 d("vfs.cache.stats.fullpathfail2")
                 + d("vfs.cache.stats.fullpathfail4")))
    mh = d("kstat.zfs.misc.arcstats.demand_metadata_hits")
    mm = d("kstat.zfs.misc.arcstats.demand_metadata_misses")
    if mh or mm:
        print("ARC metadata: %d hits, %d misses (each miss reads from disk "
              "or an L2ARC device)" % (mh, mm))
    total, top = after.get("vfs.numvnodes"), after.get("kern.maxvnodes")
    if total and top:
        used = total - after.get("vfs.freevnodes", 0)
        print("vnodes: %d of %d in use (%.0f%%), %d free, %+d recycled"
              % (used, top, 100.0 * used / top, total - used,
                 d("vfs.recycles")))


if __name__ == "__main__":
    main()
