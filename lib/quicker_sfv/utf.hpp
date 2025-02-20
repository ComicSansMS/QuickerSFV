#ifndef INCLUDE_GUARD_QUICKER_SFV_UTF_HPP
#define INCLUDE_GUARD_QUICKER_SFV_UTF_HPP

#include <cstddef>
#include <cstdint>
#include <span>

struct DecodeResult {
    uint32_t code_units_consumed;
    char32_t code_point;

    friend bool operator==(DecodeResult, DecodeResult) noexcept = default;
};

struct Utf16Encode {
    uint32_t number_of_code_units;
    char16_t encode[2];

    friend bool operator==(Utf16Encode, Utf16Encode) noexcept = default;
};

struct Utf8Encode {
    uint32_t number_of_code_units;
    char8_t encode[4];

    friend bool operator==(Utf8Encode, Utf8Encode) noexcept = default;
};

DecodeResult decodeUtf16(std::span<char16_t const> range);
Utf16Encode encodeUtf32ToUtf16(char32_t c);

DecodeResult decodeUtf8(std::span<char8_t const> range);
Utf8Encode encodeUtf32ToUtf8(char32_t c);

bool checkValidUtf8(std::span<std::byte const> range);

#endif
