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
sample, so it is only configured when `NEPCOMPRESS_ZSTD_PLUGIN_DIR`
names a directory containing exactly one 32015 provider:

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
record a run on the development machine.

`make_gdas_sample.sh` regenerates the sample; see
`test/data/README.md`.

Author: Edward Hartnett, Intelligent Data Design, Inc.
