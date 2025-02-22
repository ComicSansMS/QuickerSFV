#include <fast_crc32.hpp>

#include <intrin.h>

namespace quicker_sfv::crc {

bool supportsSse42() {
    int32_t data[4];
    __cpuidex(data, 1, 0);
    bool const sse42 = (data[2] & 0x0010'0000) != 0;
    return sse42;
}

bool supportsAvx512() {
    int32_t data[4];
    __cpuidex(data, 1, 0);
    bool const avx = (data[2] & 0x1000'0000) != 0;
    bool const pclmulqdq = (data[2] & 0x0000'0002) != 0;
    if (!avx) {
        return false;
    }
    __cpuidex(data, 7, 0);
    bool const avx512f = (data[1] & 0x0001'0000) != 0;
    bool const vpclmulqdq = (data[2] & 0x0000'0400) != 0;
    return avx512f && vpclmulqdq && pclmulqdq;
}

}
