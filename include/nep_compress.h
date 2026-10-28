/**
 * @file nep_compress.h
 * @brief LZ4 and JPEG-LS compression filter API for NetCDF-4/HDF5.
 *
 * This header declares the nep-compress C API: functions that define
 * and inquire about the LZ4 (ID 32004) HDF5 compression filter on
 * NetCDF-4 variables. BZIP2 (ID 307) is defined and inquired with
 * NetCDF-C's nc_def_var_bzip2() and nc_inq_var_bzip2(), declared in
 * netcdf_filter.h, which this header includes. JPEG-LS (ID 32018) is
 * EUMETSAT's FCIDECOMP filter, as used for Meteosat FCI Level-1c
 * data. It needs an external plugin (EUMETSAT fcidecomp or
 * hdf5plugin) for writing and reading, and works only on 8- and
 * 16-bit integer imagery. Defining a filter must
 * be done in define mode, on a NetCDF-4 variable, before any data is
 * written; applying a filter forces chunked storage. The corresponding
 * HDF5 plugin must be on HDF5_PLUGIN_PATH for both writing and
 * reading.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#ifndef NEP_COMPRESS_H
#define NEP_COMPRESS_H

#include <netcdf.h>
#include <netcdf_filter.h>

/** HDF5 filter ID for BZIP2 compression. */
#define BZIP2_ID 307
/** HDF5 filter ID for LZ4 compression. */
#define LZ4_ID 32004
/** HDF5 filter ID for JPEG-LS compression (EUMETSAT FCIDECOMP, CharLS). */
#define JPEGLS_ID 32018

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define LZ4 compression on a NetCDF-4 variable.
 *
 * Must be called in define mode, before data is written. Applying the
 * filter forces chunked storage for the variable. The LZ4 HDF5 plugin
 * must be available on HDF5_PLUGIN_PATH.
 *
 * @param ncid NetCDF file ID, from nc_create() or nc_open().
 * @param varid Variable ID, from nc_def_var() or nc_inq_varid().
 * @param level LZ4 acceleration level, 1 to 9. Higher levels
 * compress faster at the cost of a lower compression ratio.
 * @return NC_NOERR on success, NC_ENOFILTER if the LZ4 filter plugin
 * is unavailable (checked before the level), NC_EINVAL for an invalid
 * level, or another NetCDF error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int nc_def_var_lz4(int ncid, int varid, int level);

/**
 * Inquire whether LZ4 compression is in use on a NetCDF-4 variable.
 *
 * The whole filter pipeline is searched, so LZ4 is found whatever its
 * position among the variable's filters. The LZ4 HDF5 plugin does not
 * need to be available.
 *
 * @param ncid NetCDF file ID.
 * @param varid Variable ID.
 * @param[out] lz4p If not NULL, set to 1 when the variable uses the
 * LZ4 filter and 0 otherwise. May be NULL.
 * @param[out] levelp If not NULL, set to the LZ4 acceleration level
 * stored with the filter. Written only when LZ4 is present with
 * exactly one parameter. May be NULL.
 * @return NC_NOERR on success, NC_EFILTER (with *lz4p set to 1) if the
 * LZ4 filter has a parameter count other than 1, or another NetCDF
 * error code, for example for a bad ID or a non-netCDF-4 file.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp);

/**
 * Define JPEG-LS compression on a NetCDF-4 variable.
 *
 * Must be called in define mode, before data is written. The variable
 * must be of type NC_BYTE, NC_UBYTE, NC_SHORT or NC_USHORT, and its
 * rank must be 2 or 3. If the variable is not chunked yet, NetCDF-C's
 * default chunk sizes are applied first. The resulting chunks must
 * satisfy the plugin's requirements: in rank 3 the first chunk
 * dimension is the number of image components and must be 1-4, the
 * last two chunk dimensions are each at most 65535, and there must be
 * at least 16 pixels per chunk. Changing the chunking after this call
 * to a shape outside these limits makes HDF5 store the data
 * uncompressed without an error.
 *
 * @param ncid NetCDF file ID, from nc_create() or nc_open().
 * @param varid Variable ID, from nc_def_var() or nc_inq_varid().
 * @param near JPEG-LS near-lossless bound: 0 for lossless; 1-127
 * (1-byte types) or 1-255 (2-byte types) for near-lossless (lossy)
 * compression, where every decoded value is within @a near of the
 * original.
 * @return NC_NOERR on success, NC_ENOFILTER if the JPEG-LS filter
 * plugin is unavailable (checked first), NC_EBADTYPE for an
 * unsupported type, NC_EINVAL for a bad rank, @a near or chunk shape
 * (after a chunk-shape NC_EINVAL the variable may have been switched
 * to chunked storage), or another NetCDF error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int nc_def_var_jpegls(int ncid, int varid, int near);

/**
 * Inquire whether JPEG-LS compression is in use on a NetCDF-4
 * variable.
 *
 * The whole filter pipeline is searched, so filter 32018 is found
 * whatever its position among the variable's filters. The JPEG-LS
 * HDF5 plugin does not need to be available.
 *
 * @param ncid NetCDF file ID.
 * @param varid Variable ID.
 * @param[out] jpeglsp If not NULL, set to 0 at the start of the call
 * and to 1 when the variable uses the JPEG-LS filter. May be NULL.
 * @param[out] nearp If not NULL, set to the JPEG-LS near-lossless
 * bound stored with the filter when 32018 is present with 0, 9 or 13
 * parameters: 0 for 0 parameters, parameter 3 for the 9-value user
 * form (define mode), or parameter 7 for the 13-value form that the
 * plugin stores in files (including EUMETSAT FCI files). May be NULL.
 * @return NC_NOERR on success, NC_EFILTER (with *jpeglsp set to 1) if
 * the JPEG-LS filter has a parameter count other than 0, 9 or 13, or
 * another NetCDF error code, for example for a bad ID or a
 * non-netCDF-4 file.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int nc_inq_var_jpegls(int ncid, int varid, int *jpeglsp, int *nearp);

#ifdef __cplusplus
}
#endif
#endif
