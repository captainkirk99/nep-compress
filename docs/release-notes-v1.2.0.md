# nep-compress v1.2.0 - JPEG-LS for Meteosat

nep-compress v1.2.0 adds support for the EUMETSAT JPEG-LS filter
(HDF5 ID 32018, registered as FCIDECOMP), the compression used for
Meteosat Third Generation FCI Level-1c imagery. It works the same way
as BZIP2: an external plugin does the compression and nep-compress
provides the C and Fortran functions to define and inquire the
filter, an opt-in build option with a configure-time plugin probe,
tests, CI coverage and documentation.

v1.2.0 also replaces the LZ4 plugin with the upstream HDF Group
source, so files written by nep-compress now use the standard 32004
encoding that every other LZ4 provider uses. Read "Upgrading" below
if you have LZ4 files written by 1.1.0 or earlier.

Finally, v1.2.0 adds a Spack package recipe and a conda-forge recipe,
each with its own CI job, and an extended abstract,
[nep-compress-abstract.md](nep-compress-abstract.md), that describes
the package and its measured compression guidance.

## Upgrading from v1.1.0

- **LZ4 files written by 1.1.0 and earlier.** They store a single
  parameter `{level}`. They decode with any 32004 plugin, but
  re-encoding such a variable with a standard 32004 provider
  (appending data, `nccopy` without `-F`, or `h5repack` keeping
  filters) takes the level as a block size of 1-9 bytes, and the
  output grows to several times the raw size. Rewrite such variables
  with nep-compress 1.2.0, or with `nccopy -F "var,32004,0,0"`.
- **LZ4 files written by 1.2.0** store the standard two-parameter
  form `{0, 1 - level}` (default block size, encoder selector).
  nep-compress 1.1.0 and earlier cannot inquire them:
  `nc_inq_var_lz4` in those versions returns `NC_EFILTER` for any
  parameter count other than one. The data itself decodes with any
  32004 plugin.
- **`nc_inq_var_lz4` accepts both parameter forms.** The one-parameter
  form returns the stored level. In the two-parameter form, encoder
  selector 0 or 1 returns level 1, and a negative selector `e` returns
  level `1 - e` when that is at most 9. Any other value (LZ4HC, or an
  acceleration above 9) returns `NC_EFILTER`, as does any parameter
  count other than one or two.
- `nc_def_var_lz4(ncid, varid, level)` keeps its signature and
  meaning (fast-mode acceleration 1-9).
- JPEG-LS is off by default and adds two exported C symbols, an
  additive change. `SOVERSION` stays 1; the library is
  `libnepcompress.so.1.2.0`.

## New JPEG-LS API

### Why JPEG-LS

From [*Earth Observation in Practice: A Mission-by-Mission Guide to Reading Satellite Data with NetCDF and Python*](https://www.amazon.com/dp/B0HJRZ9F7L), by Edward Hartnett:

> EUMETSAT disseminates FCI Level-1c imagery with stronger lossless compression based on the JPEG-LS standard through the CharLS library. JPEG-LS is implemented as a custom HDF5 filter (`H5ZJPEGLS`), not built into standard NetCDF-4 or HDF5 installations. The filename's `special_compression` field identifies these datasets; the product user guide documents which streams and dissemination paths use JPEG-LS.
>
> To read JPEG-LS-compressed FCI NetCDF-4 files you must install the `fcidecomp` decompression software from EUMETSAT. `fcidecomp` supplies the `H5ZJPEGLS` HDF5 filter plugin, which the HDF5 library loads automatically when the file is opened. Once the plugin is installed, reads are transparent: `netCDF4.Dataset()` or `h5py.File()` behave as if the data were uncompressed. Without the plugin, accessing a compressed variable fails because the library does not recognise the filter.

### Functions

- `int nc_def_var_jpegls(int ncid, int varid, int near)` defines the
  filter in define mode. The variable must be of type `NC_BYTE`,
  `NC_UBYTE`, `NC_SHORT` or `NC_USHORT` and rank 2 or 3. If it is not
  chunked yet, NetCDF-C's default chunk sizes are applied first; the
  chunks must satisfy the plugin: in rank 3 the first chunk dimension
  is the component count (1-4), the last two chunk dimensions are each
  at most 65535, and there must be at least 16 pixels per chunk.
  `near = 0` is lossless; 1-127 (1-byte types) or 1-255 (2-byte types)
  selects JPEG-LS near-lossless mode, which is lossy: every decoded
  value is within `near` of the original.
- `int nc_inq_var_jpegls(int ncid, int varid, int *jpeglsp, int
  *nearp)` searches the whole filter pipeline and does not need the
  plugin. It accepts the 9-parameter define-mode form, the
  13-parameter form the plugin stores in files (including EUMETSAT FCI
  files) and the 0-parameter form; any other count returns
  `NC_EFILTER`.
- Fortran: `nf90_def_var_jpegls(ncid, varid, near)` and
  `nf90_inq_var_jpegls(ncid, varid, has, near)`, with the usual
  1-based `varid`.
- `nc_def_var_jpegls` returns `NC_ENOFILTER` first if the plugin is
  missing, `NC_EBADTYPE` for an unsupported type, and `NC_EINVAL` for
  a bad rank, `near` or chunk shape, so data is never silently stored
  uncompressed by a call that returned `NC_NOERR`.

## LZ4 plugin

- The LZ4 plugin source is now the upstream HDF Group hdf5_plugins
  `LZ4/src/H5Zlz4.c` at commit `4d6b8ce`, vendored unchanged apart
  from added comments (see `plugins/README.md`). The previous source
  was a fork that took `cd_values[0]` as the LZ4 acceleration instead
  of the block size.
- The HDF Group notice, `plugins/COPYING.hdf5_plugins`, is installed
  as `share/doc/nepcompress/COPYING.hdf5_plugins`.
- nep-compress files now interoperate with the other 32004 providers
  (hdf5plugin, the HDF Group builds and conda-forge's
  `hdf5-external-filter-plugins-lz4`) in both directions.

## Build

- New option `NEPCOMPRESS_BUILD_JPEGLS` (default OFF) builds the C and
  Fortran JPEG-LS API and tests.
- New cache path `NEPCOMPRESS_JPEGLS_PLUGIN_DIR` points to a directory
  holding exactly one 32018 provider (EUMETSAT fcidecomp
  `libH5Zjpegls.so` or hdf5plugin `libh5fcidecomp.so`). Configuration
  fails unless a plugin there can both encode and decode.
  nep-compress does not build a JPEG-LS plugin; the external plugin
  defines the format.
- The minimum NetCDF-C is lowered from 4.10.1 to 4.10.0. With
  NetCDF-C older than 4.10.1, `NEPCOMPRESS_ZSTD_PLUGIN_DIR` is ignored
  and `perf_compare_filters` is skipped, because NetCDF-C 4.10.1 fixed
  a bug in its zstd plugin that the test's sample file triggers.
- New cache path `NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR` points to
  another 32004 provider and enables the cross-provider LZ4 tests.
- The installed-package consumer test (`test/consumer`) no longer
  requires a Fortran compiler; it builds the Fortran consumer only
  when the Fortran library is installed.

## Spack

nep-compress can be installed with Spack as the package `nep-compress`,
with variants `lz4`, `bzip2` and `fortran` (all on by default).
`spack load nep-compress` adds the LZ4 plugin directory to
`HDF5_PLUGIN_PATH`. JPEG-LS is not available through Spack.

The recipe is under review in
[spack/spack-packages#7334](https://github.com/spack/spack-packages/pull/7334);
the `1.2.0` version will be added there. Until it merges, use the
overlay repository in this repo:
`spack repo add spack/repos/spack_repo/nepcompress`, then
`spack install nep-compress`. See [spack-plan.md](spack-plan.md).

## Conda

nep-compress has a conda-forge recipe: one `nep-compress` package for
linux-64 with the C and Fortran libraries, LZ4 and BZIP2, built
against the non-MPI NetCDF-C, NetCDF-Fortran and HDF5. The LZ4 plugin
is installed in HDF5's default plugin directory as
`lib/hdf5/plugin/libh5lz4_nepcompress.so`, so no `HDF5_PLUGIN_PATH`
is needed.

The recipe lives in `conda/recipe/`, and `conda/make_staged_recipe.py`
generates the copy submitted in
[conda-forge/staged-recipes#35426](https://github.com/conda-forge/staged-recipes/pull/35426),
which will be updated to 1.2.0. Until the feedstock exists, build the
package locally from this repository; see the README "Install with
conda" section and [conda-plan.md](conda-plan.md).

## Documentation

- An extended abstract, [nep-compress-abstract.md](nep-compress-abstract.md),
  describes nep-compress 1.2.0: the LZ4, BZIP2 and JPEG-LS filters,
  the compression benchmark and its measured guidance, and how to
  build and install the package.
- `docs/compression.md` and the Doxygen main page cover JPEG-LS and
  the LZ4 encoding change, including interoperability with other LZ4
  providers.
- `plugins/README.md` records the vendored LZ4 source and its origin.

## Testing

- New `tst_jpegls` test covers lossless round trips for all four
  supported types, near-lossless mode, rank-3 variables, the
  shuffle+JPEG-LS pipeline, rejected arguments and generic filter
  definitions, and checks HDF5 storage size to catch the plugin's
  silent raw fallback.
- New `jpegls_missing_plugin` test checks `NC_ENOFILTER` on define and
  on reading data when the plugin is absent, while metadata inquiry
  still works.
- New `ftst_nep_jpegls` Fortran test covers the Fortran wrappers.
- `tst_lz4` and `compat_file` now check the two stored LZ4
  parameters, and `tst_lz4_filters` covers every inquiry parameter
  form.
- New `lz4_large_size` and `lz4_reference_large_size` tests write a
  large variable through each LZ4 provider and fail if the file
  exceeds a size bound, catching a provider that reads the level as a
  block size.
- The CI matrix gained a `jpegls` setting; three of the four jobs run
  the JPEG-LS tests with the conda-forge hdf5plugin provider.
- The LZ4 CI jobs set `NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR` to
  hdf5plugin's plugin directory, so the cross-provider tests
  (`lz4_new_to_reference`, `lz4_reference_to_new`,
  `lz4_wrong_provider`, `lz4_reference_large_size`) run in CI.
- A new `spack` CI job builds the checkout through the Spack recipe on
  an MPICH NetCDF-C 4.10.0 stack, runs the full CTest suite and a
  `spack test run` stand-alone test.
- A new `conda` CI job builds the rattler-build recipe against
  conda-forge pinnings, runs the full CTest suite inside the conda
  build, runs the package tests, and lints the generated
  staged-recipes copy.

## Requirements

CMake 3.18+, NetCDF-C 4.10.0+ (`perf_compare_filters` needs 4.10.1),
HDF5 1.14+ (C and HL), liblz4 for LZ4, and NetCDF-Fortran for the
Fortran wrappers. BZIP2 still needs an external HDF5 plugin for ID
307. JPEG-LS, when enabled, needs an external HDF5 plugin for ID
32018. The performance test needs an external zstd plugin for ID
32015.

Author: Edward Hartnett, Intelligent Data Design, Inc.
