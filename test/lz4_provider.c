/*
 * Check which LZ4 HDF5 plugin a test process has loaded, so tests
 * detect a second or wrong provider of filter ID 32004 on
 * HDF5_PLUGIN_PATH.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Date: Oct 25, 2026
 * Copyright: Intelligent Data Design, Inc. All rights reserved.
 */
#define _GNU_SOURCE
#include "lz4_provider.h"
#include <link.h>
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__

/* Maximum number of loaded objects recorded. */
#define MAX_OBJECTS 16

/* Loaded object paths whose basename starts with libh5lz4. */
static char loaded[MAX_OBJECTS][PATH_MAX];
/* Number of entries in loaded. */
static int nloaded;

/*
 * Record the path of each loaded object whose basename starts with
 * libh5lz4.
 *
 * info - dl_iterate_phdr callback info for one loaded object.
 * size - Size of the info struct, unused.
 * data - Callback data, unused.
 * Returns 0 to continue the iteration.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
record_lz4(struct dl_phdr_info *info, size_t size, void *data)
{
    char path[PATH_MAX];
    const char *base;

    (void)size;
    (void)data;
    if (!info->dlpi_name || !info->dlpi_name[0])
        return 0;
    snprintf(path, sizeof(path), "%s", info->dlpi_name);
    base = basename(path);
    if (strncmp(base, "libh5lz4", 8) == 0 && nloaded < MAX_OBJECTS)
        snprintf(loaded[nloaded++], PATH_MAX, "%s", info->dlpi_name);
    return 0;
}

/*
 * Check that exactly one libh5lz4 plugin is loaded and that its
 * directory is the expected one.
 *
 * expected_dir - Directory the LZ4 plugin must be loaded from.
 * Returns 0 when the single loaded libh5lz4 comes from expected_dir,
 * 1 otherwise.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nep_check_lz4_provider(const char *expected_dir)
{
    char dir[PATH_MAX];
    char actual[PATH_MAX], expected[PATH_MAX];
    int i;

    nloaded = 0;
    dl_iterate_phdr(record_lz4, NULL);
    if (nloaded == 1)
    {
        snprintf(dir, sizeof(dir), "%s", loaded[0]);
        if (realpath(dirname(dir), actual) &&
            realpath(expected_dir, expected) &&
            strcmp(actual, expected) == 0)
            return 0;
    }
    fprintf(stderr, "unexpected LZ4 provider:");
    if (!nloaded)
        fprintf(stderr, " none");
    for (i = 0; i < nloaded; i++)
        fprintf(stderr, " %s", loaded[i]);
    fprintf(stderr, "\n");
    return 1;
}

#else

/*
 * Check the loaded LZ4 plugin. Only implemented on Linux; elsewhere
 * this always succeeds.
 *
 * expected_dir - Unused.
 * Returns 0.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nep_check_lz4_provider(const char *expected_dir)
{
    (void)expected_dir;
    return 0;
}
#endif
