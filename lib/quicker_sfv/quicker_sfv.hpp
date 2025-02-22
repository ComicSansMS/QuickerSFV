#ifndef INCLUDE_GUARD_QUICKER_SFV_QUICKER_SFV_HPP
#define INCLUDE_GUARD_QUICKER_SFV_QUICKER_SFV_HPP

#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/md5.hpp>
#include <quicker_sfv/md5_digest.hpp>
#include <quicker_sfv/sfv_file.hpp>
#include <quicker_sfv/utf.hpp>

namespace quicker_sfv {


struct Version {
    int major;
    int minor;
    int patch;
};

Version getVersion();

bool supportsAvx512();
}

#endif
