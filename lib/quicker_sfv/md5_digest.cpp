#include <quicker_sfv/md5_digest.hpp>

namespace {
std::byte hex_char_to_nibble(char x) {
    switch (x) {
    case '0': return std::byte{ 0 };
    case '1': return std::byte{ 1 };
    case '2': return std::byte{ 2 };
    case '3': return std::byte{ 3 };
    case '4': return std::byte{ 4 };
    case '5': return std::byte{ 5 };
    case '6': return std::byte{ 6 };
    case '7': return std::byte{ 7 };
    case '8': return std::byte{ 8 };
    case '9': return std::byte{ 9 };
    case 'a': return std::byte{ 10 };
    case 'A': return std::byte{ 10 };
    case 'b': return std::byte{ 11 };
    case 'B': return std::byte{ 11 };
    case 'c': return std::byte{ 12 };
    case 'C': return std::byte{ 12 };
    case 'd': return std::byte{ 13 };
    case 'D': return std::byte{ 13 };
    case 'e': return std::byte{ 14 };
    case 'E': return std::byte{ 14 };
    case 'f': return std::byte{ 15 };
    case 'F': return std::byte{ 15 };
    default: std::abort();
    }
}

std::byte hex_str_to_byte(char upper, char lower) {
    return (hex_char_to_nibble(upper) << 4) | hex_char_to_nibble(lower);
}
}

MD5Digest MD5Digest::fromString(std::string_view str) {
    MD5Digest ret;
    for (int i = 0; i < 16; ++i) {
        char upper = str[i*2];
        char lower = str[i*2 + 1];
        ret.data[i] = hex_str_to_byte(upper, lower);
    }
    return ret;
}

MD5Digest MD5Digest::fromString(std::u8string_view str) {
    return fromString(std::string_view(reinterpret_cast<char const*>(str.data()), str.size()));
}
