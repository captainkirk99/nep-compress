/**
 * @file lz4_config.h
 * @brief Configuration shim for the LZ4 HDF5 plugin.
 *
 * Defines the platform feature macros that H5Zlz4.c expects from an
 * autotools-style config.h, so the plugin builds under CMake without
 * generated headers.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#ifndef NEPCOMPRESS_LZ4_CONFIG_H
#define NEPCOMPRESS_LZ4_CONFIG_H
/** Standard C headers are present. */
#define STDC_HEADERS 1
/** string.h is present. */
#define HAVE_STRING_H 1
/** stdint.h is present. */
#define HAVE_STDINT_H 1
/** inttypes.h is present. */
#define HAVE_INTTYPES_H 1
/** arpa/inet.h is present. */
#define HAVE_ARPA_INET_H 1
/** netinet/in.h is present. */
#define HAVE_NETINET_IN_H 1
#endif
