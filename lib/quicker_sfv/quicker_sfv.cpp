#include <quicker_sfv/quicker_sfv.hpp>

#include <fast_crc32.hpp>

namespace quicker_sfv {

bool supportsAvx512() {
    return crc::supportsAvx512();
}

}
