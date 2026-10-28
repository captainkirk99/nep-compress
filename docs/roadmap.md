# Roadmap

## V1.2.0 Compression Filter for Meteosat

### Sprint 1: Add JPEG-LS Filter Support
- EUMETSAT JPEG-LS (HDF5 ID 32018, FCIDECOMP) through an external plugin (fcidecomp or hdf5plugin `libh5fcidecomp.so`), like BZIP2: `NEPCOMPRESS_BUILD_JPEGLS` (default OFF) and `NEPCOMPRESS_JPEGLS_PLUGIN_DIR` with a configure probe.
- C `nc_def_var_jpegls(ncid, varid, near)` / `nc_inq_var_jpegls(ncid, varid, &has, &near)` plus Fortran `nf90_*_jpegls`. `near` 0 is lossless; `near` > 0 is near-lossless (lossy).
- The define call returns `NC_ENOFILTER` first, `NC_EBADTYPE` for anything other than byte/ubyte/short/ushort, and `NC_EINVAL` for a bad rank, `near` or chunk shape, so data is never silently stored raw. The inquiry handles the 13-parameter form in FCI files.
- Synthetic-data tests only (`tst_jpegls`, `jpegls_missing_plugin`, `ftst_nep_jpegls`). CI uses the hdf5plugin directory.
- The "Earth Observation in Practice" quote is kept word for word in `docs/compression.md` and `docs/release-notes-v1.2.0.md`. A JPEG-LS row is added to the mainpage table. Version 1.2.0, SOVERSION 1.
- Out of scope: building our own CharLS plugin, committed or downloaded FCI data, JPEG-LS in `perf_compare_filters`, other JPEG-LS parameters.

Plan: [docs/plans/v1.2.0-sprint1-add-jpeg-ls-filter-support.md](plans/v1.2.0-sprint1-add-jpeg-ls-filter-support.md)

## V1.1.0 - Documentation

### Sprint 1: Documentation
nepcompress.h needs doxygen documentation too.
Need to build and deploy github pages documentation, using doxygen.
Need a CI action (user-run) which updates the gh-pages docs from source.
Need this action to be run automatically after every release.
Need a doxygen mainpage which describes nep-compress and has links to the provided compression filters, as well as a discussion which compares all available filters, including those built in to netcdf-c, zstd, zlib, and szlib.
Make sure all functions include @author tag, and all test code and some documentation include the author: Edward Hartnett, Intelligent Data Design, Inc.
The README should reference my two books: https://www.amazon.com/NetCDF-Developers-Handbook-Authoritative-High-Performance/dp/B0GYP4R5ZZ/ref=sr_1_2?dib=eyJ2IjoiMSJ9.Id1w-Fwu7QyFJvhJL0kdBrp07ACXa_DmUZaljsc5TflwIyQ_Zm9xmiRX4pwZ25ib.3U2eqqQeE5a5gX9QcbHcWEerID3L85jIX_I72uBvAYw&dib_tag=se&keywords=netcdf&qid=1792931211&sr=8-2 and https://www.amazon.com/Earth-Observation-Practice-Mission-Mission/dp/B0HJRZ9F7L/ref=sr_1_1?crid=4TKSYJ9Q604C&dib=eyJ2IjoiMSJ9.oxHqFCY61RhO47h_lKJ5XnDkmiGivjctgiLV7OTBjmD0qKdjqMMi0MM5L4OPH54bibGUud8QwV9GdLozmPn2C_gk1hZnhXOXE-1j8FqFJiyKVGFm1k9TBpgvgay2-9ic-k_IBlhQMXm5LfEj-TQ5C2IVcJwaOwUL7uhDgJrjjC8HN8-VxO44BPjXEAc4jRgLWzoN0SM_1pzEJfsVpnsr1katGmhwX_Zb8Vb5rXU6l6s.xKzsXo5C3IZ0gdGkmYGv6s58XrkmcTMxJlwbrvlUB2c&dib_tag=se&keywords=%22earth+observation+in+practice%22+hartnett&qid=1792931437&sprefix=earth+observation+in+practice+hartnett%2Caps%2C624&sr=8-1
The covers of both are stored in the docs directory as jpgs, make sure to use them in the README, at an size so they can appear side-by-side.

### Sprint 2: Fix Issues
Fix issues #3, #4 and #5.
- #3: remove the C `nc_def_var_bzip2`/`nc_inq_var_bzip2` from libnepcompress and use NetCDF-C's. Keep the Fortran `nf90_*_bzip2` wrappers bound to them. Keep SOVERSION 1.
- `nc_def_var_lz4` checks plugin availability first and returns `NC_ENOFILTER`, matching NetCDF-C. Nothing is printed to stdout.
- #4: `nc_inq_var_lz4` looks up ID 32004 anywhere in the pipeline and reads the parameter count first. Add regression tests and a C-only ASan CI job.
- #5: each test gets only its own filter's plugin directory, and a Linux check confirms the loaded LZ4 plugin is from `build/plugin`. The README documents a single-provider BZIP2 directory.
- Update the header, Fortran, compression guide, mainpage, README and netcdf skill docs for the new behavior.
- Out of scope: a configure-time plugin scanner or provider staging.

Plan: [docs/plans/v1.1.0-sprint2-fix-issues.md](plans/v1.1.0-sprint2-fix-issues.md)

### Sprint 3: Compression Performance Example
- New top-level `performance/` with `compare_filters.c` (book `zstandard.c` style, full Doxygen), run by ctest as `perf_compare_filters`. `examples/performance` is unchanged.
- Compares none, deflate 1/5/9, szip, zstd 1/3/9/19, bzip2 1/9, LZ4 1/9, each with and without shuffle; unavailable filters are skipped. Timed: create-write-close and open-read-close, minimum of 3 runs. Pass = bit-exact round trip and filter IDs verified; no ratio or speed thresholds.
- Data: `test/data/gdas_sfc_sample.nc` (`tmp2m`, `prate_ave`, `soilw1`, `land` from the GDAS sfcf003 file, shuffle + zstd 19, about 6.4 MB), built by `performance/make_gdas_sample.sh`.
- New `NEPCOMPRESS_ZSTD_PLUGIN_DIR` (needed to read the sample, and it enables the test). The test uses an ordered plugin path with `build/plugin` first, plus the LZ4 provider check.
- Reference CSV and machine info are committed and summarized on the mainpage. CI runs the test in every job and uploads the CSV.
- Out of scope: quantize/lossy rows, blosc and other hdf5plugin filters, plots, ratio/speed assertions.

Plan: [docs/plans/v1.1.0-sprint3-compression-performance-example.md](plans/v1.1.0-sprint3-compression-performance-example.md)

