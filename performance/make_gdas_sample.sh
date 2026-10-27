#!/usr/bin/env bash
# Create the compressed GDAS surface sample used by the
# perf_compare_filters test.
#
# The source is NOAA GFS/GDAS gdas.t00z.sfcf003.nc, an FV3GFS surface
# forecast file at 3-hour lead. Four full-grid fields (tmp2m,
# prate_ave, soilw1, land) plus the coordinate variables are copied
# into a small NetCDF-4 file stored with shuffle + zstd level 19.
#
# Requires nccopy and ncdump on PATH, and HDF5_PLUGIN_PATH set to a
# directory containing exactly one filter 32015 (zstd) provider.
#
# Usage: make_gdas_sample.sh <source.nc> <output.nc>
#
# Author: Edward Hartnett, Intelligent Data Design, Inc.
set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <source.nc> <output.nc>" >&2
    exit 2
fi

src="$1"
out="$2"

if [ -z "${HDF5_PLUGIN_PATH:-}" ]; then
    echo "HDF5_PLUGIN_PATH must name a directory with one filter 32015 (zstd) plugin" >&2
    exit 1
fi

tmp="$(mktemp --suffix=.nc)"
trap 'rm -f "$tmp"' EXIT

# Step 1: uncompressed copy of the wanted variables only.
nccopy -k nc4 -d 0 -V tmp2m,prate_ave,soilw1,land,grid_xt,grid_yt,time "$src" "$tmp"

# Step 2: apply shuffle (filter 2) then zstd level 19 to the data fields.
# nccopy applies no filter when -F is combined with -d, so this must be
# a second copy.
nccopy -F "tmp2m&prate_ave&soilw1&land,2|32015,19" "$tmp" "$out"

# Step 3: confirm the filters were applied (nccopy can silently apply
# no filter).
header="$(ncdump -hs "$out")"
grep -q 'tmp2m:_Filter = "32015,19"' <<<"$header" || {
    echo "tmp2m lacks _Filter 32015,19" >&2; exit 1; }
grep -q 'tmp2m:_Shuffle = "true"' <<<"$header" || {
    echo "tmp2m lacks _Shuffle" >&2; exit 1; }

# Step 4: sanity-check the size.
size=$(stat -c %s "$out")
if [ "$size" -ge 10000000 ]; then
    echo "$out is $size bytes; expected under 10000000" >&2
    exit 1
fi
