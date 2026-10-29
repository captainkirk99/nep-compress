# Compression Filters

nep-compress provides LZ4, BZIP2 and JPEG-LS filter APIs for chunked
NetCDF-4/HDF5 variables. HDF5 stores the filter ID and parameters in
each variable's filter pipeline; the corresponding plugin must be
available to decode existing compressed data, even if the application
never calls the nep-compress API. See the [README](https://github.com/captainkirk99/nep-compress/blob/main/README.md) for
installation, dependencies and a BZIP2 plugin provider.

## LZ4: prioritize speed

- Filter ID: **32004**. `libh5lz4.so` is built and installed by this
  project under `lib/plugin` (or the platform's chosen `CMAKE_INSTALL_LIBDIR`).
- `nc_def_var_lz4(ncid, varid, level)` enables the filter; valid
  levels 1-9 are passed as a single filter parameter. The parameter
  controls acceleration in the copied LZ4 implementation.
- `nc_inq_var_lz4(ncid, varid, &present, &level)` queries settings.
  Either output pointer may be NULL. It searches the whole filter
  pipeline, so LZ4 is found whatever its position among the
  variable's filters. `present` is set to 1 when LZ4 is on the
  variable; `level` is written only when LZ4 is present with exactly
  one parameter. It returns `NC_EFILTER` (with `present` set to 1) if
  the LZ4 filter has a parameter count other than 1. It does not need
  the LZ4 plugin, so metadata stays readable without it.
- LZ4's [chunk encoding](../plugins/LZ4_HDF5_format.md) differs from
  LZ4 Frame format; preserve the plugin and ID when exchanging files.

## BZIP2: prioritize size

- Filter ID: **307**. The C functions `nc_def_var_bzip2` and
  `nc_inq_var_bzip2` are NetCDF-C's own (available since NetCDF-C
  4.9.0, declared in `netcdf_filter.h`); nep-compress adds the Fortran
  `nf90_def_var_bzip2` / `nf90_inq_var_bzip2` wrappers, the
  `NEPCOMPRESS_BUILD_BZIP2` build option and the BZIP2 tests. It does
  **not** install an HDF5 BZIP2 plugin. Obtain an external plugin
  built for your HDF5 and supply its directory with
  `NEPCOMPRESS_BZIP2_PLUGIN_DIR` when configuring BZIP2 support.
  Configuring fails when ID 307 is missing or lacks an encoder or
  decoder; installing `libbz2` alone is insufficient.
- `nc_def_var_bzip2(ncid, varid, level)` uses levels 1-9, encoded as
  one filter parameter (block sizes of 100-900 KB).
- `nc_inq_var_bzip2(ncid, varid, &present, &level)` searches the filter
  IDs; either output pointer may be NULL.

## JPEG-LS: satellite imagery (EUMETSAT FCI)

From [*Earth Observation in Practice: A Mission-by-Mission Guide to Reading Satellite Data with NetCDF and Python*](https://www.amazon.com/dp/B0HJRZ9F7L), by Edward Hartnett:

> EUMETSAT disseminates FCI Level-1c imagery with stronger lossless compression based on the JPEG-LS standard through the CharLS library. JPEG-LS is implemented as a custom HDF5 filter (`H5ZJPEGLS`), not built into standard NetCDF-4 or HDF5 installations. The filename's `special_compression` field identifies these datasets; the product user guide documents which streams and dissemination paths use JPEG-LS.
>
> To read JPEG-LS-compressed FCI NetCDF-4 files you must install the `fcidecomp` decompression software from EUMETSAT. `fcidecomp` supplies the `H5ZJPEGLS` HDF5 filter plugin, which the HDF5 library loads automatically when the file is opened. Once the plugin is installed, reads are transparent: `netCDF4.Dataset()` or `h5py.File()` behave as if the data were uncompressed. Without the plugin, accessing a compressed variable fails because the library does not recognise the filter.

- Filter ID: **32018** (registered as FCIDECOMP).
- nep-compress does **not** build a JPEG-LS plugin. Get one from
  EUMETSAT fcidecomp (`libH5Zjpegls.so`; conda `-c eumetsat fcidecomp`
  or a source build) or from hdf5plugin (`libh5fcidecomp.so`), then
  configure with `NEPCOMPRESS_BUILD_JPEGLS=ON` and
  `NEPCOMPRESS_JPEGLS_PLUGIN_DIR` pointing at a directory that holds
  exactly one 32018 provider.
- `nc_def_var_jpegls(ncid, varid, near)` enables the filter. The
  variable must be of type `NC_BYTE`, `NC_UBYTE`, `NC_SHORT` or
  `NC_USHORT` and rank 2 or 3. If it is not chunked yet, NetCDF-C's
  default chunk sizes are applied first; the chunks must then satisfy
  the plugin: in rank 3 the first chunk dimension is the component
  count (1-4), the last two chunk dimensions are each at most 65535,
  and there must be at least 16 pixels per chunk. `near = 0` is
  lossless; `near > 0` (1-127 for 1-byte types, 1-255 for 2-byte
  types) is JPEG-LS near-lossless mode, which is **lossy**: every
  decoded value is within `near` of the original.
- `nc_inq_var_jpegls(ncid, varid, &present, &near)` queries settings.
  It searches the whole pipeline and does not need the plugin. It
  handles the 13-parameter form the plugin stores in files, including
  EUMETSAT FCI files, as well as the 9-parameter define-mode form.
- The plugin falls back to raw storage, with only stderr messages,
  when a chunk does not compress or cannot be handled. Unsupported
  types and chunk shapes are rejected by `nc_def_var_jpegls` for this
  reason.
- Reading an FCI file needs no nep-compress call, only the plugin on
  `HDF5_PLUGIN_PATH`. Inspect the stored filter with
  `ncdump -hs file.nc | grep _Filter` and decompress a copy with
  `nccopy -F none in.nc out.nc`.
- FCI files store variables as 2-D `ushort`/`ubyte` with chunks such
  as 601 x 22272.

Both definitions return `NC_ENOFILTER` if the plugin is unavailable
(checked first) and `NC_EINVAL` for an invalid level. Both return a
NetCDF error code for other failures. `nc_inq_var_bzip2` also returns
`NC_ENOFILTER` when the plugin is missing; `nc_inq_var_lz4` does not
need the plugin at all. `nc_def_var_jpegls` checks availability first
(`NC_ENOFILTER`), then returns `NC_EBADTYPE` for an unsupported type
or `NC_EINVAL` for a bad rank, `near` or chunk shape;
`nc_inq_var_jpegls`, like `nc_inq_var_lz4`, does not need the plugin. Apply a filter before writing data, on a
NetCDF-4 variable; use `nc_def_var_chunking` to control chunk shape.
The C API is declared by `<nep_compress.h>` and supplied by
`libnepcompress`:

```c
#include <nep_compress.h>
/* After defining a variable in an NC_NETCDF4 file: */
int status = nc_def_var_lz4(ncid, varid, 3);
int has_filter = 0, level = 0;
if (status == NC_NOERR)
    status = nc_inq_var_lz4(ncid, varid, &has_filter, &level);
```

Fortran wrappers use Fortran's 1-based `varid` and return NetCDF error
codes. They are exported by `module nep_compress` / `libnepcompressf`,
not by NEP's former `module nep`:

```fortran
use netcdf
use nep_compress
! After defining the variable in a NetCDF-4 file:
status = nf90_def_var_lz4(ncid, varid, 3)
status = nf90_inq_var_lz4(ncid, varid, present, level)
```

To use BZIP2 instead, enable its build option and substitute
`nc_def_var_bzip2` / `nc_inq_var_bzip2` or
`nf90_def_var_bzip2` / `nf90_inq_var_bzip2`. HDF5 discovers the filters
at runtime from `HDF5_PLUGIN_PATH`: include the installed LZ4 directory
and the external BZIP2 directory (on Linux, separated by `:`). Both
write *and* read tests run with the plugins; an existing compressed
file is not automatically readable without a decoder.

Keep exactly one provider per filter ID on `HDF5_PLUGIN_PATH`. When
two plugins register the same ID, which one HDF5 loads is not
defined. In particular, the BZIP2 directory must hold one ID 307
provider and no ID 32004 (LZ4) provider, and the JPEG-LS directory
must hold exactly one ID 32018 provider.

## Benchmarks and compatibility

The [LZ4 and BZIP2 performance examples](https://github.com/captainkirk99/nep-compress/blob/main/examples/performance/README.md)
keep their source, historical CSV results, plotters and metadata together.
Run them manually to obtain measurements for your machine; they are not
part of regular CTest. They do not guarantee specific speedups or ratios.

The `perf_compare_filters` test in `performance/` does run under CTest
when `NEPCOMPRESS_ZSTD_PLUGIN_DIR` is set and NetCDF-C is 4.10.1 or
later (4.10.1 fixed an input-length bug in NetCDF-C's zstd plugin). It
rewrites four real GDAS
surface fields with every available lossless filter, checks each round
trip bit-exactly and reports size, ratio and write/read throughput as
CSV and a table. Reference numbers from the development machine are in
[reference_results.csv](https://github.com/captainkirk99/nep-compress/blob/main/performance/reference_results.csv)
and
[reference_machine.txt](https://github.com/captainkirk99/nep-compress/blob/main/performance/reference_machine.txt).

nep-compress retains IDs 32004 and 307 with one-parameter filter
definitions. Tests check its own write/reopen/read values and filter
metadata, plus optional reference LZ4 plugin checks. Interchange with
files written through old NEP, in either direction, is not a release
requirement; matching IDs alone do not establish such compatibility.
For JPEG-LS, nep-compress writes the same 32018 parameter layout as
EUMETSAT's fcidecomp, because the plugin doing the work is
fcidecomp's own.

## Changes in 1.2.0

- New JPEG-LS API: `nc_def_var_jpegls` and `nc_inq_var_jpegls` in C,
  `nf90_def_var_jpegls` and `nf90_inq_var_jpegls` in Fortran, built
  with the new `NEPCOMPRESS_BUILD_JPEGLS` option and
  `NEPCOMPRESS_JPEGLS_PLUGIN_DIR`, plus tests. No behavior change for
  LZ4 or BZIP2.
- The minimum NetCDF-C is now 4.10.0. `perf_compare_filters` is
  skipped on NetCDF-C older than 4.10.1.
- A Spack package `nep-compress` (see the README).
- A conda-forge recipe `nep-compress` (see the README); its LZ4 plugin
  is installed in conda HDF5's default plugin directory as
  `lib/hdf5/plugin/libh5lz4_nepcompress.so`.
- An extended abstract (`docs/nep-compress-abstract.md`).

## Changes in 1.1.0

- `libnepcompress` no longer defines `nc_def_var_bzip2` or
  `nc_inq_var_bzip2`; NetCDF-C's versions are used.
- A missing plugin now gives `NC_ENOFILTER` from
  `nc_def_var_lz4` and `nc_def_var_bzip2`, not `NC_EFILTER`.
- `nc_inq_var_lz4` now finds LZ4 in any filter pipeline position and
  checks the parameter count before reading parameters.
- Nothing is printed to stdout by the API.

Author: Edward Hartnett, Intelligent Data Design, Inc.
