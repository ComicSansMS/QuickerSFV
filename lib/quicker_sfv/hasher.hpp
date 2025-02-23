#ifndef INCLUDE_GUARD_QUICKER_SFV_HASHER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_HASHER_HPP

#include <span>
#include <string>
#include <string_view>

namespace quicker_sfv {

class Hasher {
public:
    virtual void addData(std::span<char const> data) = 0;
    virtual Digest getDigest() = 0;
    virtual Digest digestFromString(std::u8string_view str) = 0;
};

}
#endif
