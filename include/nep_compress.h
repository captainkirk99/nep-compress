/**
 * @file nep_compress.h
 * @brief LZ4 and BZIP2 compression filter API for NetCDF-4/HDF5.
 *
 * This header declares the nep-compress C API: functions that define
 * and inquire about the LZ4 (ID 32004) and BZIP2 (ID 307) HDF5
 * compression filters on NetCDF-4 variables. Defining a filter must be
 * done in define mode, on a NetCDF-4 variable, before any data is
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
 * @return NC_NOERR on success, NC_EINVAL for an invalid level,
 * NC_EFILTER if the LZ4 filter plugin is unavailable, or another
 * NetCDF error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int nc_def_var_lz4(int ncid, int varid, int level);

/**
 * Inquire whether LZ4 compression is in use on a NetCDF-4 variable.
 *
 * @param ncid NetCDF file ID.
 * @param varid Variable ID.
 * @param[out] lz4p If not NULL, set to 1 when the variable uses the
 * LZ4 filter and 0 otherwise. May be NULL.
 * @param[out] levelp If not NULL, set to the LZ4 acceleration level
 * stored with the filter when the variable uses LZ4. May be NULL.
 * @return NC_NOERR on success, NC_EFILTER for inconsistent filter
 * parameters, or another NetCDF error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp);

/**
 * Define BZIP2 compression on a NetCDF-4 variable.
 *
 * Must be called in define mode, before data is written. Applying the
 * filter forces chunked storage for the variable. An external HDF5
 * plugin that implements filter ID 307 must be available on
 * HDF5_PLUGIN_PATH.
 *
 * @param ncid NetCDF file ID, from nc_create() or nc_open().
 * @param varid Variable ID, from nc_def_var() or nc_inq_varid().
 * @param level BZIP2 compression level, 1 to 9 (block sizes of
 * 100-900 KB). Higher levels favor compression ratio over speed.
 * @return NC_NOERR on success, NC_EINVAL for an invalid level,
 * NC_EFILTER if the BZIP2 filter plugin is unavailable, or another
 * NetCDF error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int nc_def_var_bzip2(int ncid, int varid, int level);

/**
 * Inquire whether BZIP2 compression is in use on a NetCDF-4 variable.
 *
 * @param ncid NetCDF file ID.
 * @param varid Variable ID.
 * @param[out] bzip2p If not NULL, set to 1 when the variable uses the
 * BZIP2 filter and 0 otherwise. May be NULL.
 * @param[out] levelp If not NULL, set to the BZIP2 compression level
 * stored with the filter when the variable uses BZIP2. May be NULL.
 * @return NC_NOERR on success, NC_EFILTER for inconsistent filter
 * parameters, or another NetCDF error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int nc_inq_var_bzip2(int ncid, int varid, int *bzip2p, int *levelp);

#ifdef __cplusplus
}
#endif
#endif
