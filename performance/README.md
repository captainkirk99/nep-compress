# Compression filter performance test

`perf_compare_filters` runs `compare_filters`, a program that rewrites
four real GDAS surface fields (`tmp2m`, `prate_ave`, `soilw1`, `land`,
from `test/data/gdas_sfc_sample.nc`) with every lossless filter
available to NetCDF-4: none, deflate 1/5/9, szip (NN, 32 pixels per
block), zstandard 1/3/9/19, bzip2 1/9 and LZ4 1/9 — each with the HDF5
shuffle filter off and on. Filters whose plugin is unavailable are
skipped and reported, not failed.

Write time covers `nc_create` through `nc_close` (defines, `nc_enddef`,
`nc_put_var_float`); read time covers `nc_open` through `nc_close`
(`nc_inq_varid`, `nc_get_var_float`). Each configuration runs three
times and the minimum is reported. Every round trip is checked
bit-exactly and the variable's filter IDs are verified after reopening.

The test needs a zstd (filter 32015) HDF5 plugin to read its input
sample, and NetCDF-C 4.10.1 or later (4.10.1 fixed an input-length bug
in NetCDF-C's zstd plugin), so it is only configured when
`NEPCOMPRESS_ZSTD_PLUGIN_DIR` names a directory containing exactly one
32015 provider; on older NetCDF-C it is skipped with a status message:

```sh
cmake -DNEPCOMPRESS_ZSTD_PLUGIN_DIR=$HOME/plugins/zstd-only ...
```

Run it alone with:

```sh
ctest -R perf_compare_filters --output-on-failure
ctest -L performance              # all performance-labelled tests
ctest -LE performance             # everything except them
```

The CSV lands in
`build/performance/perf_compare_filters/compare_filters.csv`.
`reference_results.csv` and `reference_machine.txt` in this directory
record the 2026-11-01 run of nep-compress 1.2.0 on the development
machine.

The BZIP2 and zstd plugin directories should hold plugins linked to
the same HDF5 library as NetCDF-C; check with
`ldd <dir>/lib*.so | grep libhdf5`.

## Recording the machine

`record_machine.sh` writes the machine and library record in the
format of `reference_machine.txt`. It records the date, the
nep-compress version and commit, the CPU, OS and compiler, the
NetCDF-C, HDF5, lz4, zstd, bzip2, libaec and zlib versions, and the
path, sha256 and HDF5 linkage of each plugin. Run it right after the
measured run, with the same `LD_LIBRARY_PATH`:

```sh
NETCDF_C_SOURCE=/path/to/netcdf-c \
  performance/record_machine.sh build performance/reference_machine.txt
```

Usage: `record_machine.sh BUILD_DIR OUTPUT [SOURCE_DIR]`. `SOURCE_DIR`
defaults to this repository. `NETCDF_C_SOURCE` is optional; when set,
the NetCDF-C commit and build type are recorded too. The script exits
1 if the build has no `compare_filters` or LZ4 plugin, or if a plugin
directory does not hold exactly one plugin.

## Plotting

`plot_compare_filters.py` graphs a results CSV (needs matplotlib):

```sh
python3 performance/plot_compare_filters.py \
  build/performance/perf_compare_filters/compare_filters.csv \
  /tmp/compare_filters
```

The committed graphs were made with
`python3 performance/plot_compare_filters.py performance/reference_results.csv docs/images/compare_filters`.

It writes `<prefix>_write.png`, `<prefix>_read.png`,
`<prefix>_write_simple.png` and `<prefix>_read_simple.png`: 2x2 panels,
one per field, scatter of ratio vs write/read throughput; one marker
shape per filter, filled = shuffle on, open = shuffle off. The
`*_simple` plots show only shuffle-on points at the fastest write level
per filter.

`make_gdas_sample.sh` regenerates the sample; see
`test/data/README.md`.

Author: Edward Hartnett, Intelligent Data Design, Inc.
