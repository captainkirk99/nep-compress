#include "nep_compress.h"

#include <hdf5.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(NEPCOMPRESS_BUILD_LZ4) || defined(NEPCOMPRESS_BUILD_BZIP2)
static int
define_filter(int ncid, int varid, int level, unsigned int id, const char *name)
{
    unsigned int value;

    if (level < 1 || level > 9)
        return NC_EINVAL;
    if (!H5Zfilter_avail(id))
    {
        printf("%s filter not available.\n", name);
        return NC_EFILTER;
    }
    value = (unsigned int)level;
    return nc_def_var_filter(ncid, varid, id, 1, &value);
}
#endif

#ifdef NEPCOMPRESS_BUILD_LZ4
int
nc_def_var_lz4(int ncid, int varid, int level)
{
    return define_filter(ncid, varid, level, LZ4_ID, "lz4");
}

int
nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp)
{
    unsigned int id, level;
    size_t nparams;
    int ret = nc_inq_var_filter(ncid, varid, &id, &nparams, &level);

    if (ret == NC_ENOFILTER)
    {
        if (lz4p)
            *lz4p = 0;
        return NC_NOERR;
    }
    if (ret)
        return ret;
    if (lz4p)
        *lz4p = id == LZ4_ID;
    if (id == LZ4_ID)
    {
        if (nparams != 1)
            return NC_EFILTER;
        if (levelp)
            *levelp = (int)level;
    }
    return NC_NOERR;
}
#endif

#ifdef NEPCOMPRESS_BUILD_BZIP2
int
nc_def_var_bzip2(int ncid, int varid, int level)
{
    return define_filter(ncid, varid, level, BZIP2_ID, "bzip2");
}

int
nc_inq_var_bzip2(int ncid, int varid, int *bzip2p, int *levelp)
{
    size_t nfilters, nparams, i;
    unsigned int *ids, level;
    int ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, NULL);

    if (ret)
        return ret;
    if (bzip2p)
        *bzip2p = 0;
    if (!nfilters)
        return NC_NOERR;
    ids = malloc(nfilters * sizeof(*ids));
    if (!ids)
        return NC_ENOMEM;
    ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, ids);
    if (ret)
        goto done;
    for (i = 0; i < nfilters; i++)
    {
        if (ids[i] != BZIP2_ID)
            continue;
        ret = nc_inq_var_filter_info(ncid, varid, ids[i], &nparams, &level);
        if (ret)
            goto done;
        if (nparams != 1)
        {
            ret = NC_EFILTER;
            goto done;
        }
        if (levelp)
            *levelp = (int)level;
        if (bzip2p)
            *bzip2p = 1;
        break;
    }
done:
    free(ids);
    return ret;
}
#endif
