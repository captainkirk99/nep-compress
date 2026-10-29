/*
 * Downstream-consumer smoke test for the installed C library.
 *
 * Confirms that nep_compress.h can be included and that the filter ID
 * macros are visible, including JPEGLS_ID.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <nep_compress.h>

/*
 * Check the filter ID macros.
 *
 * Returns 0 when LZ4_ID, BZIP2_ID and JPEGLS_ID have their expected
 * values.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
int main(void)
{
    return LZ4_ID != 32004 || BZIP2_ID != 307 || JPEGLS_ID != 32018;
}
