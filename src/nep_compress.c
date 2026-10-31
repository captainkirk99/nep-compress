/**
 * @file nep_compress.c
 * @brief Implementation of the nep-compress LZ4 and JPEG-LS filter API.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#include "nep_compress.h"

#ifdef NEPCOMPRESS_BUILD_LZ4
/** Number of parameters stored for filter 32004: block size and
 * encoder selector. */
#define LZ4_NPARAMS 2

int
nc_def_var_lz4(int ncid, int varid, int level)
{
    unsigned int params[LZ4_NPARAMS];
    int ret;

    if ((ret = nc_inq_filter_avail(ncid, LZ4_ID)))
        return ret;
    if (level < 1 || level > 9)
        return NC_EINVAL;
    params[0] = 0;
    params[1] = (unsigned int)(1 - level);
    return nc_def_var_filter(ncid, varid, LZ4_ID, LZ4_NPARAMS, params);
}

int
nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp)
{
    size_t nparams;
    unsigned int params[LZ4_NPARAMS];
    int level;
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
    if (nparams != 1 && nparams != LZ4_NPARAMS)
        return NC_EFILTER;
    if ((ret = nc_inq_var_filter_info(ncid, varid, LZ4_ID, &nparams, params)))
        return ret;
    if (nparams == 1)
        level = (int)params[0];
    else
    {
        int e = (int)params[1];

        if (e == 0 || e == 1)
            level = 1;
        else if (e < 0 && 1 - e <= 9)
            level = 1 - e;
        else
            return NC_EFILTER;
    }
    if (levelp)
        *levelp = level;
    return NC_NOERR;
}
#endif
#ifdef NEPCOMPRESS_BUILD_JPEGLS
/** Number of user parameters for filter 32018 (jls_parameters_t). */
#define JPEGLS_USER_NPARAMS 9
/** Number of parameters filter 32018 stores in the file after set_local. */
#define JPEGLS_STORED_NPARAMS 13
/** Index of the near-lossless bound in the user parameter form. */
#define JPEGLS_NEAR_USER 3
/** Index of the near-lossless bound in the stored parameter form. */
#define JPEGLS_NEAR_STORED 7
/** Largest chunk line or sample count accepted by the plugin. */
#define JPEGLS_MAX_XY 65535
/** Smallest number of pixels per chunk accepted by the plugin. */
#define JPEGLS_MIN_PIXELS 16
/** Largest number of image components (first chunk dimension, rank 3). */
#define JPEGLS_MAX_COMPONENTS 4

/**
 * Check a chunk shape against the JPEG-LS plugin's can_apply rules.
 *
 * In rank 3 the first chunk dimension is the component count and must
 * be 1-4. The last two chunk dimensions (lines and samples) must each
 * be at most 65535 and must hold at least 16 pixels.
 *
 * @param ndims Variable rank, already checked to be 2 or 3.
 * @param chunks Chunk sizes, one per dimension.
 * @return 1 when the plugin accepts the shape, 0 otherwise.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
jpegls_chunks_ok(int ndims, const size_t *chunks)
{
    size_t lines = chunks[ndims - 2], samples = chunks[ndims - 1];

    if (ndims == 3 && (chunks[0] < 1 || chunks[0] > JPEGLS_MAX_COMPONENTS))
        return 0;
    return lines <= JPEGLS_MAX_XY && samples <= JPEGLS_MAX_XY &&
           lines * samples >= JPEGLS_MIN_PIXELS;
}

int
nc_def_var_jpegls(int ncid, int varid, int near)
{
    unsigned int params[JPEGLS_USER_NPARAMS] = {0};
    size_t chunks[3];
    nc_type xtype;
    int ndims, storage, max_near, ret;

    if ((ret = nc_inq_filter_avail(ncid, JPEGLS_ID)))
        return ret;
    if ((ret = nc_inq_var(ncid, varid, NULL, &xtype, &ndims, NULL, NULL)))
        return ret;
    switch (xtype)
    {
    case NC_BYTE:
    case NC_UBYTE:
        max_near = 127;
        break;
    case NC_SHORT:
    case NC_USHORT:
        max_near = 255;
        break;
    default:
        return NC_EBADTYPE;
    }
    if (ndims != 2 && ndims != 3)
        return NC_EINVAL;
    if (near < 0 || near > max_near)
        return NC_EINVAL;
    if ((ret = nc_inq_var_chunking(ncid, varid, &storage, NULL)))
        return ret;
    if (storage != NC_CHUNKED &&
        (ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, NULL)))
        return ret;
    if ((ret = nc_inq_var_chunking(ncid, varid, &storage, chunks)))
        return ret;
    if (!jpegls_chunks_ok(ndims, chunks))
        return NC_EINVAL;
    params[JPEGLS_NEAR_USER] = (unsigned int)near;
    return nc_def_var_filter(ncid, varid, JPEGLS_ID, JPEGLS_USER_NPARAMS, params);
}

int
nc_inq_var_jpegls(int ncid, int varid, int *jpeglsp, int *nearp)
{
    unsigned int params[JPEGLS_STORED_NPARAMS];
    size_t nparams;
    int ret;

    if (jpeglsp)
        *jpeglsp = 0;
    ret = nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, NULL);
    if (ret == NC_ENOFILTER)
        return NC_NOERR;
    if (ret)
        return ret;
    if (jpeglsp)
        *jpeglsp = 1;
    if (nparams == 0)
    {
        if (nearp)
            *nearp = 0;
        return NC_NOERR;
    }
    if (nparams != JPEGLS_USER_NPARAMS && nparams != JPEGLS_STORED_NPARAMS)
        return NC_EFILTER;
    if ((ret = nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)))
        return ret;
    if (nearp)
        *nearp = (int)params[nparams == JPEGLS_USER_NPARAMS ?
                             JPEGLS_NEAR_USER : JPEGLS_NEAR_STORED];
    return NC_NOERR;
}
#endif
