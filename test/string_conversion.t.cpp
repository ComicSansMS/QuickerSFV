#include <quicker_sfv/detail/string_conversion.hpp>

#include <quicker_sfv/error.hpp>

#include <catch.hpp>

TEST_CASE("String Conversion")
{
    using namespace quicker_sfv::string_conversion;
    SECTION("Byte To Hex") {
        CHECK(byte_to_hex_str(std::byte{ 0x00 }).higher == u8'0');
        CHECK(byte_to_hex_str(std::byte{ 0x00 }).lower == u8'0');

        CHECK(byte_to_hex_str(std::byte{ 0x01 }).higher == u8'0');
        CHECK(byte_to_hex_str(std::byte{ 0x01 }).lower == u8'1');

        CHECK(byte_to_hex_str(std::byte{ 0x51 }).higher == u8'5');
        CHECK(byte_to_hex_str(std::byte{ 0x51 }).lower == u8'1');

        CHECK(byte_to_hex_str(std::byte{ 0xab }).higher == u8'a');
        CHECK(byte_to_hex_str(std::byte{ 0xab }).lower == u8'b');
    }

    SECTION("Hex to byte") {
        CHECK(hex_str_to_byte(Nibbles{ .higher = '0', .lower = '0' }) == std::byte{ 0x00 });
        CHECK(hex_str_to_byte(Nibbles{ .higher = '0', .lower = '1' }) == std::byte{ 0x01 });
        CHECK(hex_str_to_byte(Nibbles{ .higher = '1', .lower = '0' }) == std::byte{ 0x10 });
        CHECK(hex_str_to_byte(Nibbles{ .higher = 'a', .lower = 'b' }) == std::byte{ 0xab });
        CHECK(hex_str_to_byte(Nibbles{ .higher = 'A', .lower = 'B' }) == std::byte{ 0xab });
        CHECK(hex_str_to_byte(Nibbles{ .higher = 'd', .lower = 'c' }) == std::byte{ 0xdc });
        CHECK(hex_str_to_byte(Nibbles{ .higher = 'D', .lower = 'C' }) == std::byte{ 0xdc });
        CHECK(hex_str_to_byte(Nibbles{ .higher = 'e', .lower = 'f' }) == std::byte{ 0xef });
        CHECK(hex_str_to_byte(Nibbles{ .higher = 'E', .lower = 'F' }) == std::byte{ 0xef });
    }

    SECTION("Hex to str invalid input") {
        CHECK_THROWS_AS(hex_str_to_byte(Nibbles{ .higher = '0', .lower = ' ' }), quicker_sfv::Exception);
        CHECK_THROWS_AS(hex_str_to_byte(Nibbles{ .higher = '0', .lower = 'G' }), quicker_sfv::Exception);
        CHECK_THROWS_AS(hex_str_to_byte(Nibbles{ .higher = '0', .lower = 'Z' }), quicker_sfv::Exception);
        CHECK_THROWS_AS(hex_str_to_byte(Nibbles{ .higher = '0', .lower = '~' }), quicker_sfv::Exception);
        CHECK_THROWS_AS(hex_str_to_byte(Nibbles{ .higher = '0', .lower = '\0' }), quicker_sfv::Exception);
        CHECK_THROWS_AS(hex_str_to_byte(Nibbles{ .higher = 'K', .lower = '0' }), quicker_sfv::Exception);
        CHECK_THROWS_AS(hex_str_to_byte(Nibbles{ .higher = '=', .lower = '0' }), quicker_sfv::Exception);
        CHECK_THROWS_AS(hex_str_to_byte(Nibbles{ .higher = 'j', .lower = '`' }), quicker_sfv::Exception);
    }
}
