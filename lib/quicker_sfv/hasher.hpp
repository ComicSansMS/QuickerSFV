#ifndef INCLUDE_GUARD_QUICKER_SFV_HASHER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_HASHER_HPP

#include <quicker_sfv/digest.hpp>

#include <span>
#include <string>
#include <string_view>

namespace quicker_sfv {

struct HasherOptions {
    bool has_sse42;
    bool has_avx512;
};

class Hasher {
public:
    virtual ~Hasher() = 0;
    Hasher& operator=(Hasher&&) = delete;
    virtual void addData(std::span<std::byte const> data) = 0;
    virtual Digest finalize() = 0;
    virtual void reset() = 0;
};

}

#endif
