# nep-compress

nep-compress contains additional compression filters for netCDF.

NetCDF ships with support for zlib, szlib, bzip2, and zstd, added in
netcdf-c-4.9.0 (2022).

Of those choices, zstd is the clear winner for most users. It's much
faster than any of the others. But there are other, even more
attractive compression choices available with nep-compress.

## Compression Filters in nep-compress

nep-compress supports the following additional compression filters for netCDF:
* LZ4 - not as compressive as zstd, but blazing fast,
causing almost no addition delay when writing or reading data.

## Provenence of nep-compress

nep-compress contains code which was previously part of the [NetCDF
Expansion Pack
(NEP)](https://github.com/Intelligent-Data-Design-Inc/NEP) to make
compression easier to build and use.

The NEP allows users to access additional format with netCDF tools and
programs. Earth science formats like GeoTIFF and GRIB2, but also space
science formats CDF, FITS, and PDS 3/4, as well as medical images and
protien folding database formats. See the project website for more info.

nep-compress **builds an LZ4 HDF5 plugin** (`libh5lz4.so`, filter ID 32004).
For BZIP2 (ID 307), it builds the C/Fortran API but requires a compatible
**external HDF5 BZIP2 plugin** for both writing and reading. `libbz2` alone
is not a plugin. BZIP2 is OFF by default.

**Documentation:** https://captainkirk99.github.io/nep-compress/

## Dependencies

- CMake 3.18+, a C compiler, pkg-config, NetCDF-C 4.10.1+, HDF5 1.14+
  with C/HL libraries, and liblz4 when LZ4 is enabled.
- A Fortran compiler and NetCDF-Fortran when `NEPCOMPRESS_ENABLE_FORTRAN=ON`
  (the default). These must use the same NetCDF-C/HDF5 installation.
- For BZIP2, an external HDF5 plugin that implements ID 307 with an
  encoder and decoder; pass its directory to
  `NEPCOMPRESS_BZIP2_PLUGIN_DIR`. The CMake probe fails rather than
  silently skipping enabled BZIP2 coverage.

One provider is a matching HDF5 plugin package (for example a
`hdf5plugin` package installed in the *same* Conda environment as HDF5).
Its plugin directory can be queried with
`python -c 'import hdf5plugin; print(hdf5plugin.PLUGINS_PATH)'`.
Verify it supplies ID 307 using the configure-time probe; plugin
filenames alone are not proof of compatibility. Do not mix a system-HDF5
plugin with another incompatible HDF5 installation.

The BZIP2 plugin directory must contain exactly one provider of filter
ID 307, and no provider of filter ID 32004 (LZ4). On the local
development machine `/usr/local/netcdf-c/hdf5/lib/plugin` has two ID
307 providers and an ID 32004 provider, so create an isolated directory
instead:

```sh
mkdir -p "$HOME/plugins/bzip2-only"
ln -sf /usr/local/netcdf-c/hdf5/lib/plugin/lib__nch5bzip2.so "$HOME/plugins/bzip2-only/"
```

and pass `-DNEPCOMPRESS_BZIP2_PLUGIN_DIR=$HOME/plugins/bzip2-only`.

## Build and test

Choose a clean `build` directory. Set `CMAKE_PREFIX_PATH`, `PKG_CONFIG_PATH`, and
`HDF5_ROOT` for your matching HDF5/NetCDF installations as needed.

```sh
cmake -S . -B build -DNEPCOMPRESS_BUILD_LZ4=ON \
  -DNEPCOMPRESS_BUILD_BZIP2=OFF -DNEPCOMPRESS_ENABLE_FORTRAN=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/build/stage"
```

CTest sets the plugin search path for each test. For installed applications,
set it yourself to the directories containing the installed LZ4 plugin
and, if used, the external BZIP2 plugin (colon-separated on Linux):

```sh
export HDF5_PLUGIN_PATH="$HOME/nep-compress-install/lib/plugin:/path/to/bzip2/plugin"
```

Use a single provider for each filter ID in a process. The build-tree
LZ4 plugin is in `build/plugin`. Test isolation also disables unrelated
NetCDF `.ncrc` UDF autoload settings. Check `ldd` or your platform's
equivalent if a custom NetCDF or HDF5 is installed.

| CMake option | Default | Effect |
| --- | --- | --- |
| `NEPCOMPRESS_BUILD_LZ4` | ON | Build LZ4 API and plugin |
| `NEPCOMPRESS_BUILD_BZIP2` | OFF | Build the Fortran BZIP2 API and tests (uses NetCDF-C's nc_def_var_bzip2; requires an external HDF5 plugin) |
| `NEPCOMPRESS_BZIP2_PLUGIN_DIR` | empty | Directory to probe for BZIP2 when enabled |
| `NEPCOMPRESS_ENABLE_FORTRAN` | ON | Build Fortran wrappers and module |
| `NEPCOMPRESS_BUILD_BENCHMARKS` | OFF | Build slow, manual LZ4/BZIP2 examples |
| `BUILD_TESTING` | ON | Register C, HDF5 and optional Fortran tests |
| `NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR` | empty | Run optional read/write tests with an older LZ4 plugin |
| `NEPCOMPRESS_BUILD_DOCS` | OFF | Add a `docs` target that builds Doxygen HTML documentation |

## Using the libraries

```c
#include <nep_compress.h>
/* After nc_create(NC_NETCDF4), nc_def_dim and nc_def_var: */
int status = nc_def_var_lz4(ncid, varid, 3);
/* Or, with BZIP2's plugin available, via NetCDF-C's function: */
/* status = nc_def_var_bzip2(ncid, varid, 8); */
```

The bzip2 C calls (`nc_def_var_bzip2`, `nc_inq_var_bzip2`) come from
NetCDF-C, not from `libnepcompress`. Link `libnepcompress` alongside
NetCDF-C (or use the installed
`NEPCompress::nepcompress` CMake target). In Fortran, `use netcdf` and
`use nep_compress`, then call `nf90_def_var_lz4(ncid, varid, 3)` or
`nf90_def_var_bzip2(ncid, varid, 8)`. Link `libnepcompressf`; CMake
exports `NEPCompress::nepcompressf`. The module is installed under
`include/nep-compress`, so add that directory to Fortran's module path.

The [LZ4 on-disk format](plugins/LZ4_HDF5_format.md) and
[optional performance examples](examples/performance/README.md) provide
additional reference material.

## Books

For more information about netCDF and how to use it, see [The NetCDF
Developer's Handbook: The Authoritative Guide to Writing
High-Performance Programs for Scientific Data Management, Second
Edition](https://www.amazon.com/dp/B0H7Q49MPS). It is a complete
netCDF C/Fortran/Java reference, covering compression filters,
chunking and quantization in depth.

<table><tr>
<td><a href="https://www.amazon.com/dp/B0H7Q49MPS"><img src="docs/NDH_cover.jpg" width="200" alt="The NetCDF Developer's Handbook, Second Edition"></a></td>
</tr></table>

If your interests are in Earth Observation, take a look at [Earth
Observation in Practice: A Mission-by-Mission Guide to Reading
Satellite Data with NetCDF and
Python](https://www.amazon.com/dp/B0HJRZ9F7L) It is a guide to the
spacecraft, instruments and data products of 21st century remote
sensing, with GOES, NISAR, Meteosat, and all out other favorite Earth
observing platforms.

<table><tr>
<td><a href="https://www.amazon.com/dp/B0HJRZ9F7L"><img src="docs/EOiP_cover.jpg" width="200" alt="Earth Observation in Practice"></a></td>
</tr></table>



