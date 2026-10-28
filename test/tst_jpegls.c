/*
 * Test the JPEG-LS (ID 32018) compression filter through the
 * nep-compress nc_def_var_jpegls() and nc_inq_var_jpegls() functions.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */

#include "nep_compress.h"
#include "nep_test.h"
#include <hdf5.h>
#include <H5DSpublic.h>
#include <netcdf.h>
#include <string.h>
#include <stdlib.h>

/* Test file names. */
#define FILE_NAME "tst_jpegls_lossless.nc"
#define SHORT_FILE_NAME "tst_jpegls_short.nc"
#define UBYTE_FILE_NAME "tst_jpegls_ubyte.nc"
#define BYTE_FILE_NAME "tst_jpegls_byte.nc"
#define NEAR_FILE_NAME "tst_jpegls_near.nc"
#define NEAR_UBYTE_FILE_NAME "tst_jpegls_near_ubyte.nc"
#define NEAR_MAX_FILE_NAME "tst_jpegls_near_max.nc"
#define RANK3_FILE_NAME "tst_jpegls_rank3.nc"
#define SHUFFLE_FILE_NAME "tst_jpegls_shuffle.nc"
#define REJECT_FILE_NAME "tst_jpegls_reject.nc"
#define PARAMS_FILE_NAME "tst_jpegls_params.nc"
#define MISSING_FILE_NAME "tst_jpegls_missing.nc"

/* Names of the test variable and its dimensions. */
#define VAR_NAME "radiance"
#define X_NAME "x"
#define Y_NAME "y"
#define T_NAME "t"

/* Size of the test grids. */
#define NX 120
#define NY 100
#define NT 4

/* Err is used to keep track of errors within each set of tests,
 * total_err is the number of errors in the entire test program, which
 * generally cosists of several sets of tests. */
static int total_err = 0, err = 0; /* Errors in this set of tests. */

/*
 * Check that a variable's data was actually compressed: the plugin
 * silently stores chunks raw when compression fails, so compare the
 * HDF5 storage size with the raw size.
 *
 * Returns 0 when the stored size is less than half the raw size, 1
 * otherwise.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
storage_ok(const char *path, const char *var, size_t raw_bytes)
{
    hid_t fileid = -1, datasetid = -1;
    hsize_t stored = 0;
    int ret = 1;

    if ((fileid = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        return 1;
    if ((datasetid = H5Dopen2(fileid, var, H5P_DEFAULT)) >= 0)
    {
        stored = H5Dget_storage_size(datasetid);
        if (stored > 0 && stored < raw_bytes / 2)
            ret = 0;
    }
    if (datasetid >= 0)
        H5Dclose(datasetid);
    H5Fclose(fileid);
    return ret;
}

/*
 * Run the test program. With argument "missing", run only the
 * missing-plugin checks.
 *
 * Returns 0 on success, 2 if any test failed.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
int
main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "missing") == 0)
    {
        int ncid, dimid[2], varid;
        int has = -1, near = -1;

        printf("\n*** Checking JPEG-LS with a missing plugin.\n");
        printf("*** Checking nc_def_var_jpegls without the plugin...");
        {
            /* The define must fail with NC_ENOFILTER. */
            if (nc_create(MISSING_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
            if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
            if (nc_def_var(ncid, VAR_NAME, NC_USHORT, 2, dimid, &varid)) ERR;
            if (nc_def_var_jpegls(ncid, varid, 0) != NC_ENOFILTER) ERR;
            if (nc_abort(ncid)) ERR;
        }
        SUMMARIZE_ERR;
        printf("*** Checking inquiry and read without the plugin...");
        {
            unsigned short data_in[NY][NX];

            /* Metadata is readable without the plugin, data is not. */
            if (nc_open(FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_varid(ncid, VAR_NAME, &varid)) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
            if (has != 1 || near != 0) ERR;
            if (nc_get_var_ushort(ncid, varid, &data_in[0][0]) != NC_ENOFILTER) ERR;
            if (nc_close(ncid)) ERR;
        }
        SUMMARIZE_ERR;
        FINAL_RESULTS;
    }

    printf("\n*** Checking HDF5 JPEG-LS compression.\n");
    printf("*** Checking jpegls inquiry with no filter...");
    {
        int ncid;
        int dimid[2];
        int varid;
        int has = -1, near = -1;

        /* Create file and a plain variable. */
        if (nc_create(PARAMS_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_USHORT, 2, dimid, &varid)) ERR;

        /* No filter: has is 0, NULL pointers are allowed. */
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has) ERR;
        if (nc_inq_var_jpegls(ncid, varid, NULL, &near)) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;
        if (nc_inq_var_jpegls(ncid, varid, NULL, NULL)) ERR;
        if (nc_abort(ncid)) ERR;
    }
    SUMMARIZE_ERR;
    printf("*** Checking lossless jpegls with NC_USHORT...");
    {
        int ncid;
        int dimid[2];
        int varid;
        unsigned short data_out[NY][NX];
        int x, y;
        int has = 0, near = -1;
        size_t nparams = 0;
        unsigned int params[13];

        /* Create some data to write. */
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data_out[y][x] = (x * 7 + y * 3) & 0x0fff;

        /* Create file; default chunking is used. */
        if (nc_create(FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_USHORT, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0)) ERR;

        /* In define mode the user parameter form (9 values) is seen. */
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 0) ERR;
        if (nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)) ERR;
        if (nparams != 9) ERR;

        /* Write the data and close. */
        if (nc_put_var_ushort(ncid, varid, &data_out[0][0])) ERR;
        if (nc_close(ncid)) ERR;

        /* The data must really be compressed, not stored raw. */
        if (storage_ok(FILE_NAME, VAR_NAME, NX * NY * sizeof(unsigned short))) ERR;

        {
            unsigned short data_in[NY][NX];

            /* Reopen and check the stored 13-parameter form. */
            if (nc_open(FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
            if (has != 1 || near != 0) ERR;
            nparams = 0;
            if (nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)) ERR;
            if (nparams != 13 || params[0] != sizeof(unsigned short)) ERR;
            if (nc_get_var_ushort(ncid, varid, &data_in[0][0])) ERR;
            for (y = 0; y < NY; y++)
                for (x = 0; x < NX; x++)
                    if (data_in[y][x] != data_out[y][x]) ERR;
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking lossless jpegls with NC_SHORT...");
    {
        int ncid;
        int dimid[2];
        int varid;
        short data_out[NY][NX];
        int x, y;
        int has = 0, near = -1;
        size_t nparams = 0;
        unsigned int params[13];

        /* Create some data to write. */
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data_out[y][x] = (x * 7 + y * 3) % 3000 - 1500;

        if (nc_create(SHORT_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_SHORT, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0)) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 0) ERR;
        if (nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)) ERR;
        if (nparams != 9) ERR;
        if (nc_put_var_short(ncid, varid, &data_out[0][0])) ERR;
        if (nc_close(ncid)) ERR;
        if (storage_ok(SHORT_FILE_NAME, VAR_NAME, NX * NY * sizeof(short))) ERR;

        {
            short data_in[NY][NX];

            if (nc_open(SHORT_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
            if (has != 1 || near != 0) ERR;
            nparams = 0;
            if (nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)) ERR;
            if (nparams != 13 || params[0] != sizeof(short)) ERR;
            if (nc_get_var_short(ncid, varid, &data_in[0][0])) ERR;
            for (y = 0; y < NY; y++)
                for (x = 0; x < NX; x++)
                    if (data_in[y][x] != data_out[y][x]) ERR;
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking lossless jpegls with NC_UBYTE...");
    {
        int ncid;
        int dimid[2];
        int varid;
        unsigned char data_out[NY][NX];
        int x, y;
        int has = 0, near = -1;
        size_t nparams = 0;
        unsigned int params[13];

        /* Create some data to write. */
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data_out[y][x] = (x + y) % 200;

        if (nc_create(UBYTE_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_UBYTE, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0)) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 0) ERR;
        if (nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)) ERR;
        if (nparams != 9) ERR;
        if (nc_put_var_uchar(ncid, varid, &data_out[0][0])) ERR;
        if (nc_close(ncid)) ERR;
        if (storage_ok(UBYTE_FILE_NAME, VAR_NAME, NX * NY * sizeof(unsigned char))) ERR;

        {
            unsigned char data_in[NY][NX];

            if (nc_open(UBYTE_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
            if (has != 1 || near != 0) ERR;
            nparams = 0;
            if (nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)) ERR;
            if (nparams != 13 || params[0] != sizeof(unsigned char)) ERR;
            if (nc_get_var_uchar(ncid, varid, &data_in[0][0])) ERR;
            for (y = 0; y < NY; y++)
                for (x = 0; x < NX; x++)
                    if (data_in[y][x] != data_out[y][x]) ERR;
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking lossless jpegls with NC_BYTE...");
    {
        int ncid;
        int dimid[2];
        int varid;
        signed char data_out[NY][NX];
        int x, y;
        int has = 0, near = -1;
        size_t nparams = 0;
        unsigned int params[13];

        /* Create some data to write. */
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data_out[y][x] = (x + y) % 100 - 50;

        if (nc_create(BYTE_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_BYTE, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0)) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 0) ERR;
        if (nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)) ERR;
        if (nparams != 9) ERR;
        if (nc_put_var_schar(ncid, varid, &data_out[0][0])) ERR;
        if (nc_close(ncid)) ERR;
        if (storage_ok(BYTE_FILE_NAME, VAR_NAME, NX * NY * sizeof(signed char))) ERR;

        {
            signed char data_in[NY][NX];

            if (nc_open(BYTE_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
            if (has != 1 || near != 0) ERR;
            nparams = 0;
            if (nc_inq_var_filter_info(ncid, varid, JPEGLS_ID, &nparams, params)) ERR;
            if (nparams != 13 || params[0] != sizeof(signed char)) ERR;
            if (nc_get_var_schar(ncid, varid, &data_in[0][0])) ERR;
            for (y = 0; y < NY; y++)
                for (x = 0; x < NX; x++)
                    if (data_in[y][x] != data_out[y][x]) ERR;
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking near-lossless jpegls with near = 2...");
    {
        int ncid;
        int dimid[2];
        int varid;
        unsigned short data_out[NY][NX];
        int x, y;
        int has = 0, near = -1;
        int ndiff = 0;

        /* Create some data to write. */
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data_out[y][x] = (x * 7 + y * 3) & 0x0fff;

        if (nc_create(NEAR_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_USHORT, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 2)) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 2) ERR;
        if (nc_put_var_ushort(ncid, varid, &data_out[0][0])) ERR;
        if (nc_close(ncid)) ERR;
        if (storage_ok(NEAR_FILE_NAME, VAR_NAME, NX * NY * sizeof(unsigned short))) ERR;

        {
            unsigned short data_in[NY][NX];

            if (nc_open(NEAR_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
            if (has != 1 || near != 2) ERR;
            if (nc_get_var_ushort(ncid, varid, &data_in[0][0])) ERR;
            /* Every value is within 2, and at least one differs. */
            for (y = 0; y < NY; y++)
                for (x = 0; x < NX; x++)
                {
                    if (abs((int)data_in[y][x] - (int)data_out[y][x]) > 2) ERR;
                    if (data_in[y][x] != data_out[y][x])
                        ndiff++;
                }
            if (!ndiff) ERR;
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking near-lossless jpegls limits...");
    {
        int ncid;
        int dimid[2];
        int varid;
        unsigned char data_out[NY][NX];
        int x, y;
        int has = 0, near = -1;

        /* Maximum near for a 1-byte type. */
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data_out[y][x] = (x + y) % 200;

        if (nc_create(NEAR_UBYTE_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_UBYTE, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 127)) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 127) ERR;
        if (nc_put_var_uchar(ncid, varid, &data_out[0][0])) ERR;
        if (nc_close(ncid)) ERR;

        if (nc_open(NEAR_UBYTE_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        near = -1;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 127) ERR;
        if (nc_close(ncid)) ERR;
    }
    {
        int ncid;
        int dimid[2];
        int varid;
        unsigned short data_out[NY][NX];
        int x, y;
        int has = 0, near = -1;

        /* Maximum near for a 2-byte type. */
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data_out[y][x] = (x * 7 + y * 3) & 0x0fff;

        if (nc_create(NEAR_MAX_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_USHORT, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 255)) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 255) ERR;
        if (nc_put_var_ushort(ncid, varid, &data_out[0][0])) ERR;
        if (nc_close(ncid)) ERR;

        if (nc_open(NEAR_MAX_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        near = -1;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 255) ERR;
        if (nc_close(ncid)) ERR;
    }
    SUMMARIZE_ERR;
    printf("*** Checking lossless jpegls on rank-3 variables...");
    {
        int ncid;
        int dimid[3];
        int varid1, varid2;
        unsigned short data_out[NT][NY][NX];
        int t, x, y;
        int has = 0, near = -1;
        size_t chunks[3];

        /* Create some data to write. */
        for (t = 0; t < NT; t++)
            for (y = 0; y < NY; y++)
                for (x = 0; x < NX; x++)
                    data_out[t][y][x] = (x * 7 + y * 3 + t) & 0x0fff;

        if (nc_create(RANK3_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, T_NAME, NT, &dimid[0])) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[1])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[2])) ERR;

        /* One variable with 1 component per chunk, one with 4. */
        if (nc_def_var(ncid, "radiance1", NC_USHORT, 3, dimid, &varid1)) ERR;
        chunks[0] = 1;
        chunks[1] = NY;
        chunks[2] = NX;
        if (nc_def_var_chunking(ncid, varid1, NC_CHUNKED, chunks)) ERR;
        if (nc_def_var_jpegls(ncid, varid1, 0)) ERR;

        if (nc_def_var(ncid, "radiance4", NC_USHORT, 3, dimid, &varid2)) ERR;
        chunks[0] = 4;
        if (nc_def_var_chunking(ncid, varid2, NC_CHUNKED, chunks)) ERR;
        if (nc_def_var_jpegls(ncid, varid2, 0)) ERR;

        if (nc_inq_var_jpegls(ncid, varid1, &has, &near)) ERR;
        if (has != 1 || near != 0) ERR;
        if (nc_inq_var_jpegls(ncid, varid2, &has, &near)) ERR;
        if (has != 1 || near != 0) ERR;

        if (nc_put_var_ushort(ncid, varid1, &data_out[0][0][0])) ERR;
        if (nc_put_var_ushort(ncid, varid2, &data_out[0][0][0])) ERR;
        if (nc_close(ncid)) ERR;

        if (storage_ok(RANK3_FILE_NAME, "radiance1", NT * NX * NY * sizeof(unsigned short))) ERR;
        if (storage_ok(RANK3_FILE_NAME, "radiance4", NT * NX * NY * sizeof(unsigned short))) ERR;

        {
            unsigned short data_in[NT][NY][NX];

            if (nc_open(RANK3_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_varid(ncid, "radiance1", &varid1)) ERR;
            if (nc_inq_varid(ncid, "radiance4", &varid2)) ERR;
            if (nc_get_var_ushort(ncid, varid1, &data_in[0][0][0])) ERR;
            for (t = 0; t < NT; t++)
                for (y = 0; y < NY; y++)
                    for (x = 0; x < NX; x++)
                        if (data_in[t][y][x] != data_out[t][y][x]) ERR;
            if (nc_get_var_ushort(ncid, varid2, &data_in[0][0][0])) ERR;
            for (t = 0; t < NT; t++)
                for (y = 0; y < NY; y++)
                    for (x = 0; x < NX; x++)
                        if (data_in[t][y][x] != data_out[t][y][x]) ERR;
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking jpegls filter after shuffle...");
    {
        int ncid;
        int dimid[2];
        int varid;
        unsigned short data_out[NY][NX];
        int x, y;
        int has = 0, near = -1;

        /* Create some data to write. */
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data_out[y][x] = (x * 7 + y * 3) & 0x0fff;

        if (nc_create(SHUFFLE_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_USHORT, 2, dimid, &varid)) ERR;

        /* Turn on shuffle and set up compression. */
        if (nc_def_var_deflate(ncid, varid, 1, 0, 0)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0)) ERR;

        /* JPEG-LS is found behind the shuffle. */
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 0) ERR;

        if (nc_put_var_ushort(ncid, varid, &data_out[0][0])) ERR;
        if (nc_close(ncid)) ERR;

        {
            unsigned short data_in[NY][NX];

            /* Reopen the file and check. */
            if (nc_open(SHUFFLE_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
            if (has != 1 || near != 0) ERR;
            if (nc_get_var_ushort(ncid, varid, &data_in[0][0])) ERR;
            for (y = 0; y < NY; y++)
                for (x = 0; x < NX; x++)
                    if (data_in[y][x] != data_out[y][x]) ERR;
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking jpegls rejected arguments...");
    {
        int ncid;
        int dimid[4];
        int y1_dimid, x70k_dimid;
        int varid;
        int has = -1;
        size_t chunks[3];

        if (nc_create(REJECT_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;
        if (nc_def_dim(ncid, "z", 2, &dimid[2])) ERR;
        if (nc_def_dim(ncid, "w", 2, &dimid[3])) ERR;
        if (nc_def_dim(ncid, "y1", 1, &y1_dimid)) ERR;
        if (nc_def_dim(ncid, "x70k", 70000, &x70k_dimid)) ERR;

        /* Unsupported types give NC_EBADTYPE. */
        if (nc_def_var(ncid, "v_int", NC_INT, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0) != NC_EBADTYPE) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;
        if (nc_def_var(ncid, "v_float", NC_FLOAT, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0) != NC_EBADTYPE) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;
        if (nc_def_var(ncid, "v_char", NC_CHAR, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0) != NC_EBADTYPE) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;

        /* Rank 1 and rank 4 give NC_EINVAL. */
        if (nc_def_var(ncid, "v_rank1", NC_USHORT, 1, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0) != NC_EINVAL) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;
        if (nc_def_var(ncid, "v_rank4", NC_USHORT, 4, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0) != NC_EINVAL) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;

        /* A near outside the type's range gives NC_EINVAL. */
        if (nc_def_var(ncid, "v_near_neg", NC_USHORT, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, -1) != NC_EINVAL) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;
        if (nc_def_var(ncid, "v_near_128", NC_UBYTE, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 128) != NC_EINVAL) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;
        if (nc_def_var(ncid, "v_near_256", NC_USHORT, 2, dimid, &varid)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 256) != NC_EINVAL) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;

        /* Rank 3 with more than 4 components per chunk gives
         * NC_EINVAL. */
        {
            int dimid3[3];
            int t5_dimid;

            if (nc_def_dim(ncid, "t5", 5, &t5_dimid)) ERR;
            dimid3[0] = t5_dimid;
            dimid3[1] = dimid[0];
            dimid3[2] = dimid[1];
            if (nc_def_var(ncid, "v_comp5", NC_USHORT, 3, dimid3, &varid)) ERR;
            chunks[0] = 5;
            chunks[1] = NY;
            chunks[2] = NX;
            if (nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunks)) ERR;
            if (nc_def_var_jpegls(ncid, varid, 0) != NC_EINVAL) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
            if (has) ERR;
        }

        /* A chunk with fewer than 16 pixels gives NC_EINVAL. */
        if (nc_def_var(ncid, "v_tiny", NC_USHORT, 2, dimid, &varid)) ERR;
        chunks[0] = 3;
        chunks[1] = 5;
        if (nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunks)) ERR;
        if (nc_def_var_jpegls(ncid, varid, 0) != NC_EINVAL) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
        if (has) ERR;

        /* A chunk line or sample over 65535 gives NC_EINVAL. */
        {
            int dimid2[2];

            dimid2[0] = y1_dimid;
            dimid2[1] = x70k_dimid;
            if (nc_def_var(ncid, "v_wide", NC_USHORT, 2, dimid2, &varid)) ERR;
            chunks[0] = 1;
            chunks[1] = 70000;
            if (nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunks)) ERR;
            if (nc_def_var_jpegls(ncid, varid, 0) != NC_EINVAL) ERR;
            if (nc_inq_var_jpegls(ncid, varid, &has, NULL)) ERR;
            if (has) ERR;
        }

        if (nc_abort(ncid)) ERR;
    }
    SUMMARIZE_ERR;
    printf("*** Checking jpegls with generic parameter counts...");
    {
        int ncid;
        int dimid[2];
        int varid;
        int has = -1, near = -1;
        unsigned int p[3] = {0, 0, 0};

        if (nc_create(PARAMS_FILE_NAME, NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[0])) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[1])) ERR;

        /* Zero parameters define a valid, lossless filter. */
        if (nc_def_var(ncid, "v_noparams", NC_USHORT, 2, dimid, &varid)) ERR;
        if (nc_def_var_filter(ncid, varid, JPEGLS_ID, 0, NULL)) ERR;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near)) ERR;
        if (has != 1 || near != 0) ERR;

        /* Any other parameter count gives NC_EFILTER and leaves near
         * untouched. */
        if (nc_def_var(ncid, "v_3params", NC_USHORT, 2, dimid, &varid)) ERR;
        if (nc_def_var_filter(ncid, varid, JPEGLS_ID, 3, p)) ERR;
        near = -1;
        if (nc_inq_var_jpegls(ncid, varid, &has, &near) != NC_EFILTER) ERR;
        if (has != 1 || near != -1) ERR;

        if (nc_abort(ncid)) ERR;
    }
    SUMMARIZE_ERR;
    FINAL_RESULTS;
}
