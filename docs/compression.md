# Compression Filters

nep-compress provides lossless LZ4 and BZIP2 filter APIs for chunked
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

Both definitions return `NC_ENOFILTER` if the plugin is unavailable
(checked first) and `NC_EINVAL` for an invalid level. Both return a
NetCDF error code for other failures. `nc_inq_var_bzip2` also returns
`NC_ENOFILTER` when the plugin is missing; `nc_inq_var_lz4` does not
need the plugin at all. Apply a filter before writing data, on a
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
provider and no ID 32004 (LZ4) provider.

## Benchmarks and compatibility

The [LZ4 and BZIP2 performance examples](https://github.com/captainkirk99/nep-compress/blob/main/examples/performance/README.md)
keep their source, historical CSV results, plotters and metadata together.
Run them manually to obtain measurements for your machine; they are not
part of regular CTest. They do not guarantee specific speedups or ratios.

nep-compress retains IDs 32004 and 307 with one-parameter filter
definitions. Tests check its own write/reopen/read values and filter
metadata, plus optional reference LZ4 plugin checks. Interchange with
files written through old NEP, in either direction, is not a release
requirement; matching IDs alone do not establish such compatibility.

## Changes in 1.1.0

- `libnepcompress` no longer defines `nc_def_var_bzip2` or
  `nc_inq_var_bzip2`; NetCDF-C's versions are used.
- A missing plugin now gives `NC_ENOFILTER` from
  `nc_def_var_lz4` and `nc_def_var_bzip2`, not `NC_EFILTER`.
- `nc_inq_var_lz4` now finds LZ4 in any filter pipeline position and
  checks the parameter count before reading parameters.
- Nothing is printed to stdout by the API.

Author: Edward Hartnett, Intelligent Data Design, Inc.
