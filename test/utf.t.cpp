#include <quicker_sfv/utf.hpp>

#include <catch.hpp>

#include <ostream>

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

std::ostream& operator<<(std::ostream& os, quicker_sfv::Utf8Encode const& d) {
    if (d.number_of_code_units == 1) {
        return os << "{" << static_cast<uint32_t>(d.encode[0]) << "}";
    } else if (d.number_of_code_units == 2) {
        return os << "{" << static_cast<uint32_t>(d.encode[0]) << ", " << static_cast<uint16_t>(d.encode[1]) << "}";
    } else if (d.number_of_code_units == 3) {
        return os << "{" << static_cast<uint32_t>(d.encode[0]) << ", " << static_cast<uint16_t>(d.encode[1]) << ", " << static_cast<uint16_t>(d.encode[2]) << "}";
    } else if (d.number_of_code_units == 4) {
        return os << "{" << static_cast<uint32_t>(d.encode[0]) << ", " << static_cast<uint16_t>(d.encode[1]) << ", " << static_cast<uint16_t>(d.encode[2]) << ", " << static_cast<uint16_t>(d.encode[3]) << "}";
    }
    return os << "Code units: " << d.number_of_code_units;
}

TEST_CASE("Unicode")
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
        char8_t const largest_3_byte[] = {0xef, 0xbf, 0xbf};
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
}

