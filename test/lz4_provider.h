/*
 * Check which LZ4 HDF5 plugin a test process has loaded.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Date: Oct 25, 2026
 * Copyright: Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _LZ4_PROVIDER_H
/* Include guard for lz4_provider.h. */
#define _LZ4_PROVIDER_H

/*
 * Check that exactly one libh5lz4 plugin is loaded and that it comes
 * from the expected directory. Returns 0 on success, 1 otherwise.
 */
int nep_check_lz4_provider(const char *expected_dir);

#endif
