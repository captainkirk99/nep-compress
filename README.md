# nep-compress

LZ4 and BZIP2 compression for NetCDF-4/HDF5, with C and optional Fortran
wrappers. This code is being separated from
[NEP](https://github.com/Intelligent-Data-Design-Inc/NEP) to make compression
easier to build and use without installing its format readers. NEP remains
the project for UDF formats and NEXTCDF-4; it still ships its original
compression APIs during the migration. See the [compression guide](docs/compression.md)
for API details and file compatibility.

nep-compress **builds an LZ4 HDF5 plugin** (`libh5lz4.so`, filter ID 32004).
For BZIP2 (ID 307), it builds the C/Fortran API but requires a compatible
**external HDF5 BZIP2 plugin** for both writing and reading. `libbz2` alone
is not a plugin. BZIP2 is OFF by default.

**Documentation:** https://captainkirk99.github.io/nep-compress/

## Books

<table><tr>
<td><a href="https://www.amazon.com/dp/B0H7Q49MPS"><img src="docs/NDH_cover.jpg" width="200" alt="The NetCDF Developer's Handbook, Second Edition"></a></td>
<td><a href="https://www.amazon.com/dp/B0HJRZ9F7L"><img src="docs/EOiP_cover.jpg" width="200" alt="Earth Observation in Practice"></a></td>
</tr></table>

[The NetCDF Developer's Handbook: The Authoritative Guide to Writing
High-Performance Programs for Scientific Data Management, Second
Edition](https://www.amazon.com/dp/B0H7Q49MPS) is the complete
NetCDF-C and NetCDF-Fortran reference, covering compression filters,
chunking and quantization in depth.

[Earth Observation in Practice: A Mission-by-Mission Guide to Reading
Satellite Data with NetCDF and
Python](https://www.amazon.com/dp/B0HJRZ9F7L) is a mission-by-mission
guide to the spacecraft, instruments and data products of modern remote
sensing.

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
filenames alone are not proof of compatibility. On the local development
machine, the external plugin directory is
`/usr/local/netcdf-c/hdf5/lib/plugin`. Do not mix a system-HDF5 plugin
with another incompatible HDF5 installation.

## Build and test

Choose a clean `build` directory (do not overwrite an existing build without
checking its contents). Set `CMAKE_PREFIX_PATH`, `PKG_CONFIG_PATH`, and
`HDF5_ROOT` for your matching HDF5/NetCDF installations as needed. HDF5 is
located through CMake's module finder so HDF5 2.x installations satisfy the
1.14 minimum version, even if an older `HDF5_DIR` remains in the build cache.
When switching HDF5 installations, use a new build directory: other cached
HDF5 library and include paths may still point at the old installation.

```sh
cmake -S . -B build -DNEPCOMPRESS_BUILD_LZ4=ON \
  -DNEPCOMPRESS_BUILD_BZIP2=OFF -DNEPCOMPRESS_ENABLE_FORTRAN=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/build/stage"
```

For both filters on this machine, use:

```sh
export PATH="/usr/local/hdf5-2.1.1/bin:/usr/local/netcdf-c/bin:/usr/local/netcdf-fortran/bin:$PATH"
export PKG_CONFIG_PATH="/usr/local/netcdf-c/lib/pkgconfig:/usr/local/netcdf-fortran/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="/usr/local/hdf5-2.1.1/lib:/usr/local/netcdf-c/lib:/usr/local/netcdf-fortran/lib:$LD_LIBRARY_PATH"
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/usr/local/hdf5-2.1.1;/usr/local/netcdf-c;/usr/local/netcdf-fortran" \
  -DHDF5_ROOT=/usr/local/hdf5-2.1.1 \
  -DNEPCOMPRESS_BUILD_BZIP2=ON \
  -DNEPCOMPRESS_BZIP2_PLUGIN_DIR=/usr/local/netcdf-c/hdf5/lib/plugin
cmake --build build
ctest --test-dir build --output-on-failure
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
| `NEPCOMPRESS_BUILD_BZIP2` | OFF | Build BZIP2 API and require external ID 307 plugin |
| `NEPCOMPRESS_BZIP2_PLUGIN_DIR` | empty | Directory to probe for BZIP2 when enabled |
| `NEPCOMPRESS_ENABLE_FORTRAN` | ON | Build Fortran wrappers and module |
| `NEPCOMPRESS_BUILD_BENCHMARKS` | OFF | Build slow, manual LZ4/BZIP2 examples |
| `BUILD_TESTING` | ON | Register C, HDF5 and optional Fortran tests |
| `NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR` | empty | Run optional read/write tests with an older LZ4 plugin |
| `NEPCOMPRESS_BUILD_DOCS` | OFF | Add a `docs` target that builds Doxygen HTML documentation |

## Building the documentation

The documentation builds with Doxygen and Graphviz `dot`, with no
NetCDF, HDF5 or Fortran requirements. Either run the script directly:

```sh
cmake -DNEPC_OUTPUT_DIR=build/docs -P docs/build_docs.cmake
```

or configure with `-DNEPCOMPRESS_BUILD_DOCS=ON` and build the `docs`
target; HTML lands in `build/docs/html`. Set
`-DNEPCOMPRESS_DOCS_WARN_AS_ERROR=FAIL_ON_WARNINGS` to make
documentation warnings fatal, as CI does.

## Using the libraries

```c
#include <nep_compress.h>
/* After nc_create(NC_NETCDF4), nc_def_dim and nc_def_var: */
int status = nc_def_var_lz4(ncid, varid, 3);
/* Or, with BZIP2 enabled and its plugin available: */
/* status = nc_def_var_bzip2(ncid, varid, 8); */
```

Link `libnepcompress` alongside NetCDF-C (or use the installed
`NEPCompress::nepcompress` CMake target). In Fortran, `use netcdf` and
`use nep_compress`, then call `nf90_def_var_lz4(ncid, varid, 3)` or
`nf90_def_var_bzip2(ncid, varid, 8)`. Link `libnepcompressf`; CMake
exports `NEPCompress::nepcompressf`. The module is installed under
`include/nep-compress`, so add that directory to Fortran's module path.

Former NEP clients switch `<nep.h>` to `<nep_compress.h>`, `-lnep` to
`-lnepcompress` for compression calls, and `use nep` / `-lnepf` to
`use nep_compress` / `-lnepcompressf`. Keep linking NEP separately if
you also use its format libraries. The C/Fortran procedure names, filter
IDs and file encoding stay the same; this is **not** a promise of
binary-compatible linkage to the old NEP libraries. Existing compressed
files need an appropriate HDF5 decoder on the reader's plugin path.

The [LZ4 on-disk format](plugins/LZ4_HDF5_format.md) and
[optional performance examples](examples/performance/README.md) provide
additional reference material. Tests cover nep-compress's own files;
interchange with files written through old NEP is not a release requirement.
