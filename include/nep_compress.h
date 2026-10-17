#ifndef NEP_COMPRESS_H
#define NEP_COMPRESS_H

#include <netcdf.h>
#include <netcdf_filter.h>

#define BZIP2_ID 307
#define LZ4_ID 32004

#ifdef __cplusplus
extern "C" {
#endif

int nc_def_var_lz4(int ncid, int varid, int level);
int nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp);
int nc_def_var_bzip2(int ncid, int varid, int level);
int nc_inq_var_bzip2(int ncid, int varid, int *bzip2p, int *levelp);

#ifdef __cplusplus
}
#endif
#endif
