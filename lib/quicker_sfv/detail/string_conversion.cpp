/*
 *   QuickerSFV - A fast checksum verifier
 *   Copyright (C) 2025  Andreas Weis (quickersfv@andreas-weis.net)
 *
 *   This file is part of QuickerSFV.
 *
 *   QuickerSFV is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   QuickerSFV is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <quicker_sfv/detail/string_conversion.hpp>

#include <quicker_sfv/error.hpp>

#include <assert.h>

namespace quicker_sfv::string_conversion {

namespace {

std::byte hex_char_to_nibble(char8_t x) {
    if ((x >= '0') && (x <= '9')) {
        return static_cast<std::byte>(x - u8'0');
    } else if ((x >= 'a') && (x <= 'f')) {
        return static_cast<std::byte>(x - u8'a' + 10);
    } else if ((x >= 'A') && (x <= 'F')) {
        return static_cast<std::byte>(x - u8'A' + 10);
    } else {
        throwException(Error::ParserError);
    }
}

char8_t nibble_to_hex_char(std::byte b) {
    char8_t c = static_cast<char8_t>(b);
    assert((c >= 0) && (c < 16));
    if (c < 10) {
        return c + u8'0';
    } else {
        return c - 10 + u8'a';
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
