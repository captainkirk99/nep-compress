@mainpage nep-compress

# What is nep-compress?

nep-compress provides LZ4 and BZIP2 compression filters for
NetCDF-4/HDF5, with C and optional Fortran wrapper APIs. It is being
separated from [NEP](https://github.com/Intelligent-Data-Design-Inc/NEP)
to make compression easier to build and use without installing NEP's
format readers. NEP remains the project for UDF formats and NEXTCDF-4.

nep-compress builds the LZ4 HDF5 plugin (`libh5lz4`, filter ID 32004)
itself. For BZIP2 (filter ID 307) it builds the C and Fortran API but
requires an external HDF5 plugin that provides ID 307 for both writing
and reading.

# Provided filters

The C API is declared in nep_compress.h:

- nc_def_var_lz4(), nc_inq_var_lz4() — LZ4, filter ID 32004 (#LZ4_ID).
- nc_def_var_bzip2(), nc_inq_var_bzip2() — BZIP2, filter ID 307
  (#BZIP2_ID).

The Fortran module `nep_compress` provides the equivalents
nf90_def_var_lz4(), nf90_inq_var_lz4(), nf90_def_var_bzip2(), and
nf90_inq_var_bzip2().

The LZ4 filter plugin itself is implemented in H5Zlz4.c. Its on-disk
chunk encoding is described in @ref md_plugins_2LZ4__HDF5__format "the
LZ4 HDF5 format page". See the @ref md_docs_2compression "compression
guide" for usage and file-compatibility details.

# NetCDF-4 compression filter comparison

NetCDF-4 variables are compressed through HDF5 filters applied to
chunked storage. The filters available to NetCDF-4 users:

| Filter | HDF5 ID | Provided by | Plugin required | API call | Parameters / levels | Typical trade-off | Lossless |
| --- | --- | --- | --- | --- | --- | --- | --- |
| deflate (zlib) | 1 | Built into HDF5 | No | `nc_def_var_deflate(ncid, varid, shuffle, deflate, level)` | level 1-9 (0 = no compression); the same call's shuffle argument controls shuffle | Balance of speed and ratio; universally readable | Yes |
| szip | 4 | HDF5 built with libaec/szip | No (when HDF5 has szip) | `nc_def_var_szip(ncid, varid, options_mask, pixels_per_block)` | `NC_SZIP_NN` or `NC_SZIP_EC`; pixels_per_block even and <= 32 | Good on integer/telemetry-like data; cannot be combined with deflate | Yes |
| zstandard | 32015 | NetCDF-C plugin `lib__nch5zstd` | Yes | `nc_def_var_zstandard(ncid, varid, level)` | levels -131072 to 22 | Wide range from fast to high ratio | Yes |
| bzip2 | 307 | External HDF5 plugin (NetCDF-C ships `lib__nch5bzip2`; other providers exist) | Yes | `nc_def_var_bzip2(ncid, varid, level)` | level 1-9 (block size 100-900 KB) | Favors ratio over speed | Yes |
| LZ4 | 32004 | nep-compress plugin `libh5lz4` | Yes | `nc_def_var_lz4(ncid, varid, level)` | level 1-9 (acceleration parameter) | Favors speed | Yes |

Note that `nc_def_var_zstandard()` does nothing when NetCDF-C was built
without zstd; check `NC_HAS_ZSTD` or `nc-config --has-zstd` before
relying on it.

## Pre-filters and checksums

These HDF5 pipeline entries are not compressors but change the result:

- **shuffle** (ID 2) reorders bytes so the significant bytes of each
  element are grouped together, usually improving the ratio of the
  compressor that follows it. It is enabled through the shuffle argument
  of `nc_def_var_deflate()`.
- **quantize** (`nc_def_var_quantize()`) is a *lossy* step that zeroes
  low-order mantissa bits (`NC_QUANTIZE_BITGROOM`,
  `NC_QUANTIZE_GRANULARBR`, or `NC_QUANTIZE_BITROUND`), improving the
  compression achieved by a subsequent lossless filter.
- **fletcher32** (ID 3) is a checksum filter, not compression.

## Choosing a filter

- For speed, especially write or read throughput, use **LZ4**.
- For the smallest files, use **BZIP2** or a high **zstandard** level.
- For maximum portability — a file any NetCDF-4 reader can open with no
  extra software — use **deflate**, which is built into HDF5.

Every filter other than deflate and szip requires the HDF5 plugin that
implements it to be present on `HDF5_PLUGIN_PATH`, both when writing and
when a reader later opens the file.

The [performance examples](https://github.com/captainkirk99/nep-compress/blob/main/examples/performance/README.md)
contain historical LZ4 and BZIP2 measurements
([lz4_results.csv](https://github.com/captainkirk99/nep-compress/blob/main/examples/performance/lz4_results.csv),
[bzip2_results.csv](https://github.com/captainkirk99/nep-compress/blob/main/examples/performance/bzip2_results.csv)).
Measured comparisons across all of these filters will be published in a
later release.

# Books

<table><tr>
<td><a href="https://www.amazon.com/dp/B0H7Q49MPS"><img src="NDH_cover.jpg" width="200" alt="The NetCDF Developer's Handbook, Second Edition"></a></td>
<td><a href="https://www.amazon.com/dp/B0HJRZ9F7L"><img src="EOiP_cover.jpg" width="200" alt="Earth Observation in Practice"></a></td>
</tr></table>

- [The NetCDF Developer's Handbook: The Authoritative Guide to Writing
  High-Performance Programs for Scientific Data Management, Second
  Edition](https://www.amazon.com/dp/B0H7Q49MPS) — the complete NetCDF-C
  and NetCDF-Fortran reference, including compression filters, chunking
  and quantization.
- [Earth Observation in Practice: A Mission-by-Mission Guide to Reading
  Satellite Data with NetCDF and
  Python](https://www.amazon.com/dp/B0HJRZ9F7L) — a mission-by-mission
  guide to the spacecraft, instruments and data products of modern
  remote sensing.

# Author

Edward Hartnett, Intelligent Data Design, Inc.
