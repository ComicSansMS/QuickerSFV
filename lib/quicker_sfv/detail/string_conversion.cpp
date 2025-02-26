#include <quicker_sfv/detail/string_conversion.hpp>

#include <quicker_sfv/error.hpp>

namespace quicker_sfv::string_conversion {

namespace {

std::byte hex_char_to_nibble(char8_t x) {
    if ((x >= '0') && (x <= '9')) {
        return static_cast<std::byte>(x - u8'0');
    } else if ((x >= 'a') && (x <= 'f')) {
        return static_cast<std::byte>(x - u8'a' + 10);
    } else if ((x >= 'A') && (x <= 'F')) {
        return static_cast<std::byte>(x - u8'a' + 10);
    } else {
        throwException(Error::ParserError);
    }
}

char8_t nibble_to_hex_char(std::byte b) {
    char8_t c = static_cast<char8_t>(b);
    if (c < 10) {
        return c + u8'0';
    } else if (c < 16) {
        return c - 10 + u8'a';
    } else {
        throwException(Error::ParserError);
    }
}

std::byte lower_nibble(std::byte b) {
    return (b & std::byte{ 0x0f });
}

std::byte higher_nibble(std::byte b) {
    return ((b & std::byte{ 0xf0 }) >> 4);
}

} // anonymous namespace

std::byte hex_str_to_byte(char8_t higher, char8_t lower) {
    return (hex_char_to_nibble(higher) << 4) | hex_char_to_nibble(lower);
}

std::byte hex_str_to_byte(Nibbles const& nibbles) {
    return hex_str_to_byte(nibbles.higher, nibbles.lower);
}

Nibbles byte_to_hex_str(std::byte b) {
    return Nibbles{
        .higher = nibble_to_hex_char(higher_nibble(b)),
        .lower = nibble_to_hex_char(lower_nibble(b))
    };
}

}
