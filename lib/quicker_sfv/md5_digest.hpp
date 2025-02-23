#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_DIGEST_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_DIGEST_HPP

#include <quicker_sfv/digest.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace quicker_sfv {

struct MD5Digest {
    std::byte data[16];

    MD5Digest();

    static MD5Digest fromString(std::u8string_view str);

    std::u8string toString() const;

    friend bool operator==(MD5Digest const&, MD5Digest const&) = default;
    friend bool operator!=(MD5Digest const&, MD5Digest const&) = default;
};

static_assert(IsDigest<MD5Digest>, "MD5Digest is not a digest");

}
#endif
