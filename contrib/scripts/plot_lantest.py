#!/usr/bin/env python3

# This script renders an afp_lantest CSV capture into a per-test latency
# candlestick chart (PNG) for publishing in CI, mirroring the speedtest
# perfgraph workflow.
#
# (c) 2026 Andy Lemin (@andylemin)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

"""Render afp_lantest CSV output as a per-test latency candlestick chart.

afp_lantest, run with -c, prints one summary row per test:

    Test,Time_ms,Time±,Min_ms,Max_ms,Median_ms,MB/s
    Open, stat and read 512 bytes from 1000 files [8,000 AFP ops],360,33.6,324,406,349,0
    ...
    Sum of all AFP OPs = 80686,4921,,

Two parsing wrinkles handled here:
  * test names contain commas, so rows are parsed right-anchored — the numeric
    columns are the trailing fields, the name is everything before them.
  * a trailing "Sum of all AFP OPs" row is ignored.

Each test becomes a candlestick: thin wick = min..max, box = mean ± stddev,
horizontal line = median. Y is time in ms (log scale, since tests span ~2ms to
~1300ms). Lower is faster.

Usage:
    plot_lantest.py INPUT.csv [-o OUTPUT_BASENAME] [--title TITLE]
                    [--subtitle SUBTITLE] [--meta KEY=VALUE ...]

Writes OUTPUT_BASENAME.png (default basename: "lantest").
"""

import argparse
import re
import sys

import matplotlib

matplotlib.use("Agg")  # headless / no display, for CI runners
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

# Background grey matching the FlameGraph SVG palette (RGB 238,238,238).
BACKGROUND_COLOR = "#EEEEEE"
BOX_COLOR = "#1f77b4"
MEDIAN_COLOR = "#d62728"

# A summary data row ends with the numeric columns the candlestick needs:
#   ...,Time_ms,Time±,Min_ms,Max_ms,Median_ms,MB/s
# Match those trailing fields so a comma-containing test name is left intact.
ROW_TAIL_RE = re.compile(
    r"^(?P<name>.+),"
    r"(?P<mean>\d+),(?P<stddev>[\d.]+),"
    r"(?P<min>\d+),(?P<max>\d+),(?P<median>\d+),"
    r"(?P<mbs>\d+)\s*$"
)


# Map each lantest test (matched by a substring of its full name) to a short
# axis label. Order matters: first matching substring wins.
SHORT_LABELS = [
    ("Open, stat and read", "Open/stat/read"),
    ("Writing one large file", "Write 100MB"),
    ("Reading one large file", "Read 100MB"),
    ("Locking/Unlocking", "Lock/unlock"),
    ("Creating dir with", "Create 2000"),
    ("Enumerate dir", "Enumerate"),
    ("Deleting dir", "Delete 2000"),
    ("Create directory tree", "Create tree"),
    ("Directory cache hits", "Dircache hits"),
    ("Mixed cache operations", "Mixed cache"),
    ("Deep path traversal", "Deep traverse"),
    ("Cache validation", "Cache validate"),
]


def short_label(name):
    """Map a verbose lantest test name to a short axis tick label.

    Falls back to the name with its trailing "[N AFP ops]" annotation stripped
    if no keyword matches (keeps new/renamed tests readable).
    """
    for needle, label in SHORT_LABELS:
        if needle in name:
            return label
    # Strip a trailing "[...]" annotation using plain string ops (no regex,
    # so no backtracking/ReDoS risk).
    stripped = name.strip()
    if stripped.endswith("]"):
        bracket = stripped.rfind("[")
        if bracket != -1:
            stripped = stripped[:bracket]
    return stripped.strip()


def parse_csv(path):
    """Parse afp_lantest CSV into an ordered list of per-test dicts.

    Each dict has: name, label, mean, stddev, min, max, median (floats except
    the strings). Rows are returned in file order (the test execution order).
    """
    tests = []
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("Sum of all AFP OPs"):
                continue
            m = ROW_TAIL_RE.match(line)
            if not m:
                continue  # header, log noise, dircache stats, etc.
            tests.append({
                "name": m.group("name"),
                "label": short_label(m.group("name")),
                "mean": float(m.group("mean")),
                "stddev": float(m.group("stddev")),
                "min": float(m.group("min")),
                "max": float(m.group("max")),
                "median": float(m.group("median")),
            })
    return tests


def _draw_candle(ax, x, t):
    """Draw one test's candlestick at column x: wick=min..max, box=mean±std."""
    lo, hi = t["min"], t["max"]
    box_lo = max(t["mean"] - t["stddev"], lo)
    box_hi = min(t["mean"] + t["stddev"], hi)
    # Wick (full min..max range).
    ax.plot([x, x], [lo, hi], color=BOX_COLOR, linewidth=1.0, zorder=2)
    # Box (mean ± 1 stddev). Use a rectangle via bar for a filled body.
    ax.bar(x, box_hi - box_lo, bottom=box_lo, width=0.5,
           color=BOX_COLOR, alpha=0.35, edgecolor=BOX_COLOR,
           linewidth=1.0, zorder=3)
    # Median marker.
    ax.plot([x - 0.25, x + 0.25], [t["median"], t["median"]],
            color=MEDIAN_COLOR, linewidth=2.0, zorder=4)
    # Min/max caps for readability.
    ax.plot([x - 0.12, x + 0.12], [lo, lo], color=BOX_COLOR, linewidth=1.0,
            zorder=2)
    ax.plot([x - 0.12, x + 0.12], [hi, hi], color=BOX_COLOR, linewidth=1.0,
            zorder=2)


def build_info_lines(tests, meta):
    """Compose the run-config lines shown under the chart subtitle."""
    lines = [f"Tests: {len(tests)}"]
    if meta.get("iterations"):
        warm = f" +{meta['warmup']} warmup" if meta.get("warmup") else ""
        lines.append(f"Iterations: {meta['iterations']}{warm}")
    if meta.get("afp"):
        lines.append(f"AFP: {meta['afp']}")
    dc = []
    if meta.get("dircache_mode"):
        dc.append(meta["dircache_mode"].upper())
    if meta.get("dircache_size"):
        dc.append(f"size {meta['dircache_size']}")
    if meta.get("dircache_validation_freq"):
        dc.append(f"freq {meta['dircache_validation_freq']}")
    if dc:
        lines.append("Dircache: " + " · ".join(dc))
    return lines


def plot(tests, out_base, title, subtitle, meta=None):
    """Render the latency candlestick chart to <out_base>.png."""
    meta = meta or {}
    if not tests:
        sys.stderr.write(
            "ERROR: no lantest rows found in input — was afp_lantest run "
            "with -c (CSV)?\n"
        )
        return False

    fig, ax = plt.subplots(figsize=(11, 5.2))
    fig.patch.set_facecolor(BACKGROUND_COLOR)
    ax.set_facecolor(BACKGROUND_COLOR)

    xs = range(len(tests))
    for x, t in zip(xs, tests):
        _draw_candle(ax, x, t)

    ax.set_yscale("log")
    # Pad the log Y range below the smallest min and above the largest max so
    # no candle (e.g. a ~2 ms test) is clipped against the axis edge.
    data_min = min(t["min"] for t in tests)
    data_max = max(t["max"] for t in tests)
    ax.set_ylim(max(data_min / 1.6, 0.1), data_max * 1.6)
    # Plain millisecond integers on the log axis (no 10^n scientific ticks).
    ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
    ax.yaxis.set_minor_formatter(FuncFormatter(lambda v, _: ""))
    ax.set_ylabel("Time (ms) — lower is faster")
    ax.set_xticks(list(xs))
    ax.set_xticklabels([t["label"] for t in tests], rotation=25, ha="right",
                       fontsize=8)
    ax.set_xlim(-0.6, len(tests) - 0.4)
    ax.grid(True, axis="y", which="both", linestyle=":", alpha=0.4)

    # Legend explaining the candle anatomy.
    handles = [
        plt.Line2D([0], [0], color=BOX_COLOR, lw=1, label="min–max range"),
        plt.Rectangle((0, 0), 1, 1, facecolor=BOX_COLOR, alpha=0.35,
                      edgecolor=BOX_COLOR, label="mean ± std dev"),
        plt.Line2D([0], [0], color=MEDIAN_COLOR, lw=2, label="median (P95)"),
    ]
    legend = ax.legend(handles=handles, loc="upper left", fontsize=9,
                       framealpha=0.9)
    legend.get_frame().set_facecolor("#F5F5F5")

    header_lines = []
    if subtitle:
        header_lines.append(subtitle)
    info = build_info_lines(tests, meta)
    if info:
        header_lines.append(" · ".join(info))

    # Lay out the plot first, then place the title block manually so the gap
    # between the bold title and the subtitle is fixed (tight_layout leaves
    # uncontrollable slack between a suptitle and an axes title).
    fig.tight_layout(rect=(0, 0, 1, 0.88))
    suptitle = title or "Netatalk AFP Lantest — per-test latency"
    fig.text(0.5, 0.965, suptitle, ha="center", va="top",
             fontsize=14, fontweight="bold")
    if header_lines:
        fig.text(0.5, 0.905, "\n".join(header_lines), ha="center", va="top",
                 fontsize=8.5, color="#222222")

    png_path = f"{out_base}.png"
    fig.savefig(png_path, format="png", dpi=130, facecolor=fig.get_facecolor())
    plt.close(fig)
    sys.stderr.write(f"Wrote {png_path}\n")
    return True


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="lantest CSV capture (afp_lantest -c)")
    parser.add_argument(
        "-o", "--output", default="lantest",
        help="output basename (default: lantest -> lantest.png)",
    )
    parser.add_argument("--title", default=None, help="chart title")
    parser.add_argument("--subtitle", default=None, help="chart subtitle")
    parser.add_argument(
        "--meta", action="append", default=[], metavar="KEY=VALUE",
        help="run-config metadata for the header (repeatable). Known keys: "
             "afp, iterations, warmup, dircache_mode, dircache_size, "
             "dircache_validation_freq.",
    )
    args = parser.parse_args(argv)

    tests = parse_csv(args.input)
    meta = {}
    for pair in args.meta:
        if "=" in pair:
            k, v = pair.split("=", 1)
            meta[k.strip()] = v.strip()

    ok = plot(tests, args.output, args.title, args.subtitle, meta)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
