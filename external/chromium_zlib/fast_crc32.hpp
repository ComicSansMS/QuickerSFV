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
