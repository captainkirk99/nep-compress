# nep-compress v1.1.0 - Documentation and Filter Fixes

nep-compress v1.1.0 adds Doxygen documentation published to GitHub
Pages, fixes issues #3, #4 and #5, and adds a compression performance
test that compares every lossless filter available to NetCDF-4.

## Upgrading from v1.0.0

- **The bzip2 C functions now come from NetCDF-C (#3).** `libnepcompress` no
  longer defines `nc_def_var_bzip2` or `nc_inq_var_bzip2`, and
  `nep_compress.h` no longer declares them. NetCDF-C's versions (since
  4.9.0, declared in `netcdf_filter.h`, which `nep_compress.h` still
  includes) are used instead. Source code that calls them does not
  change. The Fortran `nf90_def_var_bzip2` / `nf90_inq_var_bzip2`
  wrappers are still provided and now call NetCDF-C's functions.
  `libnepcompressf` now links NetCDF-C directly.
- **A missing plugin now returns `NC_ENOFILTER`.** `nc_def_var_lz4`
  checks plugin availability (with `nc_inq_filter_avail`) before it
  checks the level, and returns `NC_ENOFILTER` instead of `NC_EFILTER`
  when the plugin is missing, as NetCDF-C does. The API no longer prints
  anything to stdout.
- **`nc_inq_var_lz4` searches the whole filter pipeline (#4).** LZ4 is
  now found in any position, not only when it is the first filter.
  The function reads the parameter count before it reads the
  parameters. If the LZ4 filter does not have exactly one parameter it
  returns `NC_EFILTER` and sets `*lz4p` to 1. `*lz4p` is set to 0 at
  the start of the call. It does not need the LZ4 plugin.
- **The shared library version is now 1.1.0.** In v1.0.0 the CMake
  project version was wrongly set to 4.2.0, so the library was
  `libnepcompress.so.4.2.0`. It is now `libnepcompress.so.1.1.0`. The
  SOVERSION is still 1.
- `libnepcompress` no longer links HDF5 directly; it links only
  NetCDF-C.

## Documentation

- Doxygen documentation for the C header, the C source, the Fortran
  module, the LZ4 plugin and the performance examples. Every function has
  an `@author` tag.
- A Doxygen main page that describes nep-compress and its filters and
  compares all NetCDF-4 lossless filters (deflate/zlib, szip, zstd,
  bzip2, LZ4, plus shuffle and Fletcher32). It includes measured results,
  performance graphs and a "Which compression should I use?" guide.
- Documentation is published at
  https://captainkirk99.github.io/nep-compress/.
- New `docs` workflow (`.github/workflows/docs.yml`) builds the
  documentation and deploys it to `gh-pages`. Run it by hand or let it
  run automatically when a release is published.
- The CI workflow has a new `docs` job that builds the documentation
  with `FAIL_ON_WARNINGS`.
- New CMake option `NEPCOMPRESS_BUILD_DOCS` (default OFF) adds a `docs`
  target. `NEPCOMPRESS_DOCS_WARN_AS_ERROR` sets Doxygen's
  `WARN_AS_ERROR`.
- The README now covers single-provider plugin directories and links to
  *The NetCDF Developer's Handbook, Second Edition* and *Earth
  Observation in Practice*, with their covers.
- `docs/compression.md` has been updated for the new API behavior and
  has a "Changes in 1.1.0" section.

## Compression performance test

- New top-level `performance/` directory with `compare_filters.c`. It
  runs under CTest as `perf_compare_filters`, with the `performance`
  label.
- It rewrites four GDAS surface fields (`tmp2m`, `prate_ave`, `soilw1`,
  `land`) from the new sample file `test/data/gdas_sfc_sample.nc`
  (about 6.4 MB), using none, deflate 1/5/9, szip, zstd 1/3/9/19,
  bzip2 1/9 and LZ4 1/9, each with shuffle off and on. Filters that are
  not available are skipped and reported.
- Write time covers create through close and read time covers open
  through close. The reported time is the minimum of 3 runs. The test
  passes when every round trip is bit-exact and the filter IDs check
  out. It does not check ratios or speeds.
- Results are written to
  `build/performance/perf_compare_filters/compare_filters.csv`. A
  reference run (`performance/reference_results.csv`,
  `performance/reference_machine.txt`) is committed and summarized on
  the main page. `performance/plot_compare_filters.py` plots a results
  CSV. `performance/make_gdas_sample.sh` rebuilds the sample file.
- New CMake option `NEPCOMPRESS_ZSTD_PLUGIN_DIR` points to a directory
  with one zstd (ID 32015) plugin. The sample file is zstd-compressed,
  so the test is configured only when this option is set. Configuration
  checks that the plugin can both encode and decode.
- CI runs the test in every job and uploads the CSV as an artifact.

## Testing and build

- **Plugin isolation (#5).** Each test's `HDF5_PLUGIN_PATH` now holds
  only its own filter's plugin directory. LZ4 tests check, on Linux,
  that the LZ4 plugin they load comes from `build/plugin`. With a
  reference LZ4 plugin configured, the new `lz4_wrong_provider` test
  checks that a plugin from the wrong provider is detected. The BZIP2
  plugin directory must hold exactly one ID 307 provider and no ID 32004
  provider.
- New `tst_lz4_filters` test: regression tests for LZ4 in multi-filter
  pipelines and for bad parameter counts.
- New `nepcompress_no_bzip2_symbols` test (Linux) checks that
  `libnepcompress` does not export the bzip2 symbols.
- New `lz4-c-asan` CI job: a C-only LZ4 build and test run under
  AddressSanitizer.
- HDF5 is found with CMake's FindHDF5 module rather than HDF5's package
  config, so HDF5 2.x installations work with the 1.14 minimum version
  and `HDF5_ROOT` is honored.
- The plugin-probe logic is now shared by BZIP2 (307) and zstd (32015).

## Requirements

Unchanged: CMake 3.18+, NetCDF-C 4.10.1+, HDF5 1.14+ (C and HL),
liblz4 for LZ4, and NetCDF-Fortran for the Fortran wrappers. BZIP2
still needs an external HDF5 plugin for ID 307. The performance test
also needs an external zstd plugin for ID 32015.

Author: Edward Hartnett, Intelligent Data Design, Inc.
