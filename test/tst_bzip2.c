/**
 * @file
 * Test the bzip2 compression in nep.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "nep_compress.h"
#include "nep_test.h"
#include <hdf5.h>
#include <H5DSpublic.h>
#include <netcdf.h>
#include <math.h> /* Define fabs(), powf(), round() */

/** Test file name. */
#define FILE_NAME "tst_bzip2.nc"
/** Length of string buffers used for file names. */
#define STR_LEN 255
/** Maximum buffer length used in the test. */
#define MAX_LEN 1024
/** Name of the X dimension. */
#define X_NAME "X"
/** Name of the Y dimension. */
#define Y_NAME "Y"
/** Rank (2) of the test variable. */
#define NDIM2 2
/** Number of elements in a small test array. */
#define NUM_ELEMENTS 6
/** Maximum length of names used in the test. */
#define MAX_NAME_LEN 50
/** Name of the elements attribute. */
#define ELEMENTS_NAME "Elements"
/** Name of the test variable. */
#define VAR_NAME "Wacky_Woolies"
/** Number of dimensions in the test variable. */
#define NDIMS 2
/** Size of the X dimension of the test variable. */
#define NX 60
/** Size of the Y dimension of the test variable. */
#define NY 120
/** Deflate level used for comparisons. */
#define DEFLATE_LEVEL 3
/** Name of the simple test dataset. */
#define SIMPLE_VAR_NAME "data"

/** Err is used to keep track of errors within each set of tests,
 * total_err is the number of errors in the entire test program, which
 * generally cosists of several sets of tests. */
static int total_err = 0, err = 0; /**< Errors in this set of tests. */

/**
 * @brief Run the test program.
 *
 * @return 0 on success, 2 if any test failed.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
main()
{
    printf("\n*** Checking HDF5 Bzip2 compression.\n");
    printf("*** Checking simple bzip2 filter...");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int data_out[NX][NY];
        int x, y;
        int level_in, bzip2;

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
        if (nc_def_var_bzip2(ncid, varid, -9) != NC_EINVAL) ERR;
        if (nc_def_var_bzip2(ncid, varid, 0) != NC_EINVAL) ERR;
        if (nc_def_var_bzip2(ncid, varid, 10) != NC_EINVAL) ERR;

        /* Check setting. */
        if (nc_inq_var_bzip2(ncid, varid, &bzip2, &level_in)) ERR;
        if (bzip2) ERR;

        /* Set up compression. */
        if (nc_def_var_bzip2(ncid, varid, 8)) ERR;

        /* Check setting. */
        if (nc_inq_var_bzip2(ncid, varid, &bzip2, &level_in)) ERR;
        if (!bzip2 || level_in != 8) ERR;
        level_in = 0;
        bzip2 = 1;
        if (nc_inq_var_bzip2(ncid, varid, NULL, &level_in)) ERR;
        if (nc_inq_var_bzip2(ncid, varid, &bzip2, NULL)) ERR;
        if (!bzip2 || level_in != 8) ERR;
        if (nc_inq_var_bzip2(ncid, varid, NULL, NULL)) ERR;

        /* Write the data. */
        if (nc_put_var(ncid, varid, data_out)) ERR;

        /* Close the file. */
        if (nc_close(ncid)) ERR;

        {
            int data_in[NX][NY];

            /* Now reopen the file and check. */
            if (nc_open(FILE_NAME, NC_NETCDF4, &ncid)) ERR;

            /* Check setting. */
            if (nc_inq_var_bzip2(ncid, varid, &bzip2, &level_in)) ERR;
            if (!bzip2 || level_in != 8) ERR;

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
    FINAL_RESULTS;
}
