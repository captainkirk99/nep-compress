/**
 * @file nep_compress.c
 * @brief Implementation of the nep-compress LZ4 filter API.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#include "nep_compress.h"

#ifdef NEPCOMPRESS_BUILD_LZ4
int
nc_def_var_lz4(int ncid, int varid, int level)
{
    unsigned int value;
    int ret;

    if ((ret = nc_inq_filter_avail(ncid, LZ4_ID)))
        return ret;
    if (level < 1 || level > 9)
        return NC_EINVAL;
    value = (unsigned int)level;
    return nc_def_var_filter(ncid, varid, LZ4_ID, 1, &value);
}

int
nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp)
{
    size_t nparams;
    unsigned int level;
    int ret;

    if (lz4p)
        *lz4p = 0;
    ret = nc_inq_var_filter_info(ncid, varid, LZ4_ID, &nparams, NULL);
    if (ret == NC_ENOFILTER)
        return NC_NOERR;
    if (ret)
        return ret;
    if (lz4p)
        *lz4p = 1;
    if (nparams != 1)
        return NC_EFILTER;
    if ((ret = nc_inq_var_filter_info(ncid, varid, LZ4_ID, &nparams, &level)))
        return ret;
    if (levelp)
        *levelp = (int)level;
    return NC_NOERR;
}
#endif
