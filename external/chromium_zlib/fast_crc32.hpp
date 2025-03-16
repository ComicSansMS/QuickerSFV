/*
 *   QuickerSFV - A fast checksum verifier
 *   Copyright (C) 2025  Andreas Weis (quickersfv@andreas-weis.net)
 *
 *   This file is part of QuickerSFV.
 *
 *   QuickerSFV is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   QuickerSFV is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef INCLUDE_GUARD_QUICKER_SFV_CHROMIUM_ZLIB_FAST_CRC32_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CHROMIUM_ZLIB_FAST_CRC32_HPP

#include <cstddef>
#include <cstdint>

/** Fast CRC32 calculation.
 */
namespace quicker_sfv::crc {

/** Checks whether the CPU supports the SSE4.2 instruction set.
 * @note This is always true for x64 CPUs.
 */
bool supportsSse42();

/** Checks whether the CPU supports the AVX512 instruction set.
 */
bool supportsAvx512();

/** Computes the CRC32 checksum (CRC-32/ISO-HDLC) of the provided buffer.
 * @param[in] buffer Buffer containing the data to be hashed.
 * @param[in] buffer_size Size of the buffer in bytes.
 * @param[in] crc_start Start value of the checksum calculation. This is 0 for the 
 *                      first call to crc32 and the return value from the most recent
 *                      call to crc32 on repeated invocations.
 * @param[in] use_avx512 Whether to use AVX512 for the computation. Should be set
 *                       to the return value of supportsAvx512().
 * @param[in] use_sse42 Whether to use SSE42 for computation. Should be set to the
 *                      return value of supportsSse42().
 */
uint32_t crc32(char const* buffer, size_t buffer_size, uint32_t crc_start, bool use_avx512, bool use_sse42);

}
#endif
