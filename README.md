# nep-compress

Standalone NetCDF-4/HDF5 LZ4 and BZIP2 C/Fortran compression APIs. The LZ4
HDF5 plugin is built here; BZIP2 uses an external HDF5 plugin (not merely
`libbz2`). NEP's format readers and logging are not dependencies.

## Local build

For the current local installations:

```sh
export PATH="/usr/local/hdf5-2.1.1/bin:/usr/local/netcdf-c/bin:/usr/local/netcdf-fortran/bin:$PATH"
export PKG_CONFIG_PATH="/usr/local/netcdf-c/lib/pkgconfig:/usr/local/netcdf-fortran/lib/pkgconfig:$PKG_CONFIG_PATH"
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/usr/local/hdf5-2.1.1;/usr/local/netcdf-c;/usr/local/netcdf-fortran" \
  -DHDF5_ROOT=/usr/local/hdf5-2.1.1 \
  -DNEPCOMPRESS_BUILD_BZIP2=ON \
  -DNEPCOMPRESS_BZIP2_PLUGIN_DIR=/usr/local/netcdf-c/hdf5/lib/plugin
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/build/stage"
```

LZ4 is ON, BZIP2 is OFF, and Fortran is ON by default. Configure with
`-DNEPCOMPRESS_ENABLE_FORTRAN=OFF` if NetCDF-Fortran is not installed. Set
`HDF5_PLUGIN_PATH` to the installed `lib/plugin` directory for LZ4 and to
the BZIP2 plugin directory when BZIP2 is used. Enabled BZIP2 requires filter
307 with both encoder and decoder; configure fails if it is missing.

Include `<nep_compress.h>` and link `libnepcompress`; Fortran clients use
`use nep_compress` (module installed under `include/nep-compress`) and link
`libnepcompressf`. Existing `nc_*_var_lz4/bzip2` and
`nf90_*_var_lz4/bzip2` entry points retain their signatures. This is a
source/build migration from NEP's `<nep.h>`, `libnep`, and `use nep`.

For cross-plugin LZ4 file tests, configure
`NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR` with an older plugin directory;
the default tests exercise this project's plugin alone. The current
implementation does not bundle or redistribute a BZIP2 plugin.
