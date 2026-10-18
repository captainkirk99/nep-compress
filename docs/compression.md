# Compression Filters

nep-compress provides lossless LZ4 and BZIP2 filter APIs for chunked
NetCDF-4/HDF5 variables. HDF5 stores the filter ID and parameters in
each variable's filter pipeline; the corresponding plugin must be
available to decode existing compressed data, even if the application
never calls the nep-compress API. See the [README](../README.md) for
installation, dependencies and a BZIP2 plugin provider.

## LZ4: prioritize speed

- Filter ID: **32004**. `libh5lz4.so` is built and installed by this
  project under `lib/plugin` (or the platform's chosen `CMAKE_INSTALL_LIBDIR`).
- `nc_def_var_lz4(ncid, varid, level)` enables the filter; valid
  levels 1-9 are passed as a single filter parameter. The parameter
  controls acceleration in the copied LZ4 implementation.
- `nc_inq_var_lz4(ncid, varid, &present, &level)` queries settings.
  Either output pointer may be NULL. On files using multiple filters,
  the current wrapper retains NEP's legacy single-filter inquiry
  behavior; use NetCDF-C's `nc_inq_var_filter_ids` and
  `nc_inq_var_filter_info` to inspect the full pipeline.
- LZ4's [chunk encoding](../plugins/LZ4_HDF5_format.md) differs from
  LZ4 Frame format; preserve the plugin and ID when exchanging files.

## BZIP2: prioritize size

- Filter ID: **307**. The C and Fortran wrappers do **not** install
  an HDF5 BZIP2 plugin. Obtain an external plugin built for your HDF5
  and supply its directory with `NEPCOMPRESS_BZIP2_PLUGIN_DIR` when
  configuring BZIP2 support. Configuring fails when ID 307 is missing
  or lacks an encoder or decoder; installing `libbz2` alone is insufficient.
- `nc_def_var_bzip2(ncid, varid, level)` uses levels 1-9, encoded as
  one filter parameter (block sizes of 100-900 KB).
- `nc_inq_var_bzip2(ncid, varid, &present, &level)` searches the filter
  IDs; either output pointer may be NULL.

Both definitions return `NC_EINVAL` for an invalid level and
`NC_EFILTER` if the plugin is unavailable. Both return a NetCDF error
code for other failures. Apply a filter before writing data, on a
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

## Benchmarks and compatibility

The [LZ4 and BZIP2 performance examples](../examples/performance/README.md)
keep their source, historical CSV results, plotters and metadata together.
Run them manually to obtain measurements for your machine; they are not
part of regular CTest. They do not guarantee specific speedups or ratios.

Files using IDs 32004 and 307 with the existing one-parameter layout
are the compatibility target. Tests check values and filter metadata
across new and installed reference LZ4 plugins. The original NEP
writer/reopen comparison remains unverified because the existing NEP
build must be safely replaced before its baseline can run; do not
infer full old-NEP compatibility from a matching filter ID alone.
