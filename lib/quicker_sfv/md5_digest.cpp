#include <quicker_sfv/md5_digest.hpp>

namespace {
std::byte hex_char_to_nibble(char8_t x) {
    if ((x >= '0') && (x <= '9')) {
        return static_cast<std::byte>(x - u8'0');
    } else if ((x >= 'a') && (x <= 'f')) {
        return static_cast<std::byte>(x - u8'a' + 10);
    } else if ((x >= 'A') && (x <= 'F')) {
        return static_cast<std::byte>(x - u8'a' + 10);
    } else {
        std::abort();
    }
}

char8_t nibble_to_hex_char(std::byte b) {
    char8_t c = static_cast<char8_t>(b);
    if (c < 10) {
        return c + u8'0';
    } else if (c < 16) {
        return c - 10 + u8'a';
    } else {
        std::abort();
    }
}

std::byte hex_str_to_byte(char8_t upper, char8_t lower) {
    return (hex_char_to_nibble(upper) << 4) | hex_char_to_nibble(lower);
}

std::byte lower_nibble(std::byte b) {
    return (b & std::byte{ 0x0f });
}

std::byte higher_nibble(std::byte b) {
    return ((b & std::byte{ 0xf0 }) >> 4);
}
}

namespace quicker_sfv {

MD5Digest MD5Digest::fromString(std::u8string_view str) {
    MD5Digest ret;
    for (int i = 0; i < 16; ++i) {
        char8_t const upper = str[i*2];
        char8_t const lower = str[i*2 + 1];
        ret.data[i] = hex_str_to_byte(upper, lower);
    }
    return ret;
}

std::u8string MD5Digest::toString() const {
    std::u8string ret;
    ret.reserve(17);
    for (std::byte const& b : data) {
        ret.push_back(nibble_to_hex_char(higher_nibble(b)));
        ret.push_back(nibble_to_hex_char(lower_nibble(b)));
    }
    return ret;
}

}
