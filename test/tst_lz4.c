/*
 * Test LZ4 compression.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Date: Nov 13, 2025
 * Copyright: Intelligent Data Design, Inc. All rights reserved.
 */

#include "nep_compress.h"
#include "nep_test.h"
#include "lz4_provider.h"
#include <hdf5.h>
#include <H5DSpublic.h>
#include <netcdf.h>

/* Name of the test NetCDF file. */
#define FILE_NAME "tst_lz4.nc"
/* Name of this test program, used to build file names. */
#define TEST "tst_lz4"
/* Length of string buffers used for file names. */
#define STR_LEN 255
/* Maximum buffer length used in the test. */
#define MAX_LEN 1024
/* Name of the X dimension. */
#define X_NAME "X"
/* Name of the Y dimension. */
#define Y_NAME "Y"
/* Rank (2) of the test variable. */
#define NDIM2 2
/* Number of elements in a small test array. */
#define NUM_ELEMENTS 6
/* Maximum length of names used in the test. */
#define MAX_NAME_LEN 50
/* Name of the elements attribute. */
#define ELEMENTS_NAME "Elements"
/* Name of the test variable. */
#define VAR_NAME "Wacky_Woolies"
/* Number of dimensions in the test variable. */
#define NDIMS 2
/* Size of the X dimension of the test variable. */
#define NX 60
/* Size of the Y dimension of the test variable. */
#define NY 120
/* Deflate level used for comparisons. */
#define DEFLATE_LEVEL 3
/* Name of the simple test dataset. */
#define SIMPLE_VAR_NAME "data"

/* Number of test files. */
#define NFILE 2

/* Size of the X dimension of the large test variable. */
#define NX_BIG 100
/* Size of the Y dimension of the large test variable. */
#define NY_BIG 100

/* Err is used to keep track of errors within each set of tests,
 * total_err is the number of errors in the entire test program, which
 * generally cosists of several sets of tests. */
static int total_err = 0, err = 0; /* Errors in this set of tests. */

/*
 * Run the test program.
 *
 * Returns 0 on success, 2 if any test failed.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
int
main()
{
    printf("\n*** Checking LZ4 filter.\n");
    printf("*** Checking LZ4 compression...");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int data_out[NX][NY];
        int x, y;
        int level_in, lz4;

        /* Create some data to write. */
        for (x = 0; x < NX; x++)
            for (y = 0; y < NY; y++)
                data_out[x][y] = x * NY + y;

        /* Create file. */
        if (nc_create(FILE_NAME, NC_NETCDF4, &ncid)) ERR;

        /* Create dims. */
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[0])) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[1])) ERR;

        /* Create the variable. */
        if (nc_def_var(ncid, VAR_NAME, NC_INT, NDIM2, dimid, &varid)) ERR;

        /* These won't work. */
        if (nc_def_var_lz4(ncid, varid, -9) != NC_EINVAL) ERR;
        if (nc_def_var_lz4(ncid, varid, 0) != NC_EINVAL) ERR;
        if (nc_def_var_lz4(ncid, varid, 10) != NC_EINVAL) ERR;

        /* Check setting. */
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
        if (lz4) ERR;

        /* Set up compression. */
        if (nc_def_var_lz4(ncid, varid, 3)) ERR;

        /* The build-tree plugin must be the loaded LZ4 provider. */
        if (nep_check_lz4_provider(NEP_LZ4_PLUGIN_DIR)) ERR;

        /* Check setting. */
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
        if (!lz4 || level_in != 3) ERR;
        level_in = 0;
        lz4 = 1;
        if (nc_inq_var_lz4(ncid, varid, NULL, &level_in)) ERR;
        if (nc_inq_var_lz4(ncid, varid, &lz4, NULL)) ERR;
        if (!lz4 || level_in != 3) ERR;
        if (nc_inq_var_lz4(ncid, varid, NULL, NULL)) ERR;

        /* Write the data. */
        if (nc_put_var(ncid, varid, data_out)) ERR;

        /* Close the file. */
        if (nc_close(ncid)) ERR;

        {
            int data_in[NX][NY];

            /* Now reopen the file and check. */
            if (nc_open(FILE_NAME, NC_NETCDF4, &ncid)) ERR;

            /* Check setting. */
            if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
            if (!lz4 || level_in != 3) ERR;

            /* Read the data. */
            if (nc_get_var(ncid, varid, data_in)) ERR;

            /* Check the data. */
            for (x = 0; x < NX; x++)
               for (y = 0; y < NY; y++)
                  if (data_in[x][y] != data_out[x][y]) ERR;

            /* Close the file. */
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking LZ4 size of compression...");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int *data_out;
        int *data_in;
        int x, f;
        int level_in, lz4;

        if (!(data_out = malloc(NX_BIG * NY_BIG * sizeof(int)))) ERR;
        if (!(data_in = malloc(NX_BIG * NY_BIG * sizeof(int)))) ERR;

        /* Create some data to write. */
        for (x = 0; x < NX_BIG * NY_BIG; x++)
            data_out[x] = x * NY_BIG + x % NX_BIG;

        for (f = 0; f < NFILE; f++)
        {
            char file_name[STR_LEN + 1];

            sprintf(file_name, "%s_%s.nc", TEST, (f ? "lz4" : "uncompressed"));
            /* nc_set_log_level(3); */

            /* Create file. */
            if (nc_create(file_name, NC_NETCDF4, &ncid)) ERR;
            if (nc_def_dim(ncid, X_NAME, NX_BIG, &dimid[0])) ERR;
            if (nc_def_dim(ncid, Y_NAME, NY_BIG, &dimid[1])) ERR;
            if (nc_def_var(ncid, VAR_NAME, NC_INT, NDIM2, dimid, &varid)) ERR;
            if (f)
                if (nc_def_var_lz4(ncid, varid, 3)) ERR;
            if (nc_put_var(ncid, varid, data_out)) ERR;
            if (nc_close(ncid)) ERR;

            /* Check file. */
            {
                if (nc_open(file_name, NC_NETCDF4, &ncid)) ERR;
                if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
                if (f)
                {
                    if (!lz4 || level_in != 3) ERR;
                }
                else
                {
                    if (lz4) ERR;
                }
                if (nc_get_var(ncid, varid, data_in)) ERR;
                for (x = 0; x < NX_BIG * NY_BIG; x++)
                    if (data_in[x] != data_out[x]) ERR;
                if (nc_close(ncid)) ERR;
            }
        } /* next file */

        free(data_out);
        free(data_in);
    }
    SUMMARIZE_ERR;
    FINAL_RESULTS;
}
