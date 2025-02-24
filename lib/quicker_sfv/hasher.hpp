#ifndef INCLUDE_GUARD_QUICKER_SFV_HASHER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_HASHER_HPP

#include <quicker_sfv/digest.hpp>

#include <span>
#include <string>
#include <string_view>

namespace quicker_sfv {

class Hasher {
public:
    virtual ~Hasher() = 0;
    virtual void addData(std::span<char const> data) = 0;
    virtual Digest finalize() = 0;
    virtual Digest digestFromString(std::u8string_view str) const = 0;
};

}
#endif
