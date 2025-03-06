#ifndef INCLUDE_GUARD_QUICKER_SFV_CHROMIUM_ZLIB_FAST_CRC32_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CHROMIUM_ZLIB_FAST_CRC32_HPP

#include <cstddef>
#include <cstdint>

namespace quicker_sfv::crc {

bool supportsSse42();

bool supportsAvx512();

uint32_t crc32(char const* buffer, size_t buffer_size, uint32_t crc_start, bool use_avx512, bool use_sse42);

}
#endif
