#!/usr/bin/env python3

# This script renders an afp_speedtest CSV capture into a throughput-vs-size
# chart (SVG + PNG) for publishing in CI, mirroring the flamegraph workflow.
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

"""Render afp_speedtest CSV output as a throughput-vs-file-size chart.

The speedtest tool (test/testsuite/speedtest.c) emits, when run with -c, one
"File Size Performance Summary" block per operation:

    # File Size Performance Summary for Read
    file_size_bytes,file_size_mb,mean_throughput_mbs,median_throughput_mbs,min_throughput_mbs,max_throughput_mbs,stddev_ms,mean_time_ms
    4096,0.004,66.15,65.65,52.79,93.01,0.0,0.1
    ...

This parser keys on those headers and ignores any surrounding log noise (the
container entrypoint prints setup messages on the same stdout), so the raw
`docker run ... > speed.csv` capture can be fed in directly.

Output: a single plot with X = file size (log2 scale) and Y = median
throughput (MB/s), one line per operation, with a shaded min..max band.

Usage:
    plot_speedtest.py INPUT.csv [-o OUTPUT_BASENAME] [--title TITLE]
                      [--subtitle SUBTITLE] [--baseline BASELINE.csv]

Writes OUTPUT_BASENAME.png (default basename: "speedtest").
"""

import argparse
import re
import sys

import matplotlib

matplotlib.use("Agg")  # headless / no display, for CI runners
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

# Operations in the order the speedtest runs them, with stable colours so the
# chart reads the same across commits.
OPERATIONS = ["Read", "Write", "Copy", "ServerCopy"]
OP_COLORS = {
    "Read": "#1f77b4",        # blue
    "Write": "#d62728",       # red
    "Copy": "#2ca02c",        # green
    "ServerCopy": "#ff7f0e",  # orange
}

SUMMARY_HEADER_RE = re.compile(
    r"#\s*File Size Performance Summary for\s+(\S+)", re.IGNORECASE
)
# Column header line that immediately precedes the data rows.
COLUMN_HEADER_PREFIX = "file_size_bytes,"
# Optional self-describing config line the speedtest may emit in CSV mode,
# e.g. "# Config: quantum_kb=1024,iterations=5,warmup=1,afp=3.4".
CONFIG_LINE_RE = re.compile(r"#\s*Config:\s*(.+)", re.IGNORECASE)
# Per-iteration data row: "Op,iteration,file_size_mb,microseconds,throughput".
ITER_ROW_RE = re.compile(r"^([A-Za-z]+),(\d+),([\d.]+),(\d+),([\d.]+)\s*$")


def _parse_config_meta(line):
    """Return the k=v pairs from a "# Config: k=v,k=v" line, else None."""
    cfg = CONFIG_LINE_RE.match(line)
    if not cfg:
        return None
    pairs = {}
    for pair in cfg.group(1).split(","):
        if "=" in pair:
            k, v = pair.split("=", 1)
            pairs[k.strip()] = v.strip()
    return pairs


def _parse_summary_row(line):
    """Parse a summary data row into a row dict, or None if it isn't one.

    Row layout: size_bytes,size_mb,mean,median,min,max,stddev,mean_time.
    """
    parts = line.split(",")
    if len(parts) < 6:
        return None
    try:
        return {
            "size_bytes": float(parts[0]),
            "mean": float(parts[2]),
            "median": float(parts[3]),
            "min": float(parts[4]),
            "max": float(parts[5]),
        }
    except ValueError:
        return None


def parse_csv(path):
    """Parse a speedtest CSV capture.

    Returns (results, meta) where:
      * results = {operation: [row_dict, ...]}, each row_dict with float keys
        size_bytes, mean, median, min, max; rows sorted by ascending size.
      * meta = dict of run metadata derived from the capture: "iterations"
        (max per-iteration index seen) plus any key=value pairs from an
        optional "# Config: k=v,k=v" line emitted by the tool.
    """
    results = {}
    meta = {}
    max_iter = 0
    current_op = None
    in_data = False

    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.strip()

            cfg = _parse_config_meta(line)
            if cfg is not None:
                meta.update(cfg)
                continue

            it = ITER_ROW_RE.match(line)
            if it:
                max_iter = max(max_iter, int(it.group(2)))

            header = SUMMARY_HEADER_RE.search(line)
            if header:
                current_op = header.group(1)
                results.setdefault(current_op, [])
                in_data = False
                continue

            if current_op is None:
                continue

            if line.startswith(COLUMN_HEADER_PREFIX):
                in_data = True
                continue

            if not in_data:
                continue

            row = _parse_summary_row(line)
            if row is None:
                in_data = False  # blank line / next section / log noise
            else:
                results[current_op].append(row)

    cleaned = {
        op: sorted(rows, key=lambda r: r["size_bytes"])
        for op, rows in results.items() if rows
    }
    if max_iter and "iterations" not in meta:
        meta["iterations"] = str(max_iter)
    return cleaned, meta


def format_size(size_bytes, _pos=None):
    """Format an exact byte count as an integer KiB/MiB axis label."""
    kib = size_bytes / 1024.0
    if kib < 1024.0:
        return f"{int(round(kib))}K"
    return f"{int(round(kib / 1024.0))}M"


def build_info_lines(results, meta):
    """Compose the run-config lines shown in the chart's info box.

    Values come from the CSV where available (size range, sweep points,
    iterations, files-per-op) and from --meta key=value pairs the caller
    injects for things the CSV cannot carry (quantum, ARC dircache, AFP
    version). Missing values are simply omitted rather than guessed.
    """
    ops = [op for op in OPERATIONS if op in results]
    ops += [op for op in results if op not in OPERATIONS]
    # Size range / point count from any populated operation.
    sizes = []
    for rows in results.values():
        sizes = [r["size_bytes"] for r in rows]
        if sizes:
            break

    lines = []
    if sizes:
        lines.append(f"Size sweep: {format_size(min(sizes))}–{format_size(max(sizes))}")
    if "iterations" in meta:
        warm = f" +{meta['warmup']} warmup" if meta.get("warmup") else ""
        lines.append(f"Iterations: {meta['iterations']}{warm} per point")
    lines.append(f"Ops: {', '.join(ops)}")
    if meta.get("afp"):
        lines.append(f"AFP: {meta['afp']}")
    if meta.get("quantum_kb"):
        lines.append(f"Quantum: {meta['quantum_kb']} KB")
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


# Background grey matching the FlameGraph SVG palette (RGB 238,238,238).
BACKGROUND_COLOR = "#EEEEEE"


def _draw_operation(ax, op, rows, baseline):
    """Plot one operation's median line, min–max band, and mean reference."""
    color = OP_COLORS.get(op, None)
    sizes = [r["size_bytes"] for r in rows]
    median = [r["median"] for r in rows]
    ax.plot(sizes, median, marker="o", markersize=4, linewidth=2,
            color=color, label=op, zorder=3)
    # min..max band gives a sense of run-to-run variance.
    ax.fill_between(sizes, [r["min"] for r in rows], [r["max"] for r in rows],
                    color=color, alpha=0.12, zorder=1)

    # Horizontal reference at this op's mean throughput, with a value label
    # just right of the Y axis so it never overlaps the Y tick labels.
    avg = sum(median) / len(median)
    ax.axhline(avg, color=color, linestyle="--", linewidth=1.0, alpha=0.6,
               zorder=2, label=f"{op} mean ({avg:.0f} MB/s)")
    ax.text(0.012, avg, f"{avg:.0f}", transform=ax.get_yaxis_transform(),
            ha="left", va="bottom", fontsize=7, color=color,
            fontweight="bold", zorder=5,
            bbox=dict(boxstyle="round,pad=0.15", facecolor="white",
                      edgecolor="none", alpha=0.7))

    if baseline and op in baseline:
        b = sorted(baseline[op], key=lambda r: r["size_bytes"])
        ax.plot([r["size_bytes"] for r in b], [r["median"] for r in b],
                linestyle="--", linewidth=1.3, color=color, alpha=0.6,
                zorder=2, label=f"{op} (baseline)")


def _configure_xaxis(ax, results):
    """Log2 X axis with one integer KiB/MiB tick per swept size, clamped."""
    ax.set_xscale("log", base=2)
    all_sizes = sorted({r["size_bytes"] for rows in results.values() for r in rows})
    if all_sizes:
        ax.set_xticks(all_sizes)
        ax.set_xticklabels([format_size(s) for s in all_sizes], fontsize=8)
        ax.minorticks_off()
        ax.set_xlim(all_sizes[0], all_sizes[-1])
    ax.xaxis.set_major_formatter(FuncFormatter(format_size))


def plot(results, out_base, title, subtitle, baseline=None, meta=None):
    """Render the throughput-vs-size chart to <out_base>.png."""
    meta = meta or {}
    fig, ax = plt.subplots(figsize=(11, 6.5))
    fig.patch.set_facecolor(BACKGROUND_COLOR)
    ax.set_facecolor(BACKGROUND_COLOR)

    # Known operations first (stable colour/order), then any unexpected ones.
    ordered = [op for op in OPERATIONS if results.get(op)]
    ordered += [op for op in results if op not in OPERATIONS and results[op]]
    for op in ordered:
        _draw_operation(ax, op, results[op], baseline)

    if not ordered:
        sys.stderr.write(
            "ERROR: no speedtest summary data found in input — "
            "was afp_speedtest run with -c (CSV)?\n"
        )
        return False

    _configure_xaxis(ax, results)
    ax.set_xlabel("File size")
    ax.set_ylabel("Throughput (MB/s)")
    ax.grid(True, which="both", linestyle=":", alpha=0.4)
    legend = ax.legend(loc="upper left", fontsize=9, framealpha=0.9)
    legend.get_frame().set_facecolor("#F5F5F5")

    suptitle = title or "Netatalk AFP Speedtest — throughput vs file size"
    fig.suptitle(suptitle, fontsize=14, fontweight="bold", y=0.96)

    # Header text below the title: the caller's subtitle plus the run-config
    # lines, rendered in the title area rather than a floating box so it never
    # overlaps the curves or the bottom-right data corner.
    header_lines = []
    if subtitle:
        header_lines.append(subtitle)
    # Pack the config items into a single "·"-separated line under the
    # subtitle (compact, won't push the plot down or overlap the curves).
    info = build_info_lines(results, meta)
    if info:
        header_lines.append(" · ".join(info))
    if header_lines:
        ax.set_title("\n".join(header_lines), fontsize=8.5, color="#222222")

    fig.tight_layout(rect=(0, 0, 1, 0.97))

    png_path = f"{out_base}.png"
    # facecolor=fig... so savefig keeps the grey background (it defaults to white).
    fig.savefig(png_path, format="png", dpi=130, facecolor=fig.get_facecolor())
    plt.close(fig)
    sys.stderr.write(f"Wrote {png_path}\n")
    return True


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="speedtest CSV capture (afp_speedtest -c)")
    parser.add_argument(
        "-o", "--output", default="speedtest",
        help="output basename (default: speedtest -> speedtest.svg/.png)",
    )
    parser.add_argument("--title", default=None, help="chart title")
    parser.add_argument("--subtitle", default=None, help="chart subtitle")
    parser.add_argument(
        "--baseline", default=None,
        help="optional prior CSV to overlay as dashed baseline lines",
    )
    parser.add_argument(
        "--meta", action="append", default=[], metavar="KEY=VALUE",
        help="run-config metadata for the info box (repeatable). Known keys: "
             "afp, quantum_kb, dircache_mode, dircache_size, "
             "dircache_validation_freq, iterations, warmup. Overrides values "
             "parsed from the CSV.",
    )
    args = parser.parse_args(argv)

    results, meta = parse_csv(args.input)
    # CLI --meta overrides anything parsed from the CSV.
    for pair in args.meta:
        if "=" in pair:
            k, v = pair.split("=", 1)
            meta[k.strip()] = v.strip()

    baseline = None
    if args.baseline:
        try:
            baseline, _ = parse_csv(args.baseline)
        except OSError as exc:
            sys.stderr.write(f"WARNING: could not read baseline: {exc}\n")

    ok = plot(results, args.output, args.title, args.subtitle, baseline, meta)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
