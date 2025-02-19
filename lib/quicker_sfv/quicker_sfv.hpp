#ifndef INCLUDE_GUARD_QUICKER_SFV_QUICKER_SFV_HPP
#define INCLUDE_GUARD_QUICKER_SFV_QUICKER_SFV_HPP

#include <quicker_sfv/md5.hpp>
#include <quicker_sfv/md5_digest.hpp>
#include <quicker_sfv/sfv_file.hpp>

namespace quicker_sfv {


struct Version {
    int major;
    int minor;
    int patch;
};

Version getVersion();

void md5(void* data, size_t size, unsigned char* out);

}

#endif
