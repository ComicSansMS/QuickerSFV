#include <quicker_sfv/quicker_sfv.hpp>

#include <fast_crc32.hpp>

namespace quicker_sfv {

Version getVersion() {
    return Version{
        .major = 1,
        .minor = 0,
        .patch = 0
    };
}

bool supportsAvx512() {
    return crc::supportsAvx512();
}

}
