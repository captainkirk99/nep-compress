/**
 * @file compare_filters.c
 * @brief Measure compression ratio and I/O throughput of every lossless
 *        NetCDF-4 filter on real GDAS surface data.
 *
 * This program reads four full-grid fields from the committed sample
 * test/data/gdas_sfc_sample.nc (tmp2m, prate_ave, soilw1 and land, each
 * 1 x 768 x 1536 NC_FLOAT) and rewrites each one with every lossless
 * filter combination available to NetCDF-4: none, deflate levels
 * 1/5/9, szip (NC_SZIP_NN, 32 pixels per block), zstandard levels
 * 1/3/9/19, bzip2 levels 1/9 and LZ4 levels 1/9 — each with the HDF5
 * shuffle filter off and on. Filters whose plugin is not available are
 * skipped and reported, not failed.
 *
 * Write time covers nc_create() through nc_close(), including the
 * defines, nc_enddef() and nc_put_var_float(). Read time covers
 * nc_open() through nc_close(), including nc_inq_varid() and
 * nc_get_var_float(). Each configuration is run @ref NREPS times and
 * the minimum is reported; bit-exact round-trip and filter-ID
 * verification happen outside the timed regions. The reported file
 * size is the whole file from stat(), so it includes a few KB of HDF5
 * metadata — the "none" row shows that overhead.
 *
 * **Output:** a CSV file with columns:
 *   @code
 *   field,filter,filter_id,level,shuffle,uncompressed_bytes,file_bytes,ratio,write_s,read_s,write_MBps,read_MBps
 *   @endcode
 * and a readable table on stdout, one block per field.
 *
 * **Typical workflow:**
 * @code
 *   ctest -R perf_compare_filters --output-on-failure
 *   # or by hand:
 *   HDF5_PLUGIN_PATH=<plugin dirs> ./compare_filters \
 *       test/data/gdas_sfc_sample.nc compare_filters.csv
 * @endcode
 *
 * **Learning Objectives:**
 * - Compare all lossless NetCDF-4 filters on the same real data
 * - See how the shuffle filter changes each compressor's ratio
 * - Learn to detect filter availability at run time with
 *   nc_inq_filter_avail()
 * - Learn to verify a variable's filter pipeline with
 *   nc_inq_var_filter_ids()
 *
 * **Key Concepts:**
 * - **Shuffle**: reorders bytes so significant bytes of adjacent values
 *   group together; usually improves every compressor's ratio at little
 *   cost. It is enabled through the shuffle argument of
 *   nc_def_var_deflate().
 * - **Filter availability and plugins**: filters other than deflate and
 *   szip are HDF5 plugins found on HDF5_PLUGIN_PATH at run time; the
 *   program probes each one before running it.
 * - **Real vs synthetic data**: real fields have spatial correlation,
 *   missing-value regions and zero-dominated areas that synthetic data
 *   lacks, so ratios differ from synthetic benchmarks.
 * - **Fill-value-heavy fields**: soilw1 (ocean = fill) and land
 *   (categorical) compress far better than the physical fields.
 *
 * **Key API functions:**
 * - nc_inq_filter_avail()    Check whether a filter ID is usable
 * - nc_def_var_deflate()     Set deflate level and/or the shuffle filter
 * - nc_def_var_szip()        Set szip compression
 * - nc_def_var_filter()      Set zstandard via the generic filter API
 * - nc_def_var_bzip2()       Set bzip2 (NetCDF-C function)
 * - nc_def_var_lz4()         Set LZ4 (nep-compress function)
 * - nc_inq_var_filter_ids()  List a variable's filter pipeline
 * - nc_put_var_float()       Write a whole variable
 * - nc_get_var_float()       Read a whole variable
 *
 * **Related Examples:**
 * - examples/performance/lz4.c   LZ4 levels on synthetic data
 * - examples/performance/bzip2.c BZIP2 levels on synthetic data
 *
 * @note Companion code for "The NetCDF Developer's Handbook: The
 * Authoritative Guide to Writing High-Performance Programs for
 * Scientific Data Management, Second Edition"
 * (https://www.amazon.com/dp/B0H7Q49MPS)
 *
 * @note This program runs as the ctest perf_compare_filters when
 * NEPCOMPRESS_ZSTD_PLUGIN_DIR is set at configure time.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

/** Enable POSIX clock_gettime(). */
#define _POSIX_C_SOURCE 200809L

#include <netcdf.h>
#include <netcdf_filter.h>
#include "nep_compress.h"
#ifdef NEPCOMPRESS_BUILD_LZ4
#include "lz4_provider.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/** Number of fields read from the sample file. */
#define NFIELDS 4
/** Grid Y dimension of each field. */
#define NY 768
/** Grid X dimension of each field. */
#define NX 1536
/** Number of float values in one field. */
#define NVALS ((size_t)NY * NX)
/** Uncompressed size of one field in bytes. */
#define FIELD_BYTES ((double)NVALS * sizeof(float))
/** Repetitions per configuration; the minimum time is reported. */
#define NREPS 3
/** Temporary file written and read for each measurement. */
#define TMP_FILE "perf_tmp.nc"
/** Szip pixels per block. */
#define SZIP_PPB 32

/** Names of the fields in the sample file. */
static const char *FIELDS[NFIELDS] = {"tmp2m", "prate_ave", "soilw1", "land"};

/** One filter configuration. */
typedef struct filter_config {
    const char *name;   /**< "none", "deflate", "szip", "zstd", "bzip2", "lz4" */
    unsigned int id;    /**< 0 for none, else HDF5 filter ID */
    int level;          /**< filter level; pixels per block for szip; 0 for none */
} filter_config;

/** The filter set: every lossless filter, each run with shuffle off and on. */
static const filter_config CONFIGS[] = {
    {"none", 0, 0},
    {"deflate", H5Z_FILTER_DEFLATE, 1}, {"deflate", H5Z_FILTER_DEFLATE, 5},
    {"deflate", H5Z_FILTER_DEFLATE, 9},
    {"szip", H5Z_FILTER_SZIP, SZIP_PPB},
    {"zstd", H5Z_FILTER_ZSTD, 1}, {"zstd", H5Z_FILTER_ZSTD, 3},
    {"zstd", H5Z_FILTER_ZSTD, 9}, {"zstd", H5Z_FILTER_ZSTD, 19},
    {"bzip2", H5Z_FILTER_BZIP2, 1}, {"bzip2", H5Z_FILTER_BZIP2, 9},
#ifdef NEPCOMPRESS_BUILD_LZ4
    {"lz4", LZ4_ID, 1}, {"lz4", LZ4_ID, 9},
#endif
};

/** Number of entries in CONFIGS. */
#define NCONFIGS (sizeof(CONFIGS) / sizeof(CONFIGS[0]))

/**
 * @brief Return current wall-clock time in seconds.
 * @return Seconds from CLOCK_MONOTONIC as a double.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static double
get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/**
 * @brief Read all four fields from the sample file.
 *
 * Opens @p path with NC_NOWRITE and reads each field in @ref FIELDS
 * with nc_get_var_float() into a newly malloc'ed buffer of @ref NVALS
 * floats. Each field must have 3 dimensions of lengths 1, @ref NY and
 * @ref NX.
 *
 * @param path Path to the sample file.
 * @param data Array of @ref NFIELDS buffer pointers to fill.
 * @return 0 on success, 1 on failure.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
read_sample(const char *path, float *data[NFIELDS])
{
    int ncid, varid, ndims, dimids[NC_MAX_VAR_DIMS], ret, f;
    size_t len;
    int d;

    if ((ret = nc_open(path, NC_NOWRITE, &ncid)))
    {
        if (ret == NC_ENOFILTER)
            fprintf(stderr, "cannot read %s: zstd (32015) plugin not on HDF5_PLUGIN_PATH\n",
                    path);
        else
            fprintf(stderr, "nc_open %s: %s\n", path, nc_strerror(ret));
        return 1;
    }
    for (f = 0; f < NFIELDS; f++)
    {
        if ((ret = nc_inq_varid(ncid, FIELDS[f], &varid)) ||
            (ret = nc_inq_varndims(ncid, varid, &ndims)) ||
            (ret = nc_inq_vardimid(ncid, varid, dimids)))
        {
            fprintf(stderr, "%s: %s\n", FIELDS[f], nc_strerror(ret));
            nc_close(ncid);
            return 1;
        }
        if (ndims != 3)
        {
            fprintf(stderr, "%s: expected 3 dims, got %d\n", FIELDS[f], ndims);
            nc_close(ncid);
            return 1;
        }
        for (d = 0; d < ndims; d++)
        {
            size_t expected = d == 0 ? 1 : (d == 1 ? NY : NX);
            if ((ret = nc_inq_dimlen(ncid, dimids[d], &len)) || len != expected)
            {
                fprintf(stderr, "%s: dim %d length %zu, expected %zu\n",
                        FIELDS[f], d, len, expected);
                nc_close(ncid);
                return 1;
            }
        }
        data[f] = (float *)malloc(NVALS * sizeof(float));
        if (!data[f])
        {
            fprintf(stderr, "malloc failed for %s\n", FIELDS[f]);
            nc_close(ncid);
            return 1;
        }
        if ((ret = nc_get_var_float(ncid, varid, data[f])))
        {
            if (ret == NC_ENOFILTER || ret == NC_EFILTER)
                fprintf(stderr, "cannot read %s: zstd (32015) plugin not on HDF5_PLUGIN_PATH\n",
                        path);
            else
                fprintf(stderr, "nc_get_var_float %s: %s\n", FIELDS[f], nc_strerror(ret));
            nc_close(ncid);
            return 1;
        }
    }
    nc_close(ncid);
    return 0;
}

/**
 * @brief Check whether one filter ID is usable.
 *
 * Deflate and "none" are always available. For every other filter a
 * scratch file is created and nc_inq_filter_avail() is called on it;
 * the file is removed afterwards.
 *
 * @param id HDF5 filter ID, or 0 for no filter.
 * @return 1 if the filter is available, 0 otherwise.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
filter_available(unsigned int id)
{
    int ncid, ret;

    if (id == 0 || id == H5Z_FILTER_DEFLATE)
        return 1;
    if ((ret = nc_create(TMP_FILE, NC_NETCDF4 | NC_CLOBBER, &ncid)))
        return 0;
    ret = nc_inq_filter_avail(ncid, id);
    nc_close(ncid);
    remove(TMP_FILE);
    return ret == NC_NOERR;
}

/**
 * @brief Define one configuration's filters on a variable.
 *
 * Shuffle, when requested, is always the first filter in the pipeline;
 * it is enabled with nc_def_var_deflate(ncid, varid, 1, 0, 0). zstd is
 * set with the generic nc_def_var_filter() on purpose, because
 * nc_def_var_zstandard() silently does nothing when NetCDF-C lacks
 * zstd.
 *
 * @param ncid    Open file ID, in define mode.
 * @param varid   Variable to filter.
 * @param c       Filter configuration.
 * @param shuffle 1 to prepend the shuffle filter, 0 otherwise.
 * @return NetCDF error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
define_filters(int ncid, int varid, const filter_config *c, int shuffle)
{
    if (c->id == 0)
    {
        if (shuffle)
            return nc_def_var_deflate(ncid, varid, 1, 0, 0);
        return NC_NOERR;
    }
    if (c->id == H5Z_FILTER_DEFLATE)
        return nc_def_var_deflate(ncid, varid, shuffle, 1, c->level);
    if (shuffle)
    {
        int ret = nc_def_var_deflate(ncid, varid, 1, 0, 0);
        if (ret)
            return ret;
    }
    switch (c->id)
    {
    case H5Z_FILTER_SZIP:
        return nc_def_var_szip(ncid, varid, NC_SZIP_NN, c->level);
    case H5Z_FILTER_ZSTD:
    {
        unsigned int p = (unsigned int)c->level;
        return nc_def_var_filter(ncid, varid, H5Z_FILTER_ZSTD, 1, &p);
    }
    case H5Z_FILTER_BZIP2:
        return nc_def_var_bzip2(ncid, varid, c->level);
#ifdef NEPCOMPRESS_BUILD_LZ4
    case LZ4_ID:
        return nc_def_var_lz4(ncid, varid, c->level);
#endif
    default:
        return NC_EINVAL;
    }
}

/**
 * @brief Timed write of one field to @ref TMP_FILE.
 *
 * The timed region runs from before nc_create() until after
 * nc_close(). The file has fixed dimensions time = 1, grid_yt =
 * @ref NY, grid_xt = @ref NX, one NC_FLOAT variable "data" chunked
 * {1, @ref NY, @ref NX} like the source file, with fill turned off and
 * the requested filters defined.
 *
 * @param data    Field data, @ref NVALS floats.
 * @param c       Filter configuration.
 * @param shuffle 1 to enable the shuffle filter, 0 otherwise.
 * @param secs    Returns the elapsed write time in seconds.
 * @return 0 on success, 1 on any error.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
write_file(const float *data, const filter_config *c, int shuffle, double *secs)
{
    int ncid, varid, dimids[3], ret;
    size_t chunksizes[3] = {1, NY, NX};
    double t0;

    t0 = get_time();
    if ((ret = nc_create(TMP_FILE, NC_NETCDF4 | NC_CLOBBER, &ncid)))
        goto err;
    if ((ret = nc_def_dim(ncid, "time", 1, &dimids[0])) ||
        (ret = nc_def_dim(ncid, "grid_yt", NY, &dimids[1])) ||
        (ret = nc_def_dim(ncid, "grid_xt", NX, &dimids[2])) ||
        (ret = nc_def_var(ncid, "data", NC_FLOAT, 3, dimids, &varid)) ||
        (ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes)) ||
        (ret = nc_def_var_fill(ncid, varid, NC_NOFILL, NULL)) ||
        (ret = define_filters(ncid, varid, c, shuffle)) ||
        (ret = nc_enddef(ncid)) ||
        (ret = nc_put_var_float(ncid, varid, data)) ||
        (ret = nc_close(ncid)))
        goto err;
    *secs = get_time() - t0;
    return 0;
err:
    fprintf(stderr, "write_file: %s\n", nc_strerror(ret));
    return 1;
}

/**
 * @brief Timed read of @ref TMP_FILE into a buffer.
 *
 * The timed region runs from before nc_open() until after nc_close()
 * and includes nc_inq_varid() and nc_get_var_float().
 *
 * @param out  Buffer of @ref NVALS floats to fill.
 * @param secs Returns the elapsed read time in seconds.
 * @return 0 on success, 1 on any error.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
read_file(float *out, double *secs)
{
    int ncid, varid, ret;
    double t0;

    t0 = get_time();
    if ((ret = nc_open(TMP_FILE, NC_NOWRITE, &ncid)))
        goto err;
    if ((ret = nc_inq_varid(ncid, "data", &varid)) ||
        (ret = nc_get_var_float(ncid, varid, out)) ||
        (ret = nc_close(ncid)))
        goto err;
    *secs = get_time() - t0;
    return 0;
err:
    fprintf(stderr, "read_file: %s\n", nc_strerror(ret));
    return 1;
}

/**
 * @brief Untimed check that @ref TMP_FILE has the expected filter pipeline.
 *
 * Reopens the file and calls nc_inq_var_filter_ids() twice — once for
 * the count, once for the list. Expected pipelines: none = {} or {2}
 * with shuffle; deflate = {1} or {2, 1}; other filters = {id} or
 * {2, id}.
 *
 * @param c       Filter configuration that was written.
 * @param shuffle 1 if the shuffle filter was enabled.
 * @return 0 on match, 1 on mismatch or error.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
verify_file(const filter_config *c, int shuffle)
{
    int ncid, varid, ret;
    size_t nfilters, i;
    unsigned int *ids;
    unsigned int expect[2];
    size_t nexpect;
    int bad = 0;

    if ((ret = nc_open(TMP_FILE, NC_NOWRITE, &ncid)) ||
        (ret = nc_inq_varid(ncid, "data", &varid)) ||
        (ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, NULL)))
    {
        fprintf(stderr, "verify_file: %s\n", nc_strerror(ret));
        return 1;
    }
    ids = (unsigned int *)malloc((nfilters ? nfilters : 1) * sizeof(unsigned int));
    if (!ids || (ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, ids)))
    {
        fprintf(stderr, "verify_file: %s\n", ids ? nc_strerror(ret) : "malloc failed");
        free(ids);
        nc_close(ncid);
        return 1;
    }
    nc_close(ncid);

    nexpect = 0;
    if (shuffle)
        expect[nexpect++] = H5Z_FILTER_SHUFFLE;
    if (c->id)
        expect[nexpect++] = c->id;

    if (nfilters != nexpect)
        bad = 1;
    else
        for (i = 0; i < nexpect; i++)
            if (ids[i] != expect[i])
                bad = 1;
    if (bad)
    {
        fprintf(stderr, "verify_file %s level %d shuffle %d: expected filters",
                c->name, c->level, shuffle);
        for (i = 0; i < nexpect; i++)
            fprintf(stderr, " %u", expect[i]);
        fprintf(stderr, ", got");
        for (i = 0; i < nfilters; i++)
            fprintf(stderr, " %u", ids[i]);
        fprintf(stderr, "\n");
        free(ids);
        return 1;
    }
    free(ids);
    return 0;
}

/**
 * @brief Run one configuration @ref NREPS times and report the minima.
 *
 * Each repetition calls write_file(), read_file(), verify_file() and
 * memcmp() against the original field; any failure prints the field,
 * filter, level and shuffle to stderr and returns 1. After the loop the
 * file is stat'ed for @c file_bytes and removed, a CSV row is written
 * to @p csv and a table row is printed to stdout.
 *
 * On the first LZ4 run the loaded provider is checked once with
 * nep_check_lz4_provider(), after the first write_file().
 *
 * @param csv     Open CSV file for the result row.
 * @param field   Index into @ref FIELDS.
 * @param data    Original field data, @ref NVALS floats.
 * @param buf     Scratch buffer of @ref NVALS floats for reads.
 * @param c       Filter configuration.
 * @param shuffle 1 to enable the shuffle filter, 0 otherwise.
 * @return 0 on success, 1 on any error.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
run_one(FILE *csv, int field, const float *data, float *buf,
        const filter_config *c, int shuffle)
{
#ifdef NEPCOMPRESS_BUILD_LZ4
    static int lz4_checked;
#endif
    double write_min = 1e30, read_min = 1e30, secs;
    struct stat st;
    long long file_bytes;
    double ratio, wmbps, rmbps;
    int rep;

    for (rep = 0; rep < NREPS; rep++)
    {
        if (write_file(data, c, shuffle, &secs))
            goto fail;
        write_min = secs < write_min ? secs : write_min;
#ifdef NEPCOMPRESS_BUILD_LZ4
        if (c->id == LZ4_ID && !lz4_checked)
        {
            lz4_checked = 1;
            if (nep_check_lz4_provider(NEP_LZ4_PLUGIN_DIR))
                goto fail;
        }
#endif
        if (read_file(buf, &secs))
            goto fail;
        read_min = secs < read_min ? secs : read_min;
        if (verify_file(c, shuffle))
            goto fail;
        if (memcmp(data, buf, NVALS * sizeof(float)))
        {
            fprintf(stderr, "%s %s level %d shuffle %d: data mismatch\n",
                    FIELDS[field], c->name, c->level, shuffle);
            goto fail;
        }
    }
    if (stat(TMP_FILE, &st) != 0)
    {
        perror("stat");
        goto fail;
    }
    file_bytes = (long long)st.st_size;
    remove(TMP_FILE);

    ratio = FIELD_BYTES / (double)file_bytes;
    wmbps = FIELD_BYTES / 1e6 / write_min;
    rmbps = FIELD_BYTES / 1e6 / read_min;
    fprintf(csv, "%s,%s,%u,%d,%d,%lld,%lld,%.3f,%.6f,%.6f,%.3f,%.3f\n",
            FIELDS[field], c->name, c->id, c->level, shuffle,
            (long long)NVALS * (long long)sizeof(float), file_bytes,
            ratio, write_min, read_min, wmbps, rmbps);
    printf("%-8s %5d %7d %8.2f %10.1f %10.1f\n",
           c->name, c->level, shuffle, ratio, wmbps, rmbps);
    return 0;
fail:
    fprintf(stderr, "field %s filter %s level %d shuffle %d failed\n",
            FIELDS[field], c->name, c->level, shuffle);
    remove(TMP_FILE);
    return 1;
}

/**
 * @brief Main entry point.
 *
 * Usage: compare_filters \<sample.nc\> \<results.csv\>. Reads the
 * sample, probes each distinct filter ID once, then runs every field x
 * configuration x shuffle combination, writing the CSV and the table.
 *
 * @param argc Argument count.
 * @param argv sample path and CSV output path.
 * @return 0 on success, 1 on failure, 2 on bad arguments.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
main(int argc, char **argv)
{
    float *data[NFIELDS] = {NULL}, *buf = NULL;
    int avail[NCONFIGS];
    FILE *csv;
    int f, shuffle, ret = 0;
    size_t ci, cj;
    int found;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <sample.nc> <results.csv>\n", argv[0]);
        return 2;
    }
    if (read_sample(argv[1], data))
        return 1;
    buf = (float *)malloc(NVALS * sizeof(float));
    if (!buf)
    {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    /* Probe each distinct filter ID once and cache the result. */
    for (ci = 0; ci < NCONFIGS; ci++)
    {
        found = 0;
        for (cj = 0; cj < ci; cj++)
            if (CONFIGS[cj].id == CONFIGS[ci].id)
            {
                avail[ci] = avail[cj];
                found = 1;
                break;
            }
        if (!found)
        {
            avail[ci] = filter_available(CONFIGS[ci].id);
            if (!avail[ci])
                printf("# skipped %s: filter %u not available\n",
                       CONFIGS[ci].name, CONFIGS[ci].id);
        }
    }

    if (!(csv = fopen(argv[2], "w")))
    {
        fprintf(stderr, "cannot open %s\n", argv[2]);
        return 1;
    }
    fprintf(csv, "field,filter,filter_id,level,shuffle,uncompressed_bytes,"
            "file_bytes,ratio,write_s,read_s,write_MBps,read_MBps\n");

    for (f = 0; f < NFIELDS && !ret; f++)
    {
        printf("== %s ==\n", FIELDS[f]);
        printf("%-8s %5s %7s %8s %10s %10s\n",
               "filter", "level", "shuffle", "ratio", "write MB/s", "read MB/s");
        for (ci = 0; ci < NCONFIGS && !ret; ci++)
        {
            if (!avail[ci])
                continue;
            for (shuffle = 0; shuffle <= 1 && !ret; shuffle++)
                ret = run_one(csv, f, data[f], buf, &CONFIGS[ci], shuffle);
        }
    }

    free(buf);
    for (f = 0; f < NFIELDS; f++)
        free(data[f]);
    fclose(csv);
    return ret;
}
