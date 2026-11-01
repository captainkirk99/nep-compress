#!/bin/sh
# Write the machine and library record for a perf_compare_filters run,
# in the format of performance/reference_machine.txt.
#
# Run it right after the measured run, with the same LD_LIBRARY_PATH,
# so ldd resolves the libraries the run used. Set NETCDF_C_SOURCE to
# the NetCDF-C source tree the installed library was built from to
# record its commit and build type.
#
# Usage: record_machine.sh BUILD_DIR OUTPUT [SOURCE_DIR]
#
# Author: Edward Hartnett, Intelligent Data Design, Inc.
set -eu

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    echo "usage: $0 BUILD_DIR OUTPUT [SOURCE_DIR]" >&2
    exit 2
fi

build=$1
out=$2
src=${3:-$(dirname "$0")/..}
cache=$build/CMakeCache.txt
exe=$build/bin/compare_filters
lz4=$build/plugin/libh5lz4.so

for f in "$cache" "$exe" "$lz4"; do
    if [ ! -f "$f" ]; then
        echo "$f not found" >&2
        exit 1
    fi
done

# Print the value of CMake cache entry $1, or nothing.
cache_value() {
    sed -n "s/^$1:[^=]*=//p" "$cache" | head -n 1
}

# Print the resolved path of the library whose soname starts with $2 in
# the ldd output of $1, or nothing.
resolved() {
    ldd "$1" 2>/dev/null | awk -v p="$2" 'index($1, p) == 1 && $3 ~ /^\// { print $3; exit }'
}

# Print the Debian package version of $1, or "unknown".
pkg_version() {
    dpkg-query -W -f='${Version}' "$1:$(dpkg --print-architecture 2>/dev/null)" 2>/dev/null || echo unknown
}

# Print "<dir>/<file> -> <realpath> sha256 <sum> links <libhdf5>" for the
# single plugin in directory $1; exit 1 unless there is exactly one.
provider() {
    n=$(ls "$1"/lib*.so* 2>/dev/null | wc -l)
    if [ "$n" -ne 1 ]; then
        echo "$1 must contain exactly one plugin, found $n" >&2
        exit 1
    fi
    f=$(ls "$1"/lib*.so*)
    real=$(readlink -f "$f")
    sum=$(sha256sum "$real" | cut -d' ' -f1)
    h5=$(ldd "$real" 2>/dev/null | awk '$1 ~ /^libhdf5\.so/ { print $1 " => " $3; exit }')
    echo "$f -> $real sha256 $sum links ${h5:-no direct libhdf5}"
}

bzip2_dir=$(cache_value NEPCOMPRESS_BZIP2_PLUGIN_DIR)
zstd_dir=$(cache_value NEPCOMPRESS_ZSTD_PLUGIN_DIR)
bzip2_line=
if [ -n "$bzip2_dir" ]; then
    bzip2_line=$(provider "$bzip2_dir")
fi
zstd_line=$(provider "$zstd_dir")

netcdf_lib=$(resolved "$exe" libnetcdf.so)
hdf5_lib=$(resolved "$exe" libhdf5.so)
hdf5_version=
if [ -n "$hdf5_lib" ]; then
    hdf5_version=$(sed -n 's/^#define H5_VERSION "\(.*\)"/\1/p' \
        "$(dirname "$hdf5_lib")/../include/H5pubconf.h" 2>/dev/null)
fi
netcdf_extra=
if [ -n "${NETCDF_C_SOURCE:-}" ]; then
    commit=$(git -C "$NETCDF_C_SOURCE" rev-parse --short HEAD 2>/dev/null || true)
    netcdf_extra=" ${commit:-unknown}"
fi
cflags=
if [ -n "$netcdf_lib" ]; then
    cflags=$(sed -n 's/^CFLAGS:[[:space:]]*//p' "$(dirname "$netcdf_lib")/libnetcdf.settings" 2>/dev/null)
fi
build_type=
if [ -n "${NETCDF_C_SOURCE:-}" ] && [ -f "$NETCDF_C_SOURCE/build/CMakeCache.txt" ]; then
    build_type=$(sed -n 's/^CMAKE_BUILD_TYPE:[^=]*=//p' "$NETCDF_C_SOURCE/build/CMakeCache.txt" | head -n 1)
fi
cc=$(cache_value CMAKE_C_COMPILER)
nreps=$(sed -n 's/^#define NREPS \([0-9]*\).*/\1/p' "$src/performance/compare_filters.c")
plot=$(python3 -c 'import sys, matplotlib; print("Python %s, matplotlib %s" % (sys.version.split()[0], matplotlib.__version__))' 2>/dev/null || echo unknown)

{
    echo "Run date: $(date -u '+%Y-%m-%d %H:%M UTC')"
    echo "nep-compress: $(cache_value CMAKE_PROJECT_VERSION) ($(git -C "$src" describe --tags --always --dirty 2>/dev/null || echo unknown))"
    echo "CPU: $(lscpu | sed -n 's/^Model name:[[:space:]]*//p')"
    echo "Cores: $(nproc)"
    echo "OS: $(uname -sr)"
    echo "Compiler: $("$cc" --version 2>/dev/null | head -n 1)"
    echo "NetCDF-C: $(cache_value NETCDF_VERSION)$netcdf_extra (${netcdf_lib:-unknown}), CFLAGS ${cflags:-unknown}${build_type:+, build type $build_type}"
    echo "HDF5: ${hdf5_version:-unknown} (${hdf5_lib:-unknown})"
    echo "lz4: $(pkg_version liblz4-1)"
    echo "zstd: $(pkg_version libzstd1)"
    echo "bzip2: $(pkg_version libbz2-1.0)"
    echo "libaec: $(pkg_version libaec0)"
    echo "zlib: $(pkg_version zlib1g)"
    echo "LZ4 provider: $lz4 sha256 $(sha256sum "$lz4" | cut -d' ' -f1) (nep-compress, vendored HDF Group plugin)"
    if [ -n "$bzip2_line" ]; then
        echo "BZIP2 provider: $bzip2_line"
    fi
    echo "zstd provider: $zstd_line"
    echo "NREPS: ${nreps:-unknown}"
    echo "Sample: test/data/gdas_sfc_sample.nc"
    echo "Plotting: $plot"
} > "$out"
