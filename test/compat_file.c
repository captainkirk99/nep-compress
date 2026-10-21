#include "nep_compress.h"

#include <netcdf.h>
#include <stdio.h>
#include <string.h>

#define COUNT 64

static int
fail(int status, const char *operation)
{
    fprintf(stderr, "%s: %s\n", operation, nc_strerror(status));
    return 1;
}

int
main(int argc, char **argv)
{
    int ncid, varid, dimid, storage, values[COUNT], actual[COUNT], status, i;
    unsigned int id, param;
    size_t nparams = 1, chunk;
    const int lz4 = argc == 4 && strcmp(argv[3], "lz4") == 0;
    const int bzip2 = argc == 4 && strcmp(argv[3], "bzip2") == 0;
    const unsigned int expected_id = lz4 ? LZ4_ID : BZIP2_ID;
    const int level = lz4 ? 3 : 8;

    if (argc != 4 || (!lz4 && !bzip2) ||
        (strcmp(argv[1], "write") && strcmp(argv[1], "read") &&
         strcmp(argv[1], "missing")))
    {
        fprintf(stderr, "usage: compat_file write|read|missing path lz4|bzip2\n");
        return 2;
    }

    for (i = 0; i < COUNT; i++)
        values[i] = (i * i + 17) % 103;

    if (strcmp(argv[1], "write") == 0 || strcmp(argv[1], "missing") == 0)
    {
        if ((status = nc_create(argv[2], NC_NETCDF4 | NC_CLOBBER, &ncid)))
            return fail(status, "create");
        if ((status = nc_def_dim(ncid, "sample", COUNT, &dimid)))
            return fail(status, "dimension");
        if ((status = nc_def_var(ncid, "values", NC_INT, 1, &dimid, &varid)))
            return fail(status, "variable");
        if ((status = nc_def_var_chunking(ncid, varid, NC_CHUNKED, (size_t[]){16})))
            return fail(status, "chunking");
#if defined(NEPCOMPRESS_BUILD_LZ4) && defined(NEPCOMPRESS_BUILD_BZIP2)
        status = lz4 ? nc_def_var_lz4(ncid, varid, level)
                     : nc_def_var_bzip2(ncid, varid, level);
#elif defined(NEPCOMPRESS_BUILD_LZ4)
        status = nc_def_var_lz4(ncid, varid, level);
#else
        status = nc_def_var_bzip2(ncid, varid, level);
#endif
        if (strcmp(argv[1], "missing") == 0)
        {
            nc_close(ncid);
            return status == NC_EFILTER ? 0 : fail(status, "expected NC_EFILTER");
        }
        if (status)
            return fail(status, "filter");
        if ((status = nc_put_var_int(ncid, varid, values)))
            return fail(status, "write");
        if ((status = nc_close(ncid)))
            return fail(status, "close");
    }
    else
    {
        if ((status = nc_open(argv[2], NC_NOWRITE, &ncid)))
            return fail(status, "open");
        if ((status = nc_inq_varid(ncid, "values", &varid)))
            return fail(status, "variable");
        if ((status = nc_inq_var_chunking(ncid, varid, &storage, &chunk)))
            return fail(status, "chunking");
        if (storage != NC_CHUNKED || chunk != 16)
        {
            fprintf(stderr, "unexpected chunking: %d %zu\n", storage, chunk);
            return 1;
        }
        if ((status = nc_inq_var_filter(ncid, varid, &id, &nparams, &param)))
            return fail(status, "filter info");
        if (id != expected_id || nparams != 1 || param != (unsigned int)level)
        {
            fprintf(stderr, "unexpected filter ID/parameters: %u %zu %u\n", id, nparams, param);
            return 1;
        }
        if ((status = nc_get_var_int(ncid, varid, actual)))
            return fail(status, "read");
        for (i = 0; i < COUNT; i++)
            if (values[i] != actual[i])
            {
                fprintf(stderr, "mismatch at %d\n", i);
                return 1;
            }
        if ((status = nc_close(ncid)))
            return fail(status, "close");
    }
    return 0;
}
