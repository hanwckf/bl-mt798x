/**
 * \file alignment.h
 *
 * \brief Utility code for dealing with unaligned memory accesses
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_LIBRARY_ALIGNMENT_H
#define MBEDTLS_LIBRARY_ALIGNMENT_H

#include <stdint.h>

#define UINT_UNALIGNED_STRUCT
typedef struct {
    uint16_t x;
} __attribute__((packed)) mbedtls_uint16_unaligned_t;
typedef struct {
    uint32_t x;
} __attribute__((packed)) mbedtls_uint32_unaligned_t;
typedef struct {
    uint64_t x;
} __attribute__((packed)) mbedtls_uint64_unaligned_t;

static inline uint32_t mbedtls_get_unaligned_uint32(const void *p)
{
    uint32_t r;
#if defined(UINT_UNALIGNED)
    mbedtls_uint32_unaligned_t *p32 = (mbedtls_uint32_unaligned_t *) p;
    r = *p32;
#elif defined(UINT_UNALIGNED_STRUCT)
    mbedtls_uint32_unaligned_t *p32 = (mbedtls_uint32_unaligned_t *) p;
    r = p32->x;
#else
    memcpy(&r, p, sizeof(r));
#endif
    return r;
}

static inline void mbedtls_put_unaligned_uint32(void *p, uint32_t x)
{
#if defined(UINT_UNALIGNED)
    mbedtls_uint32_unaligned_t *p32 = (mbedtls_uint32_unaligned_t *) p;
    *p32 = x;
#elif defined(UINT_UNALIGNED_STRUCT)
    mbedtls_uint32_unaligned_t *p32 = (mbedtls_uint32_unaligned_t *) p;
    p32->x = x;
#else
    memcpy(p, &x, sizeof(x));
#endif
}

/*
 * Detect GCC built-in byteswap routines
 */
#if defined(__GNUC__) && defined(__GNUC_PREREQ)
#if __GNUC_PREREQ(4, 3)
#define MBEDTLS_BSWAP32 __builtin_bswap32
#endif /* __GNUC_PREREQ(4,3) */
#endif /* defined(__GNUC__) && defined(__GNUC_PREREQ) */

/*
 * Detect Clang built-in byteswap routines
 */
#if defined(__clang__) && defined(__has_builtin)
#if __has_builtin(__builtin_bswap32) && !defined(MBEDTLS_BSWAP32)
#define MBEDTLS_BSWAP32 __builtin_bswap32
#endif /* __has_builtin(__builtin_bswap32) */
#endif /* defined(__clang__) && defined(__has_builtin) */

#if !defined(MBEDTLS_BSWAP32)
static inline uint32_t mbedtls_bswap32(uint32_t x)
{
    return
        (x & 0x000000ff) << 24 |
        (x & 0x0000ff00) <<  8 |
        (x & 0x00ff0000) >>  8 |
        (x & 0xff000000) >> 24;
}
#define MBEDTLS_BSWAP32 mbedtls_bswap32
#endif /* !defined(MBEDTLS_BSWAP32) */

#if (__BYTE_ORDER__) == (__ORDER_BIG_ENDIAN__)
#define MBEDTLS_IS_BIG_ENDIAN 1
#else
#define MBEDTLS_IS_BIG_ENDIAN 0
#endif

/**
 * Get the unsigned 32 bits integer corresponding to four bytes in
 * big-endian order (MSB first).
 *
 * \param   data    Base address of the memory to get the four bytes from.
 * \param   offset  Offset from \p data of the first and most significant
 *                  byte of the four bytes to build the 32 bits unsigned
 *                  integer from.
 */
#define MBEDTLS_GET_UINT32_BE(data, offset)                                \
    ((MBEDTLS_IS_BIG_ENDIAN)                                               \
        ? mbedtls_get_unaligned_uint32((data) + (offset))                  \
        : MBEDTLS_BSWAP32(mbedtls_get_unaligned_uint32((data) + (offset))) \
    )

/**
 * Put in memory a 32 bits unsigned integer in big-endian order.
 *
 * \param   n       32 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 32
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the most significant
 *                  byte of the 32 bits unsigned integer \p n.
 */
#define MBEDTLS_PUT_UINT32_BE(n, data, offset)                                   \
    {                                                                            \
        if (MBEDTLS_IS_BIG_ENDIAN)                                               \
        {                                                                        \
            mbedtls_put_unaligned_uint32((data) + (offset), (uint32_t) (n));     \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            mbedtls_put_unaligned_uint32((data) + (offset), MBEDTLS_BSWAP32((uint32_t) (n))); \
        }                                                                        \
    }

#endif /* MBEDTLS_LIBRARY_ALIGNMENT_H */
