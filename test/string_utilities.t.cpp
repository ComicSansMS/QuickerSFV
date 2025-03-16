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
#include <quicker_sfv/string_utilities.hpp>

#include <catch.hpp>

#include <format>

std::ostream& operator<<(std::ostream& os, quicker_sfv::DecodeResult const& d) {
    return os << "Units consumed: " << d.code_units_consumed << ", Code point: " << static_cast<uint32_t>(d.code_point);
}

std::ostream& operator<<(std::ostream& os, quicker_sfv::Utf16Encode const& d) {
    if (d.number_of_code_units == 1) {
        return os << "{" << static_cast<uint16_t>(d.encode[0]) << "}";
    } else if (d.number_of_code_units == 2) {
        return os << "{" << static_cast<uint16_t>(d.encode[0]) << ", " << static_cast<uint16_t>(d.encode[1]) << "}";
    }
    return os << "Code units: " << d.number_of_code_units;
}

namespace Catch {

template<>
struct StringMaker<quicker_sfv::DecodeResult> {
    static std::string convert(quicker_sfv::DecodeResult value) {
        return std::format("Units consumed: {}, Code point: {}", value.code_units_consumed, static_cast<int>(value.code_point));
    }
};

template<>
struct StringMaker<quicker_sfv::Utf8Encode> {
    static std::string convert(quicker_sfv::Utf8Encode d) {
        if (d.number_of_code_units == 1) {
            return std::format("{{{}}}", static_cast<uint32_t>(d.encode[0]));
        } else if (d.number_of_code_units == 2) {
            return std::format("{{{}, {}}}", static_cast<uint32_t>(d.encode[0]), static_cast<uint32_t>(d.encode[1]));
        } else if (d.number_of_code_units == 3) {
            return std::format("{{{}, {}, {}}}", static_cast<uint32_t>(d.encode[0]), static_cast<uint32_t>(d.encode[1]),
                                                 static_cast<uint32_t>(d.encode[2]));
        } else if (d.number_of_code_units == 4) {
            return std::format("{{{}, {}, {}}}", static_cast<uint32_t>(d.encode[0]), static_cast<uint32_t>(d.encode[1]),
                                                 static_cast<uint32_t>(d.encode[2]), static_cast<uint32_t>(d.encode[3]));
        }
        return std::format("Code units: {}", d.number_of_code_units);
    }
};

template<>
struct StringMaker<quicker_sfv::Utf16Encode> {
    static std::string convert(quicker_sfv::Utf16Encode d) {
        if (d.number_of_code_units == 1) {
            return std::format("{{{}}}", static_cast<uint32_t>(d.encode[0]));
        } else if (d.number_of_code_units == 2) {
            return std::format("{{{}, {}}}", static_cast<uint32_t>(d.encode[0]), static_cast<uint32_t>(d.encode[1]));
        }
        return std::format("Code units: {}", d.number_of_code_units);
    }
};


}

TEST_CASE("String Utilities")
{
    using quicker_sfv::DecodeResult;
    using quicker_sfv::Utf8Encode;
    using quicker_sfv::Utf16Encode;
    SECTION("Utf8 Decode") {
        using quicker_sfv::decodeUtf8;
        CHECK(decodeUtf8(u8"") == DecodeResult{ .code_units_consumed = 1, .code_point = 0 });
        CHECK(decodeUtf8(u8"A") == DecodeResult{ .code_units_consumed = 1, .code_point = 65 });
        CHECK(decodeUtf8(u8" ") == DecodeResult{ .code_units_consumed = 1, .code_point = 32 });
        CHECK(decodeUtf8(u8"~") == DecodeResult{ .code_units_consumed = 1, .code_point = 126 });
        CHECK(decodeUtf8(u8"¡") == DecodeResult{ .code_units_consumed = 2, .code_point = 161 });
        CHECK(decodeUtf8(u8"߿") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x7ff });
        CHECK(decodeUtf8(u8"ࠀ") == DecodeResult{ .code_units_consumed = 3, .code_point = 0x800 });
        CHECK(decodeUtf8(u8"⁈") == DecodeResult{ .code_units_consumed = 3, .code_point = 0x2048 });
        char8_t const largest_3_byte[] = { 0xef, 0xbf, 0xbf };
        CHECK(decodeUtf8(largest_3_byte) == DecodeResult{ .code_units_consumed = 3, .code_point = 0xffff });
        CHECK(decodeUtf8(u8"𐀀") == DecodeResult{ .code_units_consumed = 4, .code_point = 0x10000 });
        CHECK(decodeUtf8(u8"𐐷") == DecodeResult{ .code_units_consumed = 4, .code_point = 0x10437 });
        CHECK(decodeUtf8(u8"🍫") == DecodeResult{ .code_units_consumed = 4, .code_point = 0x1f36b });
        CHECK(decodeUtf8(u8"𤭢") == DecodeResult{ .code_units_consumed = 4, .code_point = 0x24b62 });
        CHECK(decodeUtf8(u8"嶲") == DecodeResult{ .code_units_consumed = 4, .code_point = 0x2f9f4 });
        char8_t const private_area_utf8[] = { 0xf4, 0x83, 0x98, 0xaf };
        CHECK(decodeUtf8(private_area_utf8) == DecodeResult{ .code_units_consumed = 4, .code_point = 0x10362f });
        char8_t const biggest_code_point[] = { 0xf4, 0x8f, 0xbf, 0xbf };
        CHECK(decodeUtf8(biggest_code_point) == DecodeResult{ .code_units_consumed = 4, .code_point = 0x10ffff });
    }
    SECTION("Utf8 Decoding error") {
        using quicker_sfv::decodeUtf8;
        CHECK(decodeUtf8(std::span<char8_t const>{}) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t leading_multibyte[] = {0x80, u8'A'};
        CHECK(decodeUtf8(leading_multibyte) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });

        char8_t dangling_2byte[] = {0xc0, u8'A'};
        CHECK(decodeUtf8(dangling_2byte) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_2byte_h[] = {0xc0, 0xc0};
        CHECK(decodeUtf8(dangling_2byte_h) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_2byte_e[] = {0xc0};
        CHECK(decodeUtf8(dangling_2byte_e) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });

        char8_t dangling_3byte_1[] = { 0xe0, u8'A', u8'B' };
        CHECK(decodeUtf8(dangling_3byte_1) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_3byte_1h[] = { 0xe0, 0xc0, u8'B' };
        CHECK(decodeUtf8(dangling_3byte_1h) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_3byte_1e[] = { 0xe0 };
        CHECK(decodeUtf8(dangling_3byte_1e) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_3byte_2[] = { 0xe0, 0x80, u8'B' };
        CHECK(decodeUtf8(dangling_3byte_2) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_3byte_2h[] = { 0xe0, 0x80, 0xc0 };
        CHECK(decodeUtf8(dangling_3byte_2h) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_3byte_2e[] = { 0xe0, 0x80 };
        CHECK(decodeUtf8(dangling_3byte_2e) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });

        char8_t dangling_4byte_1[] = { 0xf0, u8'A', u8'B', u8'C' };
        CHECK(decodeUtf8(dangling_4byte_1) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_4byte_1h[] = { 0xf0, 0xc0, u8'B', u8'C' };
        CHECK(decodeUtf8(dangling_4byte_1h) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_4byte_1e[] = { 0xf0 };
        CHECK(decodeUtf8(dangling_4byte_1e) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_4byte_2[] = { 0xf0, 0x80, u8'B', u8'C' };
        CHECK(decodeUtf8(dangling_4byte_2) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_4byte_2h[] = { 0xf0, 0x80, 0xc0, u8'C' };
        CHECK(decodeUtf8(dangling_4byte_2h) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_4byte_2e[] = { 0xf0, 0x80 };
        CHECK(decodeUtf8(dangling_4byte_2e) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_4byte_3[] = { 0xf0, 0x80, 0x80, u8'C' };
        CHECK(decodeUtf8(dangling_4byte_3) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_4byte_3h[] = { 0xf0, 0x80, 0x80, 0xc0 };
        CHECK(decodeUtf8(dangling_4byte_3h) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char8_t dangling_4byte_3e[] = { 0xf0, 0x80, 0x80 };
        CHECK(decodeUtf8(dangling_4byte_3e) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });

        char8_t invalid_header[] = { 0xf8, 0x80, 0x80 };
        CHECK(decodeUtf8(invalid_header) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
    }
    SECTION("Utf16 Decode") {
        using quicker_sfv::decodeUtf16;
        CHECK(decodeUtf16(u"") == DecodeResult{ .code_units_consumed = 1, .code_point = 0 });
        CHECK(decodeUtf16(u"A") == DecodeResult{ .code_units_consumed = 1, .code_point = 65 });
        CHECK(decodeUtf16(u" ") == DecodeResult{ .code_units_consumed = 1, .code_point = 32 });
        CHECK(decodeUtf16(u"~") == DecodeResult{ .code_units_consumed = 1, .code_point = 126 });
        CHECK(decodeUtf16(u"¡") == DecodeResult{ .code_units_consumed = 1, .code_point = 161 });
        CHECK(decodeUtf16(u"߿") == DecodeResult{ .code_units_consumed = 1, .code_point = 0x7ff });
        CHECK(decodeUtf16(u"ࠀ") == DecodeResult{ .code_units_consumed = 1, .code_point = 0x800 });
        CHECK(decodeUtf16(u"⁈") == DecodeResult{ .code_units_consumed = 1, .code_point = 0x2048 });
        char16_t const largest_single_unit[] = { 0xffff, 0 };
        CHECK(decodeUtf16(largest_single_unit) == DecodeResult{ .code_units_consumed = 1, .code_point = 0xffff });
        CHECK(decodeUtf16(u"𐀀") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x10000 });
        CHECK(decodeUtf16(u"𐐷") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x10437 });
        CHECK(decodeUtf16(u"🍫") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x1f36b });
        CHECK(decodeUtf16(u"𤭢") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x24b62 });
        CHECK(decodeUtf16(u"嶲") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x2f9f4 });
        char16_t const private_area_utf16[] = { 0xdbcd, 0xde2f, 0 };
        CHECK(decodeUtf16(private_area_utf16) == DecodeResult{ .code_units_consumed = 2, .code_point = 0x10362f });
        char16_t const biggest_code_point[] = { 0xdbff, 0xdfff, 0 };
        CHECK(decodeUtf16(biggest_code_point) == DecodeResult{ .code_units_consumed = 2, .code_point = 0x10ffff });
    }
    SECTION("Utf16 Decoding Error") {
        using quicker_sfv::decodeUtf16;
        char16_t dangling_surrogate[] = { 0xd822, u'A' };
        CHECK(decodeUtf16(dangling_surrogate) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char16_t dangling_surrogate_end_of_string[] = { 0xD822 };
        CHECK(decodeUtf16(dangling_surrogate_end_of_string) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        char16_t surrogate_starting_with_low[] = { 0xdc00, 0xd800 };
        CHECK(decodeUtf16(surrogate_starting_with_low) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
        CHECK(decodeUtf16(std::span<char16_t const>{}) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });
    }
    SECTION("Utf16 Decode Non Strict") {
        using quicker_sfv::decodeUtf16_non_strict;
        CHECK(decodeUtf16_non_strict(u"") == DecodeResult{ .code_units_consumed = 1, .code_point = 0 });
        CHECK(decodeUtf16_non_strict(u"A") == DecodeResult{ .code_units_consumed = 1, .code_point = 65 });
        CHECK(decodeUtf16_non_strict(u" ") == DecodeResult{ .code_units_consumed = 1, .code_point = 32 });
        CHECK(decodeUtf16_non_strict(u"~") == DecodeResult{ .code_units_consumed = 1, .code_point = 126 });
        CHECK(decodeUtf16_non_strict(u"¡") == DecodeResult{ .code_units_consumed = 1, .code_point = 161 });
        CHECK(decodeUtf16_non_strict(u"߿") == DecodeResult{ .code_units_consumed = 1, .code_point = 0x7ff });
        CHECK(decodeUtf16_non_strict(u"ࠀ") == DecodeResult{ .code_units_consumed = 1, .code_point = 0x800 });
        CHECK(decodeUtf16_non_strict(u"⁈") == DecodeResult{ .code_units_consumed = 1, .code_point = 0x2048 });
        char16_t const largest_single_unit[] = { 0xffff, 0 };
        CHECK(decodeUtf16_non_strict(largest_single_unit) == DecodeResult{ .code_units_consumed = 1, .code_point = 0xffff });
        CHECK(decodeUtf16_non_strict(u"𐀀") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x10000 });
        CHECK(decodeUtf16_non_strict(u"𐐷") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x10437 });
        CHECK(decodeUtf16_non_strict(u"🍫") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x1f36b });
        CHECK(decodeUtf16_non_strict(u"𤭢") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x24b62 });
        CHECK(decodeUtf16_non_strict(u"嶲") == DecodeResult{ .code_units_consumed = 2, .code_point = 0x2f9f4 });
        char16_t const private_area_utf16[] = { 0xdbcd, 0xde2f, 0 };
        CHECK(decodeUtf16_non_strict(private_area_utf16) == DecodeResult{ .code_units_consumed = 2, .code_point = 0x10362f });
        char16_t const biggest_code_point[] = { 0xdbff, 0xdfff, 0 };
        CHECK(decodeUtf16_non_strict(biggest_code_point) == DecodeResult{ .code_units_consumed = 2, .code_point = 0x10ffff });
        // the following would all be errors under strict decoding rules
        char16_t dangling_surrogate[] = { 0xd822, u'A' };
        CHECK(decodeUtf16_non_strict(dangling_surrogate) == DecodeResult{ .code_units_consumed = 1, .code_point = 0xd822 });
        char16_t dangling_surrogate_end_of_string[] = { 0xd822 };
        CHECK(decodeUtf16_non_strict(dangling_surrogate_end_of_string) == DecodeResult{ .code_units_consumed = 1, .code_point = 0xd822 });
        char16_t surrogate_starting_with_low[] = { 0xdc00, 0xd800 };
        CHECK(decodeUtf16_non_strict(surrogate_starting_with_low) == DecodeResult{ .code_units_consumed = 1, .code_point = 0xdc00 });
        // the only error condition is an empty input
        CHECK(decodeUtf16_non_strict(std::span<char16_t const>{}) == DecodeResult{ .code_units_consumed = 0, .code_point = 0 });

    }
    SECTION("Encode Utf16") {
        using quicker_sfv::encodeUtf32ToUtf16;
        CHECK(encodeUtf32ToUtf16(U'\0') == Utf16Encode{ .number_of_code_units = 1, .encode = { 0, 0 } });
        CHECK(encodeUtf32ToUtf16(U'A') == Utf16Encode{ .number_of_code_units = 1, .encode = { 65, 0 } });
        CHECK(encodeUtf32ToUtf16(U' ') == Utf16Encode{ .number_of_code_units = 1, .encode = { 32, 0 } });
        CHECK(encodeUtf32ToUtf16(U'~') == Utf16Encode{ .number_of_code_units = 1, .encode = { 126, 0 } });
        CHECK(encodeUtf32ToUtf16(U'¡') == Utf16Encode{ .number_of_code_units = 1, .encode = { 161, 0 } });
        CHECK(encodeUtf32ToUtf16(U'߿') == Utf16Encode{ .number_of_code_units = 1, .encode = { 0x7ff, 0 } });
        CHECK(encodeUtf32ToUtf16(U'ࠀ') == Utf16Encode{ .number_of_code_units = 1, .encode = { 0x800, 0 } });
        CHECK(encodeUtf32ToUtf16(U'⁈') == Utf16Encode{ .number_of_code_units = 1, .encode = { 0x2048, 0 } });
        CHECK(encodeUtf32ToUtf16(char32_t{ 0xffff }) == Utf16Encode{ .number_of_code_units = 1, .encode = { 0xffff, 0 } });
        CHECK(encodeUtf32ToUtf16(U'𐀀') == Utf16Encode{ .number_of_code_units = 2, .encode = { 0xd800, 0xdc00 } });
        CHECK(encodeUtf32ToUtf16(U'𐐷') == Utf16Encode{ .number_of_code_units = 2, .encode = { 0xd801, 0xdc37 } });
        CHECK(encodeUtf32ToUtf16(U'🍫') == Utf16Encode{ .number_of_code_units = 2, .encode = { 0xd83c, 0xdf6b } });
        CHECK(encodeUtf32ToUtf16(U'𤭢') == Utf16Encode{ .number_of_code_units = 2, .encode = { 0xd852, 0xdf62 } });
        CHECK(encodeUtf32ToUtf16(U'嶲') == Utf16Encode{ .number_of_code_units = 2, .encode = { 0xd87e, 0xddf4 } });
        CHECK(encodeUtf32ToUtf16(char32_t{ 0x10362f }) == Utf16Encode{ .number_of_code_units = 2, .encode = { 0xdbcd, 0xde2f } });
        CHECK(encodeUtf32ToUtf16(char32_t{ 0x10ffff }) == Utf16Encode{ .number_of_code_units = 2, .encode = { 0xdbff, 0xdfff } });
    }
    SECTION("Encode Utf16 error") {
        using quicker_sfv::encodeUtf32ToUtf16;
        CHECK(encodeUtf32ToUtf16(char32_t{ 0x110000 }) == Utf16Encode{ .number_of_code_units = 0, .encode = { 0, 0 } });
        CHECK(encodeUtf32ToUtf16(char32_t{ 0x10000001 }) == Utf16Encode{ .number_of_code_units = 0, .encode = { 0, 0 } });
    }
    SECTION("Encode Utf8") {
        using quicker_sfv::encodeUtf32ToUtf8;
        CHECK(encodeUtf32ToUtf8(U'\0') == Utf8Encode{ .number_of_code_units = 1, .encode = { 0, 0, 0, 0 } });
        CHECK(encodeUtf32ToUtf8(U'A') == Utf8Encode{ .number_of_code_units = 1, .encode = { 65, 0, 0, 0 } });
        CHECK(encodeUtf32ToUtf8(U' ') == Utf8Encode{ .number_of_code_units = 1, .encode = { 32, 0, 0, 0 } });
        CHECK(encodeUtf32ToUtf8(U'~') == Utf8Encode{ .number_of_code_units = 1, .encode = { 126, 0, 0, 0 } });
        CHECK(encodeUtf32ToUtf8(U'¡') == Utf8Encode{ .number_of_code_units = 2, .encode = { 0xc2, 0xa1, 0, 0 } });
        CHECK(encodeUtf32ToUtf8(U'߿') == Utf8Encode{ .number_of_code_units = 2, .encode = { 0xdf, 0xbf, 0, 0 } });
        CHECK(encodeUtf32ToUtf8(U'ࠀ') == Utf8Encode{ .number_of_code_units = 3, .encode = { 0xe0, 0xa0, 0x80, 0 } });
        CHECK(encodeUtf32ToUtf8(U'⁈') == Utf8Encode{ .number_of_code_units = 3, .encode = { 0xe2, 0x81, 0x88, 0 } });
        CHECK(encodeUtf32ToUtf8(char32_t{ 0xffff }) == Utf8Encode{ .number_of_code_units = 3, .encode = { 0xef, 0xbf, 0xbf, 0} });
        CHECK(encodeUtf32ToUtf8(U'𐀀') == Utf8Encode{ .number_of_code_units = 4, .encode = { 0xf0, 0x90, 0x80, 0x80 } });
        CHECK(encodeUtf32ToUtf8(U'𐐷') == Utf8Encode{ .number_of_code_units = 4, .encode = { 0xf0, 0x90, 0x90, 0xb7 } });
        CHECK(encodeUtf32ToUtf8(U'🍫') == Utf8Encode{ .number_of_code_units = 4, .encode = { 0xf0, 0x9f, 0x8d, 0xab } });
        CHECK(encodeUtf32ToUtf8(U'𤭢') == Utf8Encode{ .number_of_code_units = 4, .encode = { 0xf0, 0xa4, 0xad, 0xa2 } });
        CHECK(encodeUtf32ToUtf8(U'嶲') == Utf8Encode{ .number_of_code_units = 4, .encode = { 0xf0, 0xaf, 0xa7, 0xb4 } });
        CHECK(encodeUtf32ToUtf8(char32_t{ 0x10362f }) == Utf8Encode{ .number_of_code_units = 4, .encode = { 0xf4, 0x83, 0x98, 0xaf } });
        CHECK(encodeUtf32ToUtf8(char32_t{ 0x10ffff }) == Utf8Encode{ .number_of_code_units = 4, .encode = { 0xf4, 0x8f, 0xbf, 0xbf } });
    }
    SECTION("Encode Utf8 error") {
        using quicker_sfv::encodeUtf32ToUtf8;
        CHECK(encodeUtf32ToUtf8(char32_t{ 0x110000 }) == Utf8Encode{ .number_of_code_units = 0, .encode = { 0, 0, 0, 0 } });
        CHECK(encodeUtf32ToUtf8(char32_t{ 0x10000001 }) == Utf8Encode{ .number_of_code_units = 0, .encode = { 0, 0, 0, 0 } });
    }
    SECTION("Check valid Utf8") {
        using quicker_sfv::checkValidUtf8;
        CHECK(checkValidUtf8(std::string_view{}));
        CHECK(checkValidUtf8(""));
        CHECK(checkValidUtf8("hello there! this is just some harmless text"));
        unsigned char const u_special_characters[] = {
            'A', 0xc2, 0xa1, 0xe0, 0xa0, 0x80, 0xf0, 0x9f, 0x8d, 0xab, 0xf0, 0xaf, 0xa7, 0xb4, 'Z', '\0'
        };
        CHECK(checkValidUtf8(reinterpret_cast<char const*>(u_special_characters)));
        unsigned char const u_bogus_values[] = {
            'A', 0xf2, 'B', 'X', '~', 'Z', '\0'
        };
        CHECK_FALSE(checkValidUtf8(reinterpret_cast<char const*>(u_bogus_values)));
    }
    SECTION("Assume Utf8") {
        using quicker_sfv::assumeUtf8;
        CHECK(assumeUtf8(std::string_view{}) == u8"");
        CHECK(assumeUtf8("") == u8"");
        CHECK(assumeUtf8("abc") == u8"abc");
        unsigned char const u_special_characters[] = {
            'A', 0xc2, 0xa1, 0xe0, 0xa0, 0x80, 0xf0, 0x9f, 0x8d, 0xab, 0xf0, 0xaf, 0xa7, 0xb4, 'Z', '\0'
        };
        CHECK(assumeUtf8(reinterpret_cast<char const*>(u_special_characters)) == u8"A¡ࠀ🍫嶲Z");
    }
    SECTION("Check valid Utf wide string") {
        using quicker_sfv::checkValidUtfWideString;
        CHECK(checkValidUtfWideString(std::wstring_view{}));
        CHECK(checkValidUtfWideString(std::wstring_view(L"")));
        CHECK(checkValidUtfWideString(std::wstring_view(L"hello there! this is just some harmless text")));
        wchar_t const u_special_characters[] = {
            L'A', 0x2048, 0xd83c, 0xdf6b, L'Z', L'\0'
        };
        CHECK(checkValidUtfWideString(std::wstring_view(u_special_characters)));
        if constexpr (sizeof(wchar_t) == 4) {
#ifdef _MSC_VER
            CHECK(false);
#else
            wchar_t const u_bogus_values[] = {
                'A', 0x110000, 'B', 'X', '~', 'Z', '\0'
            };
            CHECK_FALSE(checkValidUtfWideString(std::wstring_view(u_bogus_values)));
#endif
        } else if constexpr (sizeof(wchar_t) == 2) {
            wchar_t const u_bogus_values[] = {
                'A', 0xdfff, 'B', 'X', '~', 'Z', '\0'
            };
            CHECK_FALSE(checkValidUtfWideString(std::wstring_view(u_bogus_values)));
        }
    }

    SECTION("Convert Utf16 to Utf8") {
        using quicker_sfv::convertToUtf8;
        CHECK(convertToUtf8(std::u16string_view{}) == u8"");
        CHECK(convertToUtf8(u"") == u8"");
        CHECK(convertToUtf8(u"Hello there!") == u8"Hello there!");
        CHECK(convertToUtf8(u"A¡ࠀ🍫嶲Z") == u8"A¡ࠀ🍫嶲Z");
    }

    SECTION("Convert Utf8 to Utf16") {
        using quicker_sfv::convertToUtf16;
        CHECK(convertToUtf16(std::u8string_view{}) == u"");
        CHECK(convertToUtf16(u8"") == u"");
        CHECK(convertToUtf16(u8"Hello there!") == u"Hello there!");
        CHECK(convertToUtf16(u8"A¡ࠀ🍫嶲Z") == u"A¡ࠀ🍫嶲Z");
    }

    SECTION("Trim") {
        using quicker_sfv::trim;
        CHECK(trim(u8"") == u8"");
        CHECK(trim(u8"    ") == u8"");
        CHECK(trim(u8"abc") == u8"abc");
        CHECK(trim(u8"   abc   ") == u8"abc");
        CHECK(trim(u8" \t\r\n\v\fabc\t\r\n\v\f   ") == u8"abc");
        CHECK(trim(u8" \t\r\n\v\fa  b  c\t\r\n\v\f   ") == u8"a  b  c");
    }
    SECTION("Trim All Utf") {
        using quicker_sfv::trimAllUtf;
        CHECK(trimAllUtf(u8"") == u8"");
        CHECK(trimAllUtf(u8"   ") == u8"");
        CHECK(trimAllUtf(u8"abc") == u8"abc");
        CHECK(trimAllUtf(u8"   abc   ") == u8"abc");
        CHECK(trimAllUtf(u8" \t\r\n\v\fabc\t\r\n\v\f   ") == u8"abc");
        CHECK(trimAllUtf(u8" \t\r\n\v\fa  b  c\t\r\n\v\f   ") == u8"a  b  c");
        auto const d = [](char32_t c) -> std::u8string {
            auto const u = quicker_sfv::encodeUtf32ToUtf8(c);
            std::u8string ret;
            for (size_t i = 0; i < u.number_of_code_units; ++i) {
                ret.push_back(u.encode[i]);
            }
            return ret;
        };
        CHECK(trimAllUtf(d(0x2000) + d(0x0085) + d(0x00A0) + d(0x1680) + d(0x2000) + d(0x2001) + d(0x2002) + d(0x2003) +
            d(0x2004) + d(0x2005) + d(0x2006) + d(0x2007) + d(0x2008) + d(0x2009) + d(0x200A) + d(0x2028) + d(0x2029) +
            d(0x202F) + d(0x205F) + d(0x3000) + u8" \t\r\n\v\fabc\t\r\n\v\f   " +
            d(0x2000) + d(0x0085) + d(0x00A0) + d(0x1680) + d(0x2000) + d(0x2001) + d(0x2002) + d(0x2003) +
            d(0x2004) + d(0x2005) + d(0x2006) + d(0x2007) + d(0x2008) + d(0x2009) + d(0x200A) + d(0x2028) + d(0x2029) +
            d(0x202F) + d(0x205F) + d(0x3000) + d(0x2000) + d(0x2001)) == u8"abc");
        CHECK(trimAllUtf(d(0x2000) + d(0x0085) + d(0x00A0) + d(0x1680) + d(0x2000) + d(0x2001) + d(0x2002) + d(0x2003) +
            d(0x2004) + d(0x2005) + d(0x2006) + d(0x2007) + d(0x2008) + d(0x2009) + d(0x200A) + d(0x2028) + d(0x2029) +
            d(0x202F) + d(0x205F) + d(0x3000) + u8" \t\r\n\v\fa  b  c\t\r\n\v\f   " +
            d(0x2000) + d(0x0085) + d(0x00A0) + d(0x1680) + d(0x2000) + d(0x2001) + d(0x2002) + d(0x2003) +
            d(0x2004) + d(0x2005) + d(0x2006) + d(0x2007) + d(0x2008) + d(0x2009) + d(0x200A) + d(0x2028) + d(0x2029) +
            d(0x202F) + d(0x205F) + d(0x3000) + d(0x2000) + d(0x2001)) == u8"a  b  c");
        CHECK(trimAllUtf(d(0x2000) + d(0x0085) + d(0x00A0) + d(0x1680) + d(0x2000) + d(0x2001) + d(0x2002) + d(0x2003) +
            d(0x2004) + d(0x2005) + d(0x2006) + d(0x2007) + d(0x2008) + d(0x2009) + d(0x200A) + d(0x2028) + d(0x2029) +
            d(0x202F) + d(0x205F) + d(0x3000) + d(0x1F36B) + u8" \t\r\n\v\fabc\t\r\n\v\f   " +
            d(0x2000) + d(0x0085) + d(0x00A0) + d(0x1680) + d(0x2000) + d(0x2001) + d(0x2002) + d(0x2003) +
            d(0x2004) + d(0x2005) + d(0x2006) + d(0x2007) + d(0x2008) + d(0x2009) + d(0x200A) + d(0x2028) + d(0x2029) +
            d(0x202F) + d(0x205F) + d(0x3000) + d(0x2000) + d(0x2001)) == u8"🍫 \t\r\n\v\fabc");
    }
}

