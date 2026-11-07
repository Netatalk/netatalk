#!/usr/bin/env python3
"""Per-request durations and cache events from an afpd maxdebug log.

usage: afpd_log.py afpd.<pid>.XXXXXX [more logs]

The log is the one afpd writes after the first SIGINT to a session process
(default:maxdebug, in the session's netatalk-<pid> temporary folder). Each
request is the span from "<== Start AFP command" to "==> Finished AFP
command". A request whose lines name "navmark-<label>" starts phase <label>.
maxdebug logging slows afpd: compare phases of one log, never against a log
taken at another level.
"""

import calendar
import collections
import re
import sys

TS = re.compile(r"^(\w{3}) +(\d+) (\d\d):(\d\d):(\d\d)\.(\d{6}) ")
START = re.compile(r"<== Start AFP command: (\S+)")
DONE = re.compile(r"==> Finished AFP command: (\S+) -> (\S+)")
MARK = re.compile(r"navmark-([A-Za-z0-9_]+)")
EVENTS = collections.OrderedDict([
    ("miss", re.compile(r"\{not in cache\}")),
    ("hit", re.compile(r"\{found in cache\}|\{cached \(unvalidated\)")),
    ("validated", re.compile(r"\{validated")),
    ("cnid_query", re.compile(r"querying CNID database")),
    ("walk", re.compile(r"movecwd\(to: did")),
    ("selfheal", re.compile(r"movecwd: stale dircache entry|self-healing")),
    ("added", re.compile(r"dir_add_fill\(.*\{cached:")),
    ("removed", re.compile(r"\{removed\}")),
    ("listing_read", re.compile(r"enumerate: cache miss")),
    ("ad_read", re.compile(r"ad_metadata\(|ad_open\(.*BEGIN")),
])
STATS = re.compile(r"(dircache statistics[^:]*:|pfd_cache:|enumerate "
                   r"statistics:|ARC operations:|ARC ghost performance:)(.*)")


MONTHS = ("Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
          "Oct", "Nov", "Dec")
YEAR_US = 366 * 86400 * 1000000


def usec(m):
    """The stamp in microseconds, with its day, in a leap year the log
    does not name; a request that spans the year's end reads YEAR_US back."""
    mon, day, h, mi, s, us = m.groups()
    days = sum(calendar.monthrange(2000, i)[1]
               for i in range(1, MONTHS.index(mon) + 1)) + int(day) - 1
    return (((days * 24 + int(h)) * 60 + int(mi)) * 60 + int(s)) * 1000000 \
        + int(us)


def parse(path, reqs, stats):
    cur = None
    # names such as "Icon\r" hold a CR, so only "\n" ends a line
    with open(path, errors="replace", newline="\n") as f:
        for line in f:
            m = TS.match(line)
            t = usec(m) if m else None
            s = START.search(line)
            if s and t is not None:
                cur = {"cmd": s.group(1), "t0": t, "ev": collections.Counter(),
                       "mark": None}
                continue
            d = DONE.search(line)
            if d and cur is not None and t is not None:
                cur["us"] = t - cur["t0"] + (YEAR_US if t < cur["t0"] else 0)
                cur["result"] = d.group(2)
                reqs.append(cur)
                cur = None
                continue
            st = STATS.search(line)
            if st:
                stats.append((st.group(1) + st.group(2)).strip())
            if cur is None:
                continue
            mk = MARK.search(line)
            if mk:
                cur["mark"] = mk.group(1)
            for name, rx in EVENTS.items():
                if rx.search(line):
                    cur["ev"][name] += 1


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    reqs, stats = [], []
    for path in sys.argv[1:]:
        parse(path, reqs, stats)
    phase = "all"
    phases = collections.OrderedDict()
    for r in reqs:
        if r["mark"]:
            phase = r["mark"]
            continue
        phases.setdefault(phase, []).append(r)
    for phase, rs in phases.items():
        total = sum(r["us"] for r in rs)
        ev = collections.Counter()
        for r in rs:
            ev.update(r["ev"])
        print("== phase %s: %d requests, %.1f ms in afpd" % (
            phase, len(rs), total / 1000.0))
        by = collections.OrderedDict()
        for r in rs:
            by.setdefault(r["cmd"], []).append(r["us"])
        print("   %-22s %6s %10s %9s %9s" % ("command", "n", "total_ms",
                                            "mean_us", "max_us"))
        for cmd, us in sorted(by.items(), key=lambda kv: -sum(kv[1])):
            print("   %-22s %6d %10.1f %9.0f %9d" % (
                cmd, len(us), sum(us) / 1000.0, sum(us) / len(us), max(us)))
        print("   events: " + ", ".join("%s %d" % (k, ev[k]) for k in EVENTS
                                        if ev[k]))
        for r in sorted(rs, key=lambda r: -r["us"])[:5]:
            print("   slow: %8d us %-20s %-14s %s" % (
                r["us"], r["cmd"], r["result"], dict(r["ev"])))
    if stats:
        print("== session statistics")
        for line in stats:
            print("   " + line)


if __name__ == "__main__":
    main()
