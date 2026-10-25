/**
 * @file consumer.c
 * @brief Downstream-consumer smoke test for the installed C library.
 *
 * Confirms that nep_compress.h can be included and that the filter ID
 * macros are visible.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <nep_compress.h>

/**
 * @brief Check the filter ID macros.
 *
 * @return 0 when LZ4_ID and BZIP2_ID have their expected values.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int main(void)
{
    return LZ4_ID != 32004 || BZIP2_ID != 307;
}
