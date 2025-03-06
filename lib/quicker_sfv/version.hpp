#ifndef INCLUDE_GUARD_QUICKER_SFV_VERSION_HPP
#define INCLUDE_GUARD_QUICKER_SFV_VERSION_HPP

namespace quicker_sfv {

/** Version number.
 */
struct Version {
    int major;
    int minor;
    int patch;
};

/** Returns the version number of the library build.
 */
Version getVersion();

}

#endif
