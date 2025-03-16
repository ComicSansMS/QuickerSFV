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
#ifndef INCLUDE_GUARD_QUICKER_SFV_STRING_CONVERSION_HPP
#define INCLUDE_GUARD_QUICKER_SFV_STRING_CONVERSION_HPP

#include <cstddef>

/** Conversion between ASCII hex and byte.
 */
namespace quicker_sfv::string_conversion {

/** The hex representation of two 4-bit parts of a single byte.
 * Valid values for either field are `'0'`-`'9'`, `'A'`-`'F'`, and `'a'`-`'f'`.
 */
struct Nibbles {
    char8_t higher;     ///< The higher (most-significant) 4 bits.
    char8_t lower;      ///< The lower (least-significant) 4 bits.
};

/** Converts a pair of ASCII hex characters to the corresponding byte.
 * Valid values for either input character are `'0'`-`'9'`, `'A'`-`'F'`,
 * and `'a'`-`'f'`.
 * @param[in] higher The higher (most-significant) nibble of the byte.
 * @param[in] lower The lower (least-significant) nibble of the byte.
 * @return The converted byte value.
 * @throw Exception Error::ParserError if an input has an invalid value.
 */
std::byte hex_str_to_byte(char8_t higher, char8_t lower);

/** Converts a pair of ASCII hex characters to the corresponding byte.
 * @param[in] nibbles The input characters in Nibbles representation.
 * @return The converted byte value.
 * @throw Exception Error::ParserError if nibbles has an invalid value.
 */
std::byte hex_str_to_byte(Nibbles const& nibbles);

/** Converts a byte to ASCII hex representation.
 * @param[in] b Byte value to be converted.
 * @return ASCII hex characters of the byte in Nibbles representation.
 */
Nibbles byte_to_hex_str(std::byte b);

}
#endif
