/*
 * Test nc_inq_var_lz4 with LZ4 anywhere in a multi-filter pipeline,
 * and with an LZ4 filter that has the wrong parameter count.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Date: Oct 25, 2026
 * Copyright: Intelligent Data Design, Inc. All rights reserved.
 */

#include "nep_compress.h"
#include "nep_test.h"
#include "lz4_provider.h"
#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>

/* Name of the X dimension. */
#define X_NAME "X"
/* Name of the Y dimension. */
#define Y_NAME "Y"
/* Rank (2) of the test variable. */
#define NDIM2 2
/* Name of the test variable. */
#define VAR_NAME "Wacky_Woolies"
/* Size of the X dimension of the test variable. */
#define NX 60
/* Size of the Y dimension of the test variable. */
#define NY 120
/* Filter ID for the szip filter. */
#define SZIP_ID 4

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
    int data_out[NX][NY];
    int x, y;

    /* Create some data to write. */
    for (x = 0; x < NX; x++)
        for (y = 0; y < NY; y++)
            data_out[x][y] = x * NY + y;

    printf("\n*** Checking nc_inq_var_lz4 with other filters.\n");
    printf("*** Checking LZ4 after shuffle...");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int lz4 = 0, level_in = 0;
        int data_in[NX][NY];

        /* Create file, variable and filters. */
        if (nc_create("tst_lz4_shuffle.nc", NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[0])) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_INT, NDIM2, dimid, &varid)) ERR;
        if (nc_def_var_deflate(ncid, varid, 1, 0, 0)) ERR;
        if (nc_def_var_lz4(ncid, varid, 3)) ERR;

        /* Check in define mode; LZ4 is found behind the shuffle. */
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
        if (!lz4 || level_in != 3) ERR;
        if (nep_check_lz4_provider(NEP_LZ4_PLUGIN_DIR)) ERR;

        /* Write, close and reopen. */
        if (nc_put_var(ncid, varid, data_out)) ERR;
        if (nc_close(ncid)) ERR;
        if (nc_open("tst_lz4_shuffle.nc", NC_NETCDF4, &ncid)) ERR;
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
        if (!lz4 || level_in != 3) ERR;
        if (nc_get_var(ncid, varid, data_in)) ERR;
        for (x = 0; x < NX; x++)
            for (y = 0; y < NY; y++)
                if (data_in[x][y] != data_out[x][y]) ERR;
        if (nc_close(ncid)) ERR;
    }
    SUMMARIZE_ERR;
    printf("*** Checking LZ4 after fletcher32...");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int lz4 = 0, level_in = 0;
        int data_in[NX][NY];

        /* Create file, variable and filters. */
        if (nc_create("tst_lz4_fletcher32.nc", NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[0])) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_INT, NDIM2, dimid, &varid)) ERR;
        if (nc_def_var_fletcher32(ncid, varid, 1)) ERR;
        if (nc_def_var_lz4(ncid, varid, 3)) ERR;

        /* Check in define mode; LZ4 is found behind fletcher32. */
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
        if (!lz4 || level_in != 3) ERR;
        if (nep_check_lz4_provider(NEP_LZ4_PLUGIN_DIR)) ERR;

        /* Write, close and reopen. */
        if (nc_put_var(ncid, varid, data_out)) ERR;
        if (nc_close(ncid)) ERR;
        if (nc_open("tst_lz4_fletcher32.nc", NC_NETCDF4, &ncid)) ERR;
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
        if (!lz4 || level_in != 3) ERR;
        if (nc_get_var(ncid, varid, data_in)) ERR;
        for (x = 0; x < NX; x++)
            for (y = 0; y < NY; y++)
                if (data_in[x][y] != data_out[x][y]) ERR;
        if (nc_close(ncid)) ERR;
    }
    SUMMARIZE_ERR;
    printf("*** Checking LZ4 after deflate...");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int lz4 = 0, level_in = 0;
        int data_in[NX][NY];

        /* Create file, variable and filters. */
        if (nc_create("tst_lz4_deflate.nc", NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[0])) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_INT, NDIM2, dimid, &varid)) ERR;
        if (nc_def_var_deflate(ncid, varid, 0, 1, 1)) ERR;
        if (nc_def_var_lz4(ncid, varid, 5)) ERR;

        /* Check in define mode; LZ4 is found behind deflate. */
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
        if (!lz4 || level_in != 5) ERR;
        if (nep_check_lz4_provider(NEP_LZ4_PLUGIN_DIR)) ERR;

        /* Write, close and reopen. */
        if (nc_put_var(ncid, varid, data_out)) ERR;
        if (nc_close(ncid)) ERR;
        if (nc_open("tst_lz4_deflate.nc", NC_NETCDF4, &ncid)) ERR;
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
        if (!lz4 || level_in != 5) ERR;
        if (nc_get_var(ncid, varid, data_in)) ERR;
        for (x = 0; x < NX; x++)
            for (y = 0; y < NY; y++)
                if (data_in[x][y] != data_out[x][y]) ERR;
        if (nc_close(ncid)) ERR;
    }
    SUMMARIZE_ERR;
    printf("*** Checking LZ4 after szip...");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int lz4 = 0, level_in = 0;
        int data_in[NX][NY];
        int szip_avail;

        /* Create the file first, since nc_inq_filter_avail needs an
         * ncid. */
        if (nc_create("tst_lz4_szip.nc", NC_NETCDF4, &ncid)) ERR;
        szip_avail = nc_inq_filter_avail(ncid, SZIP_ID) == NC_NOERR;
        if (!szip_avail)
        {
            printf("skipped\n");
            if (nc_close(ncid)) ERR;
        }
        else
        {
            /* Create dims, variable and filters. */
            if (nc_def_dim(ncid, X_NAME, NX, &dimid[0])) ERR;
            if (nc_def_dim(ncid, Y_NAME, NY, &dimid[1])) ERR;
            if (nc_def_var(ncid, VAR_NAME, NC_INT, NDIM2, dimid, &varid)) ERR;
            if (nc_def_var_szip(ncid, varid, NC_SZIP_NN, 32)) ERR;
            if (nc_def_var_lz4(ncid, varid, 3)) ERR;

            /* Check in define mode; LZ4 is found behind szip. */
            if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
            if (!lz4 || level_in != 3) ERR;
            if (nep_check_lz4_provider(NEP_LZ4_PLUGIN_DIR)) ERR;

            /* Write, close and reopen. */
            if (nc_put_var(ncid, varid, data_out)) ERR;
            if (nc_close(ncid)) ERR;
            if (nc_open("tst_lz4_szip.nc", NC_NETCDF4, &ncid)) ERR;
            if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in)) ERR;
            if (!lz4 || level_in != 3) ERR;
            if (nc_get_var(ncid, varid, data_in)) ERR;
            for (x = 0; x < NX; x++)
                for (y = 0; y < NY; y++)
                    if (data_in[x][y] != data_out[x][y]) ERR;
            if (nc_close(ncid)) ERR;
        }
    }
    SUMMARIZE_ERR;
    printf("*** Checking LZ4 with a bad parameter count...");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int lz4 = 0, level_in = -1;
        int data_in[NX][NY];
        unsigned int p[3] = {3, 0, 0};

        /* Create file and variable; define LZ4 directly with three
         * parameters instead of one. */
        if (nc_create("tst_lz4_badparams.nc", NC_NETCDF4, &ncid)) ERR;
        if (nc_def_dim(ncid, X_NAME, NX, &dimid[0])) ERR;
        if (nc_def_dim(ncid, Y_NAME, NY, &dimid[1])) ERR;
        if (nc_def_var(ncid, VAR_NAME, NC_INT, NDIM2, dimid, &varid)) ERR;
        if (nc_def_var_filter(ncid, varid, LZ4_ID, 3, p)) ERR;

        /* In define mode the inquiry reports NC_EFILTER, sets lz4 to
         * 1 and leaves the level untouched. */
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in) != NC_EFILTER) ERR;
        if (!lz4 || level_in != -1) ERR;
        if (nep_check_lz4_provider(NEP_LZ4_PLUGIN_DIR)) ERR;

        /* Write, close and reopen; the plugin only reads the first
         * parameter, so the data round-trips. */
        if (nc_put_var(ncid, varid, data_out)) ERR;
        if (nc_close(ncid)) ERR;
        if (nc_open("tst_lz4_badparams.nc", NC_NETCDF4, &ncid)) ERR;
        lz4 = 0;
        if (nc_inq_var_lz4(ncid, varid, &lz4, &level_in) != NC_EFILTER) ERR;
        if (!lz4 || level_in != -1) ERR;
        if (nc_get_var(ncid, varid, data_in)) ERR;
        for (x = 0; x < NX; x++)
            for (y = 0; y < NY; y++)
                if (data_in[x][y] != data_out[x][y]) ERR;
        if (nc_close(ncid)) ERR;
    }
    SUMMARIZE_ERR;
    FINAL_RESULTS;
}
