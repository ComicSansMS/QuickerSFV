/* Vectorized CRC implementation adapted from
 * crc32_simd.c
 *
 * Copyright 2017 The Chromium Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */
 
/*
 * crc32_avx512_simd_(): compute the crc32 of the buffer, where the buffer
 * length must be at least 256, and a multiple of 64. Based on:
 *
 * "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
 *  V. Gopal, E. Ozturk, et al., 2009, http://intel.ly/2ySEwL0
 */

#include <fast_crc32.hpp>

#include <emmintrin.h>
#include <smmintrin.h>
#include <wmmintrin.h>
#include <immintrin.h>

namespace quicker_sfv::crc::detail {

uint32_t crc32_avx512_simd_(  /* AVX512+PCLMUL */
    const unsigned char* buf,
    size_t len,
    uint32_t crc)
{
    /*
     * Definitions of the bit-reflected domain constants k1,k2,k3,k4
     * are similar to those given at the end of the paper, and remaining
     * constants and CRC32+Barrett polynomials remain unchanged.
     *
     * Replace the index of x from 128 to 512. As follows:
     * k1 = ( x ^ ( 512 * 4 + 32 ) mod P(x) << 32 )' << 1 = 0x011542778a
     * k2 = ( x ^ ( 512 * 4 - 32 ) mod P(x) << 32 )' << 1 = 0x01322d1430
     * k3 = ( x ^ ( 512 + 32 ) mod P(x) << 32 )' << 1 = 0x0154442bd4
     * k4 = ( x ^ ( 512 - 32 ) mod P(x) << 32 )' << 1 = 0x01c6e41596
     */
    static const uint64_t alignas(64) k1k2[] = { 0x011542778a, 0x01322d1430,
                                                0x011542778a, 0x01322d1430,
                                                0x011542778a, 0x01322d1430,
                                                0x011542778a, 0x01322d1430 };
    static const uint64_t alignas(64) k3k4[] = { 0x0154442bd4, 0x01c6e41596,
                                                0x0154442bd4, 0x01c6e41596,
                                                0x0154442bd4, 0x01c6e41596,
                                                0x0154442bd4, 0x01c6e41596 };
    static const uint64_t alignas(16) k5k6[] = { 0x01751997d0, 0x00ccaa009e };
    static const uint64_t alignas(16) k7k8[] = { 0x0163cd6124, 0x0000000000 };
    static const uint64_t alignas(16) poly[] = { 0x01db710641, 0x01f7011641 };
    __m512i x0, x1, x2, x3, x4, x5, x6, x7, x8, y5, y6, y7, y8;
    __m128i a0, a1, a2, a3;

    /*
     * There's at least one block of 256.
     */
    x1 = _mm512_loadu_si512((__m512i*)(buf + 0x00));
    x2 = _mm512_loadu_si512((__m512i*)(buf + 0x40));
    x3 = _mm512_loadu_si512((__m512i*)(buf + 0x80));
    x4 = _mm512_loadu_si512((__m512i*)(buf + 0xC0));

    x1 = _mm512_xor_si512(x1, _mm512_castsi128_si512(_mm_cvtsi32_si128(crc)));

    x0 = _mm512_load_si512((__m512i*)k1k2);

    buf += 256;
    len -= 256;

    /*
     * Parallel fold blocks of 256, if any.
     */
    while (len >= 256)
    {
        x5 = _mm512_clmulepi64_epi128(x1, x0, 0x00);
        x6 = _mm512_clmulepi64_epi128(x2, x0, 0x00);
        x7 = _mm512_clmulepi64_epi128(x3, x0, 0x00);
        x8 = _mm512_clmulepi64_epi128(x4, x0, 0x00);


        x1 = _mm512_clmulepi64_epi128(x1, x0, 0x11);
        x2 = _mm512_clmulepi64_epi128(x2, x0, 0x11);
        x3 = _mm512_clmulepi64_epi128(x3, x0, 0x11);
        x4 = _mm512_clmulepi64_epi128(x4, x0, 0x11);

        y5 = _mm512_loadu_si512((__m512i*)(buf + 0x00));
        y6 = _mm512_loadu_si512((__m512i*)(buf + 0x40));
        y7 = _mm512_loadu_si512((__m512i*)(buf + 0x80));
        y8 = _mm512_loadu_si512((__m512i*)(buf + 0xC0));

        x1 = _mm512_xor_si512(x1, x5);
        x2 = _mm512_xor_si512(x2, x6);
        x3 = _mm512_xor_si512(x3, x7);
        x4 = _mm512_xor_si512(x4, x8);

        x1 = _mm512_xor_si512(x1, y5);
        x2 = _mm512_xor_si512(x2, y6);
        x3 = _mm512_xor_si512(x3, y7);
        x4 = _mm512_xor_si512(x4, y8);

        buf += 256;
        len -= 256;
    }

    /*
     * Fold into 512-bits.
     */
    x0 = _mm512_load_si512((__m512i*)k3k4);

    x5 = _mm512_clmulepi64_epi128(x1, x0, 0x00);
    x1 = _mm512_clmulepi64_epi128(x1, x0, 0x11);
    x1 = _mm512_xor_si512(x1, x2);
    x1 = _mm512_xor_si512(x1, x5);

    x5 = _mm512_clmulepi64_epi128(x1, x0, 0x00);
    x1 = _mm512_clmulepi64_epi128(x1, x0, 0x11);
    x1 = _mm512_xor_si512(x1, x3);
    x1 = _mm512_xor_si512(x1, x5);

    x5 = _mm512_clmulepi64_epi128(x1, x0, 0x00);
    x1 = _mm512_clmulepi64_epi128(x1, x0, 0x11);
    x1 = _mm512_xor_si512(x1, x4);
    x1 = _mm512_xor_si512(x1, x5);

    /*
     * Single fold blocks of 64, if any.
     */
    while (len >= 64)
    {
        x2 = _mm512_loadu_si512((__m512i*)buf);

        x5 = _mm512_clmulepi64_epi128(x1, x0, 0x00);
        x1 = _mm512_clmulepi64_epi128(x1, x0, 0x11);
        x1 = _mm512_xor_si512(x1, x2);
        x1 = _mm512_xor_si512(x1, x5);

        buf += 64;
        len -= 64;
    }

    /*
     * Fold 512-bits to 384-bits.
     */
    a0 = _mm_load_si128((__m128i*)k5k6);

    a1 = _mm512_extracti32x4_epi32(x1, 0);
    a2 = _mm512_extracti32x4_epi32(x1, 1);

    a3 = _mm_clmulepi64_si128(a1, a0, 0x00);
    a1 = _mm_clmulepi64_si128(a1, a0, 0x11);

    a1 = _mm_xor_si128(a1, a3);
    a1 = _mm_xor_si128(a1, a2);

    /*
     * Fold 384-bits to 256-bits.
     */
    a2 = _mm512_extracti32x4_epi32(x1, 2);
    a3 = _mm_clmulepi64_si128(a1, a0, 0x00);
    a1 = _mm_clmulepi64_si128(a1, a0, 0x11);
    a1 = _mm_xor_si128(a1, a3);
    a1 = _mm_xor_si128(a1, a2);

    /*
     * Fold 256-bits to 128-bits.
     */
    a2 = _mm512_extracti32x4_epi32(x1, 3);
    a3 = _mm_clmulepi64_si128(a1, a0, 0x00);
    a1 = _mm_clmulepi64_si128(a1, a0, 0x11);
    a1 = _mm_xor_si128(a1, a3);
    a1 = _mm_xor_si128(a1, a2);

    /*
     * Fold 128-bits to 64-bits.
     */
    a2 = _mm_clmulepi64_si128(a1, a0, 0x10);
    a3 = _mm_setr_epi32(~0, 0, ~0, 0);
    a1 = _mm_srli_si128(a1, 8);
    a1 = _mm_xor_si128(a1, a2);

    a0 = _mm_loadl_epi64((__m128i*)k7k8);
    a2 = _mm_srli_si128(a1, 4);
    a1 = _mm_and_si128(a1, a3);
    a1 = _mm_clmulepi64_si128(a1, a0, 0x00);
    a1 = _mm_xor_si128(a1, a2);

    /*
     * Barret reduce to 32-bits.
     */
    a0 = _mm_load_si128((__m128i*)poly);

    a2 = _mm_and_si128(a1, a3);
    a2 = _mm_clmulepi64_si128(a2, a0, 0x10);
    a2 = _mm_and_si128(a2, a3);
    a2 = _mm_clmulepi64_si128(a2, a0, 0x00);
    a1 = _mm_xor_si128(a1, a2);

    /*
     * Return the crc32.
     */
    return _mm_extract_epi32(a1, 1);
}

}
