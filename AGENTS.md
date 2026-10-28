# AGENTS.md

- Local builds use `/usr/local/netcdf-c`, `/usr/local/netcdf-fortran`
  and `/usr/local/hdf5-2.1.1` (pass them through `CMAKE_PREFIX_PATH`
  and `HDF5_ROOT`; see README "Build and test").
- The Spack recipe lives in `spack/repos/spack_repo/nepcompress`;
  `docs/spack-plan.md` describes keeping it in sync with upstream
  `spack/spack-packages`.
