# Roadmap

## V1.2.0 Compression Filter for Mereosat

From "Earth Observation in Practice":

EUMETSAT disseminates FCI Level-1c imagery with stronger lossless compression based on the JPEG-LS standard through the CharLS library. JPEG-LS is implemented as a custom HDF5 filter (`H5ZJPEGLS`), not built into standard NetCDF-4 or HDF5 installations. The filename's `special_compression` field identifies these datasets; the product user guide documents which streams and dissemination paths use JPEG-LS.

To read JPEG-LS-compressed FCI NetCDF-4 files you must install the `fcidecomp` decompression software from EUMETSAT. `fcidecomp` supplies the `H5ZJPEGLS` HDF5 filter plugin, which the HDF5 library loads automatically when the file is opened. Once the plugin is installed, reads are transparent: `netCDF4.Dataset()` or `h5py.File()` behave as if the data were uncompressed. Without the plugin, accessing a compressed variable fails because the library does not recognise the filter.

- We need to accomodate this filter in nep-compress.

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
Add subdirectory performance.
Everything in performance subdir is written and run as a repo test, i.e. run with "make test"
Need a test which tries all available filters and compares.
We need some real data to try this on. Take a sizable (10 MB) sample from this data file: 
Style similar to this program from my book: https://github.com/captainkirk99/NetCDF_Developers_Handbook/blob/main/examples/performance/zstandard.c
Full doxygen documentation.

