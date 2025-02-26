#ifndef INCLUDE_GUARD_QUICKER_SFV_VERSION_HPP
#define INCLUDE_GUARD_QUICKER_SFV_VERSION_HPP

namespace quicker_sfv {

struct Version {
    int major;
    int minor;
    int patch;
};

Version getVersion();

}

#endif
