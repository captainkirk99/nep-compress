/*
 * This file is an example of an HDF5 filter plugin.
 * The plugin can be used with the HDF5 library vesrion 1.8.11+ to read
 * HDF5 datasets compressed with lz4.
 */

/**
 * @file H5Zlz4.c
 * @brief HDF5 filter plugin for LZ4 compression (filter ID 32004).
 *
 * This file is vendored unchanged from HDF Group hdf5_plugins
 * `LZ4/src/H5Zlz4.c` at commit `4d6b8cec92` (upstream master); only
 * comments were added. See plugins/README.md for how to update it.
 *
 * Filter parameters: `cd_values[0]` is the block size in bytes (0
 * selects the default of 1 GiB) and `cd_values[1]` is the encoder
 * selector (0 or 1 for the default fast encoder, a negative value for
 * fast-mode acceleration `1 - value`, and a value >= 2 for LZ4HC at
 * that level).
 *
 * The on-disk chunk encoding is described in
 * @ref md_plugins_2LZ4__HDF5__format "the LZ4 HDF5 format page".
 *
 * Copyright (c) 2017, The HDF Group. See plugins/COPYING.hdf5_plugins.
 *
 * @author The HDF Group
 */

#include "lz4_config.h"
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#endif
#ifdef HAVE_STRING_H
#if !defined STDC_HEADERS && defined HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h> /* for htonl() */
#else
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#endif

#include "H5PLextern.h"
#include "lz4.h"
#include "lz4hc.h"

/**
 * @brief The LZ4 filter callback called by the HDF5 library.
 *
 * Runs in both directions: compression (forward) and decompression
 * (H5Z_FLAG_REVERSE set in @p flags). The buffer is replaced in
 * place through @p buf / @p buf_size.
 *
 * @param flags Filter flags; H5Z_FLAG_REVERSE selects decompression.
 * @param cd_nelmts Number of filter parameters in @p cd_values.
 * @param cd_values Filter parameters; `cd_values[0]` is the block
 * size in bytes (0 for the 1 GiB default) and `cd_values[1]` is the
 * encoder selector.
 * @param nbytes Size in bytes of the input data in @p buf.
 * @param buf_size Size in bytes of the buffer pointed to by @p buf.
 * @param buf Pointer to the buffer holding the data; on success it is
 * replaced with the filtered buffer.
 * @return The size of the filtered data, or 0 on failure.
 *
 * @author The HDF Group
 */
static size_t H5Z_filter_lz4(unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[],
                             size_t nbytes, size_t *buf_size, void **buf);

/** LZ4 HDF5 filter ID. */
#define H5Z_FILTER_LZ4 32004

/** Convert a 64-bit value from host to network byte order. */
#define htonll(x) (((uint64_t)(htonl((uint32_t)((x << 32) >> 32))) << 32) | htonl(((uint32_t)(x >> 32))))
/** Convert a 64-bit value from network to host byte order. */
#define ntohll(x) htonll(x)

/** Convert a 16-bit value from host to big endian byte order. */
#define htobe16t(x) htons(x)
/** Convert a 32-bit value from host to big endian byte order. */
#define htobe32t(x) htonl(x)
/** Convert a 64-bit value from host to big endian byte order. */
#define htobe64t(x) htonll(x)
/** Convert a 16-bit value from big endian to host byte order. */
#define be16toht(x) ntohs(x)
/** Convert a 32-bit value from big endian to host byte order. */
#define be32toht(x) ntohl(x)
/** Convert a 64-bit value from big endian to host byte order. */
#define be64toht(x) ntohll(x)

/** Default LZ4 block size (1 GiB); LZ4 needs blocks < 1.97 GiB. */
#define DEFAULT_BLOCK_SIZE 1 << 30 /* 1GiB. LZ4 needs blocks < 1.97GiB. */

/**
 * @brief HDF5 filter class structure for the LZ4 filter.
 *
 * @author The HDF Group
 */
const H5Z_class2_t H5Z_LZ4[1] = {{
    H5Z_CLASS_T_VERS,             /* H5Z_class_t version */
    (H5Z_filter_t)H5Z_FILTER_LZ4, /* Filter id number             */
#ifdef FILTER_DECODE_ONLY
    0, /* encoder_present flag (false is not available) */
#else
    1, /* encoder_present flag (set to true) */
#endif
    1, /* decoder_present flag (set to true) */
    "HDF5 lz4 filter; see "
    "https://github.com/HDFGroup/hdf5_plugins/blob/master/docs/RegisteredFilterPlugins.md",
    /* Filter name for debugging    */
    NULL,                       /* The "can apply" callback     */
    NULL,                       /* The "set local" callback     */
    (H5Z_func_t)H5Z_filter_lz4, /* The actual filter function   */
}};

/**
 * @brief Return the plugin type (H5PL_TYPE_FILTER) to the HDF5
 * library.
 *
 * @return H5PL_TYPE_FILTER.
 *
 * @author The HDF Group
 */
H5PL_type_t
H5PLget_plugin_type(void)
{
    return H5PL_TYPE_FILTER;
}
/**
 * @brief Return the LZ4 filter class structure to the HDF5 library.
 *
 * @return Pointer to H5Z_LZ4.
 *
 * @author The HDF Group
 */
const void *
H5PLget_plugin_info(void)
{
    return H5Z_LZ4;
}

/* liblz4 caps fast-mode acceleration at LZ4_ACCELERATION_MAX internally but
 * does not expose the macro in lz4.h; mirror the value from lz4.c here. */
#ifndef LZ4_ACCELERATION_MAX
/** Maximum fast-mode acceleration accepted by liblz4. */
#define LZ4_ACCELERATION_MAX 65537
#endif

/**
 * @brief Compress one block, selecting the encoder from the filter
 * parameter.
 *
 * Mirrors liblz4's lz4frame compressionLevel convention: a value >=
 * LZ4HC_CLEVEL_MIN (2) selects LZ4HC at that level, 0 or 1 selects the
 * default fast encoder, and a negative value selects fast mode with
 * acceleration `1 - value`.
 *
 * @param src Pointer to the input block.
 * @param dst Pointer to the output buffer.
 * @param srcSz Size in bytes of the input block.
 * @param dstCap Capacity in bytes of the output buffer.
 * @param encoderParam Encoder selector from `cd_values[1]`.
 * @return The number of compressed bytes, or 0 on failure.
 *
 * @author The HDF Group
 */
static int
lz4_encode(const char *src, char *dst, int srcSz, int dstCap, int encoderParam)
{
    /* Mirror liblz4's lz4frame compressionLevel convention:
     *   >= LZ4HC_CLEVEL_MIN (2) -> LZ4HC at that level
     *   0 or 1                  -> default fast (acceleration 1)
     *   < 0                     -> fast, acceleration = -encoderParam + 1
     * Clamp to the usable range first so the acceleration arithmetic cannot
     * overflow. */
    if (encoderParam > LZ4HC_CLEVEL_MAX)
        encoderParam = LZ4HC_CLEVEL_MAX;
    if (encoderParam < -(LZ4_ACCELERATION_MAX - 1))
        encoderParam = -(LZ4_ACCELERATION_MAX - 1);

    if (encoderParam >= LZ4HC_CLEVEL_MIN)
        return LZ4_compress_HC(src, dst, srcSz, dstCap, encoderParam);
    else {
        int acceleration = (encoderParam < 0) ? -encoderParam + 1 : 1;
        return LZ4_compress_fast(src, dst, srcSz, dstCap, acceleration);
    }
}

static size_t
H5Z_filter_lz4(unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[], size_t nbytes,
               size_t *buf_size, void **buf)
{
    void  *outBuf = NULL;
    size_t ret_value;

    if (flags & H5Z_FLAG_REVERSE) {
        uint32_t             *i32Buf;
        uint32_t              blockSize;
        char                 *roBuf; /* pointer to current write position */
        uint64_t              decompSize;
        const char           *rpos     = (char *)*buf; /* pointer to current read position */
        const uint64_t *const i64Buf   = (uint64_t *)rpos;
        const uint64_t        origSize = (uint64_t)(be64toht(*i64Buf)); /* is saved in be format */
        rpos += 8;                                                      /* advance the pointer */

        i32Buf    = (uint32_t *)rpos;
        blockSize = (uint32_t)(be32toht(*i32Buf));
        rpos += 4;
        if (blockSize > origSize)
            blockSize = origSize;

        if (NULL == (outBuf = malloc(origSize))) {
            printf("cannot malloc\n");
            goto error;
        }
        roBuf      = (char *)outBuf; /* pointer to current write position */
        decompSize = 0;
        /// start with the first block ///
        while (decompSize < origSize) {
            uint32_t compressedBlockSize; /// is saved in be format

            if (origSize - decompSize < blockSize) /* the last block can be smaller than blockSize. */
                blockSize = origSize - decompSize;
            i32Buf              = (uint32_t *)rpos;
            compressedBlockSize = be32toht(*i32Buf); /// is saved in be format
            rpos += 4;
            if (compressedBlockSize == blockSize) /* there was no compression */
            {
                memcpy(roBuf, rpos, blockSize);
            }
            else /* do the decompression */
            {
                int compressedBytes = LZ4_decompress_safe(rpos, roBuf, compressedBlockSize, blockSize);
                if (compressedBytes != blockSize) {
                    printf("decompressed size not the same: %d, != %d\n", compressedBytes, blockSize);
                    goto error;
                }
            }

            rpos += compressedBlockSize; /* advance the read pointer to the next block */
            roBuf += blockSize;          /* advance the write pointer */
            decompSize += blockSize;
        }
        free(*buf);
        *buf      = outBuf;
        *buf_size = (size_t)origSize;
        outBuf    = NULL;
        ret_value =
            (size_t)origSize; // should always work, as orig_size cannot be > 2GB (sizeof(size_t) < 4GB)
    }
    else /* forward filter */
    {
        size_t    blockSize;
        size_t    nBlocks;
        size_t    outSize; /* size of the output buffer. Header size (12 bytes) is included */
        size_t    block;
        uint64_t *i64Buf;
        uint32_t *i32Buf;
        size_t    maxDestSize;
        char     *rpos;  /* pointer to current read position */
        char     *roBuf; /* pointer to current write position */
        int       encoderParam;
        int       blockBound;
        size_t    perBlock;

        if (nbytes > INT32_MAX) {
            /* can only compress chunks up to 2GB */
            goto error;
        }

        if (cd_nelmts > 0 && cd_values[0] > 0) {
            blockSize = cd_values[0];
        }
        else {
            blockSize = DEFAULT_BLOCK_SIZE;
        }
        encoderParam = (cd_nelmts > 1) ? (int)cd_values[1] : 0;
        if (blockSize > nbytes) {
            blockSize = nbytes;
        }
        nBlocks = (nbytes - 1) / blockSize + 1;

        /* LZ4_compressBound returns 0 when blockSize exceeds LZ4_MAX_INPUT_SIZE,
         * which sits ~32 MiB below INT32_MAX -- so a block in that gap passes the
         * nbytes check above. Reject it before sizing the output buffer; this
         * also guarantees the (int)blockSize casts below stay in range. */
        blockBound = LZ4_compressBound((int)blockSize);
        if (blockBound <= 0)
            goto error;

        /* Output size = nBlocks * (blockBound + 4-byte block header) + 12-byte
         * chunk header. Guard the size_t arithmetic before malloc: a tiny
         * blockSize makes nBlocks huge, and on a 32-bit platform the product
         * can wrap, yielding an undersized buffer and a heap overflow in the
         * write loop below. blockBound > 0 here, so perBlock cannot be zero. */
        perBlock = (size_t)blockBound + 4;
        if (nBlocks > (SIZE_MAX - 12) / perBlock)
            goto error;
        maxDestSize = nBlocks * perBlock + 12;
        if (NULL == (outBuf = malloc(maxDestSize))) {
            goto error;
        }

        rpos  = (char *)*buf;   /* pointer to current read position */
        roBuf = (char *)outBuf; /* pointer to current write position */
        /* header */
        i64Buf    = (uint64_t *)(roBuf);
        i64Buf[0] = htobe64t((uint64_t)nbytes); /* Store decompressed size in be format */
        roBuf += 8;

        i32Buf    = (uint32_t *)(roBuf);
        i32Buf[0] = htobe32t((uint32_t)blockSize); /* Store the block size in be format */
        roBuf += 4;

        outSize = 12; /* size of the output buffer. Header size (12 bytes) is included */

        for (block = 0; block < nBlocks; ++block) {
            uint32_t compBlockSize; /// reserve space for compBlockSize
            size_t   origWritten = block * blockSize;
            if (nbytes - origWritten < blockSize) /* the last block may be < blockSize */
                blockSize = nbytes - origWritten;

            compBlockSize = lz4_encode(rpos, roBuf + 4, (int)blockSize, LZ4_compressBound((int)blockSize),
                                       encoderParam); /// reserve space for compBlockSize
            if (!compBlockSize)
                goto error;
            if (compBlockSize >= blockSize) /* compression did not save any space, do a memcpy instead */
            {
                compBlockSize = blockSize;
                memcpy(roBuf + 4, rpos, blockSize);
            }

            i32Buf    = (uint32_t *)(roBuf);
            i32Buf[0] = htobe32t((uint32_t)compBlockSize); /* write blocksize */
            roBuf += 4;

            rpos += blockSize;      /* advance read pointer */
            roBuf += compBlockSize; /* advance write pointer */
            outSize += compBlockSize + 4;
        }

        free(*buf);
        *buf      = outBuf;
        *buf_size = outSize;
        outBuf    = NULL;
        ret_value = outSize;
    }

    if (outBuf)
        free(outBuf);
    return ret_value;

error:
    if (outBuf)
        free(outBuf);
    outBuf = NULL;
    return 0;
}
