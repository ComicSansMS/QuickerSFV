#ifndef INCLUDE_GUARD_QUICKER_SFV_CHROMIUM_ZLIB_FAST_CRC32_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CHROMIUM_ZLIB_FAST_CRC32_HPP

#include <cstddef>
#include <cstdint>

namespace quicker_sfv::crc {

bool supportsSse42();

bool supportsAvx512();

namespace detail {
uint32_t crc32_avx512_simd_(  /* AVX512+PCLMUL */
    const unsigned char* buf,
    size_t len,
    uint32_t crc);

uint32_t crc32_sse42_simd_(  /* SSE4.2+PCLMUL */
    const unsigned char* buf,
    size_t len,
    uint32_t crc);
}
}
#endif
