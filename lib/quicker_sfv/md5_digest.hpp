#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_DIGEST_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_DIGEST_HPP

#include <cstddef>
#include <string>
#include <string_view>

struct MD5Digest {
    std::byte data[16];
    static MD5Digest fromString(std::string_view str);
    static MD5Digest fromString(std::u8string_view str);

    std::u8string toString() const;

    friend bool operator==(MD5Digest const&, MD5Digest const&) = default;
    friend bool operator!=(MD5Digest const&, MD5Digest const&) = default;
};

#endif
