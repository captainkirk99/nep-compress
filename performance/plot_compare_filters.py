"""Generate filter-comparison plots from compare_filters CSV output.

Usage:
    python3 plot_compare_filters.py [input.csv] [output_prefix]

Reads:  input.csv (default reference_results.csv)
Writes: <output_prefix>_write.png, <output_prefix>_read.png,
        <output_prefix>_write_simple.png and
        <output_prefix>_read_simple.png
        (output_prefix defaults to compare_filters)

Columns expected: field,filter,filter_id,level,shuffle,uncompressed_bytes,
file_bytes,ratio,write_s,read_s,write_MBps,read_MBps

Author: Edward Hartnett, Intelligent Data Design, Inc.
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.ticker import ScalarFormatter, NullFormatter, LogLocator

INPUT = sys.argv[1] if len(sys.argv) > 1 else "reference_results.csv"
PREFIX = sys.argv[2] if len(sys.argv) > 2 else "compare_filters"

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
try:
    with open(INPUT, newline="") as f:
        for row in csv.DictReader(f):
            rows.append({
                "field":   row["field"],
                "filter":  row["filter"],
                "level":   int(row["level"]),
                "shuffle": int(row["shuffle"]),
                "ratio":   float(row["ratio"]),
                "write_MBps": float(row["write_MBps"]),
                "read_MBps":  float(row["read_MBps"]),
            })
except FileNotFoundError:
    print(f"Error: {INPUT} not found. Run the perf_compare_filters test first.",
          file=sys.stderr)
    sys.exit(1)

# Fields and filters in the order they appear in the CSV.
fields = list(dict.fromkeys(r["field"] for r in rows))
filters = list(dict.fromkeys(r["filter"] for r in rows))

# Colorblind-safe tab10 colors and one marker shape per filter.
COLORS = {"none": "grey", "deflate": "tab:blue", "szip": "tab:green",
          "zstd": "tab:orange", "bzip2": "tab:purple", "lz4": "tab:red"}
MARKERS = ["o", "s", "^", "D", "v", "P", "X", "<", ">", "h"]
color_of = {name: COLORS.get(name, f"C{i}") for i, name in enumerate(filters)}
marker_of = {name: MARKERS[i % len(MARKERS)]
             for i, name in enumerate(filters)}

# Plain number tick labels on log axes, no scientific notation or minors.
def plain_axes(ax, logx=True, logy=False):
    """Set log/linear scales with plain tick labels and no minors."""
    if logx:
        ax.set_xscale("log")
        ax.xaxis.set_major_locator(LogLocator(base=10, subs=(1.0, 2.0, 5.0)))
    if logy:
        ax.set_yscale("log")
        ax.yaxis.set_major_locator(LogLocator(base=10, subs=(1.0, 2.0, 5.0)))
    fmt = ScalarFormatter()
    fmt.set_scientific(False)
    for axis in (ax.xaxis, ax.yaxis):
        axis.set_major_formatter(fmt)
        axis.set_minor_formatter(NullFormatter())

# Alternate annotation offsets to reduce label overlap along a curve.
OFFSETS = [(4, 4), (4, -10)]

def finish_panel(ax, field, pts, key):
    """Scales, title, labels and grid shared by all panels."""
    fratios = [r["ratio"] for r in pts]
    plain_axes(ax, logx=True,
               logy=max(fratios) / min(fratios) > 10)
    ax.set_title(field, fontsize=11)
    ax.set_xlabel(f"{key.split('_')[0]} throughput (MB/s)", fontsize=9)
    ax.set_ylabel("compression ratio", fontsize=9)
    ax.grid(True, which="major", linestyle="-", color="grey", alpha=0.5)
    ax.set_axisbelow(True)

# ---------------------------------------------------------------------------
# Full plots: every filter level x shuffle as curves
# ---------------------------------------------------------------------------
def make_plot(key, title, out):
    """Scatter ratio vs throughput column for every field."""
    fig, axes = plt.subplots(2, 2, figsize=(13, 10))
    for ax, field in zip(axes.flat, fields):
        for name in filters:
            for shuffle in (0, 1):
                pts = sorted(
                    (r for r in rows
                     if r["field"] == field and r["filter"] == name
                     and r["shuffle"] == shuffle),
                    key=lambda r: r["level"])
                if not pts:
                    continue
                color = color_of[name]
                x = [r[key] for r in pts]
                y = [r["ratio"] for r in pts]
                # Connect the level sweep with a thin curve when the
                # series has more than one level.
                if len(pts) > 1:
                    ax.plot(x, y, color=color, linewidth=0.8,
                            linestyle="-" if shuffle else "--", zorder=2)
                ax.scatter(x, y, marker=marker_of[name],
                           facecolors=color if shuffle else "white",
                           edgecolors=color, linewidths=1.5,
                           s=70, zorder=3)
                # Annotate shuffle-on points with their level.
                if shuffle and name != "none":
                    for i, r in enumerate(pts):
                        ax.annotate(str(r["level"]),
                                    (r[key], r["ratio"]),
                                    textcoords="offset points",
                                    xytext=OFFSETS[i % 2],
                                    fontsize=8, color=color)
        finish_panel(ax, field,
                     [r for r in rows if r["field"] == field], key)
    fig.suptitle(title, fontsize=13)
    # Shared legend: one color+marker entry per filter, plus the
    # filled/open shuffle key.
    handles = [Line2D([], [], marker=marker_of[name], color="none",
                      markerfacecolor=color_of[name],
                      markeredgecolor=color_of[name],
                      markersize=9, label=name)
               for name in filters]
    handles += [
        Line2D([], [], marker="o", color="none", markerfacecolor="black",
               markeredgecolor="black", markersize=9, label="shuffle on"),
        Line2D([], [], marker="o", color="none", markerfacecolor="white",
               markeredgecolor="black", markersize=9, label="shuffle off"),
    ]
    fig.legend(handles=handles, loc="lower center",
               ncol=min(len(handles), 4), fontsize=9,
               frameon=True, edgecolor="black")
    plt.tight_layout(rect=[0, 0.06, 1, 0.97])
    plt.savefig(out, dpi=150, format="png")
    plt.close(fig)
    print(f"Saved {out}")

# ---------------------------------------------------------------------------
# Simple plots: shuffle on only, fastest level per filter
# ---------------------------------------------------------------------------
# Pick, per filter, the level with the highest mean write_MBps across
# all fields (shuffle on). Used for both simple plots.
best_level = {}
for name in filters:
    levels = sorted(set(r["level"] for r in rows
                        if r["filter"] == name and r["shuffle"] == 1))
    best = max(levels,
               key=lambda lv: sum(r["write_MBps"] for r in rows
                                  if r["filter"] == name and r["level"] == lv
                                  and r["shuffle"] == 1) /
               max(1, sum(1 for r in rows
                          if r["filter"] == name and r["level"] == lv
                          and r["shuffle"] == 1)))
    best_level[name] = best
    print(f"{name}: fastest write level {best}")

def make_simple_plot(key, title, out):
    """Scatter ratio vs throughput for the fastest level per filter."""
    fig, axes = plt.subplots(2, 2, figsize=(13, 10))
    for ax, field in zip(axes.flat, fields):
        plotted = []
        for name in filters:
            pts = [r for r in rows
                   if r["field"] == field and r["filter"] == name
                   and r["shuffle"] == 1 and r["level"] == best_level[name]]
            if not pts:
                continue
            plotted += pts
            r = pts[0]
            color = color_of[name]
            ax.scatter([r[key]], [r["ratio"]], marker=marker_of[name],
                       facecolors=color, edgecolors=color,
                       linewidths=1.5, s=70, zorder=3)
            if name != "none":
                ax.annotate(str(r["level"]), (r[key], r["ratio"]),
                            textcoords="offset points", xytext=(4, 4),
                            fontsize=8, color=color)
        finish_panel(ax, field, plotted, key)
    fig.suptitle(title + " (fastest level per filter, shuffle on)",
                 fontsize=13)
    handles = [Line2D([], [], marker=marker_of[name], color="none",
                      markerfacecolor=color_of[name],
                      markeredgecolor=color_of[name], markersize=9,
                      label=name if name == "none"
                      else f"{name} {best_level[name]}")
               for name in filters]
    fig.legend(handles=handles, loc="lower center",
               ncol=min(len(handles), 4), fontsize=9,
               frameon=True, edgecolor="black")
    plt.tight_layout(rect=[0, 0.06, 1, 0.97])
    plt.savefig(out, dpi=150, format="png")
    plt.close(fig)
    print(f"Saved {out}")

TITLE = "NetCDF-4 filter comparison on GDAS surface data: ratio vs"
make_plot("write_MBps", f"{TITLE} write throughput", f"{PREFIX}_write.png")
make_plot("read_MBps", f"{TITLE} read throughput", f"{PREFIX}_read.png")
make_simple_plot("write_MBps", f"{TITLE} write throughput",
                 f"{PREFIX}_write_simple.png")
make_simple_plot("read_MBps", f"{TITLE} read throughput",
                 f"{PREFIX}_read_simple.png")
