#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_DIGEST_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_DIGEST_HPP

#include <cstddef>
#include <format>
#include <string_view>

struct MD5Digest {
    std::byte data[16];
    static MD5Digest fromString(std::string_view str);

    friend bool operator==(MD5Digest const&, MD5Digest const&) = default;
    friend bool operator!=(MD5Digest const&, MD5Digest const&) = default;
};

template<>
struct std::formatter<MD5Digest, char>
{
    template<class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<class FmtContext>
    FmtContext::iterator format(MD5Digest const& v, FmtContext& ctx) const
    {
        return format_to(ctx.out(), "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            static_cast<uint8_t>(v.data[0]),
            static_cast<uint8_t>(v.data[1]),
            static_cast<uint8_t>(v.data[2]),
            static_cast<uint8_t>(v.data[3]),
            static_cast<uint8_t>(v.data[4]),
            static_cast<uint8_t>(v.data[5]),
            static_cast<uint8_t>(v.data[6]),
            static_cast<uint8_t>(v.data[7]),
            static_cast<uint8_t>(v.data[8]),
            static_cast<uint8_t>(v.data[9]),
            static_cast<uint8_t>(v.data[10]),
            static_cast<uint8_t>(v.data[11]),
            static_cast<uint8_t>(v.data[12]),
            static_cast<uint8_t>(v.data[13]),
            static_cast<uint8_t>(v.data[14]),
            static_cast<uint8_t>(v.data[15])
        );
    }
};

#endif
