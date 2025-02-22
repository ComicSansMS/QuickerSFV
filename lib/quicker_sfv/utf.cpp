#include <quicker_sfv/utf.hpp>

#include <span>

namespace quicker_sfv {

DecodeResult decodeUtf16(std::span<char16_t const> range) {
    constexpr DecodeResult const error = DecodeResult{ .code_units_consumed = 0, .code_point = 0 };
    enum State {
        Neutral,
        Awaiting1CodeUnit,
    } state = State::Neutral;
    constexpr char16_t const SURROGATE_MASK = 0xfc00;
    constexpr char16_t const SURROGATE_HEADER_HIGH = 0xd800;
    constexpr char16_t const SURROGATE_HEADER_LOW = 0xdc00;
    size_t i = 0;
    size_t const i_end = range.size();
    char32_t acc = 0;
    for (; i < i_end; ++i) {
        wchar_t const c = range[i];
        switch (state) {
        case State::Neutral: {
            if ((c & SURROGATE_MASK) == SURROGATE_HEADER_HIGH) {
                acc |= ((c & (~SURROGATE_MASK)) << 10);
                state = State::Awaiting1CodeUnit;
            } else if ((c & SURROGATE_MASK) == SURROGATE_HEADER_LOW) {
                return error;
            } else {
                return DecodeResult{ .code_units_consumed = 1, .code_point = static_cast<char32_t>(c) };
            }
        } break;
        case State::Awaiting1CodeUnit: {
            if ((c & SURROGATE_MASK) == SURROGATE_HEADER_LOW) {
                acc |= (c & (~SURROGATE_MASK));
                acc += 0x0001'0000;
                return DecodeResult{ .code_units_consumed = 2, .code_point = acc };
            }
            return error;
        } break;
        default: return error;
        }
    }
    return error;
}

DecodeResult decodeUtf16_non_strict(std::span<char16_t const> range) {
    constexpr DecodeResult const error = DecodeResult{ .code_units_consumed = 0, .code_point = 0 };
    enum State {
        Neutral,
        Awaiting1CodeUnit,
    } state = State::Neutral;
    constexpr char16_t const SURROGATE_MASK = 0xfc00;
    constexpr char16_t const SURROGATE_HEADER_HIGH = 0xd800;
    constexpr char16_t const SURROGATE_HEADER_LOW = 0xdc00;
    size_t i = 0;
    size_t const i_end = range.size();
    char32_t acc = 0;
    for (; i < i_end; ++i) {
        wchar_t const c = range[i];
        switch (state) {
        case State::Neutral: {
            if ((c & SURROGATE_MASK) == SURROGATE_HEADER_HIGH) {
                acc |= ((c & (~SURROGATE_MASK)) << 10);
                state = State::Awaiting1CodeUnit;
            } else if ((c & SURROGATE_MASK) == SURROGATE_HEADER_LOW) {
                // technically a decoding error, but we treat it like a single character
                return DecodeResult{ .code_units_consumed = 1, .code_point = static_cast<char32_t>(c) };
            } else {
                return DecodeResult{ .code_units_consumed = 1, .code_point = static_cast<char32_t>(c) };
            }
        } break;
        case State::Awaiting1CodeUnit: {
            if ((c & SURROGATE_MASK) == SURROGATE_HEADER_LOW) {
                acc |= (c & (~SURROGATE_MASK));
                acc += 0x0001'0000;
                return DecodeResult{ .code_units_consumed = 2, .code_point = acc };
            }
            // technically a decoding error, but we treat it like a single character
            return DecodeResult{ .code_units_consumed = 1, .code_point = static_cast<char32_t>(range[i - 1]) };
        } break;
        default: return error;
        }
    }
    return error;
}


Utf16Encode encodeUtf32ToUtf16(char32_t c) {
    constexpr char16_t const SURROGATE_HEADER_HIGH = 0xd800;
    constexpr char16_t const SURROGATE_HEADER_LOW = 0xdc00;
    constexpr char32_t const SURROGATE_MASK_LOW = 0x0000'03ff;
    constexpr char32_t const SURROGATE_MASK_HIGH = 0x000f'fc00;

    if (c < 0x0001'0000) {
        return Utf16Encode{ .number_of_code_units = 1, .encode = { static_cast<char16_t>(c), 0 } };
    } else if (c <= 0x10ffff) {
        char32_t const tmp = c - 0x0001'0000;
        char16_t const low = static_cast<char16_t>(tmp & SURROGATE_MASK_LOW) | SURROGATE_HEADER_LOW;
        char16_t const high = static_cast<char16_t>((tmp & SURROGATE_MASK_HIGH) >> 10) | SURROGATE_HEADER_HIGH;
        return Utf16Encode{ .number_of_code_units = 2, .encode = {high, low}, };
    }
    return Utf16Encode{ .number_of_code_units = 0, .encode = {} };
}

DecodeResult decodeUtf8(std::span<char8_t const> range) {
    enum State {
        Neutral,
        Awaiting1Byte,
        Awaiting2Bytes,
        Awaiting3Bytes,
    } state = State::Neutral;
    constexpr char8_t const MASK_SINGLE_BYTE{ 0b1000'0000 };
    constexpr char8_t const VALUE_SINGLE_BYTE{ 0b0000'0000 };
    constexpr char8_t const MASK_MULTI_BYTE{ 0b1100'0000 };
    constexpr char8_t const VALUE_MULTI_BYTE{ 0b1000'0000 };
    constexpr char8_t const MASK_HEADER_2_BYTES{ 0b1110'0000 };
    constexpr char8_t const VALUE_HEADER_2_BYTES{ 0b1100'0000 };
    constexpr char8_t const MASK_HEADER_3_BYTES{ 0b1111'0000 };
    constexpr char8_t const VALUE_HEADER_3_BYTES{ 0b1110'0000 };
    constexpr char8_t const MASK_HEADER_4_BYTES{ 0b1111'1000 };
    constexpr char8_t const VALUE_HEADER_4_BYTES{ 0b1111'0000 };
    auto check_multi_byte = [](char8_t b) -> bool { return (b & MASK_MULTI_BYTE) == VALUE_MULTI_BYTE; };
    auto add_multi_byte = [](char32_t acc, char8_t b) -> char32_t { return (acc << 6) | (b & (~MASK_MULTI_BYTE)); };
    char32_t acc = 0;
    uint32_t code_units_count = 0;
    constexpr DecodeResult const error = DecodeResult{ .code_units_consumed = 0, .code_point = 0 };
    for (auto const& b : range) {
        ++code_units_count;
        switch (state) {
        case State::Neutral: {
            if ((b & MASK_SINGLE_BYTE) == VALUE_SINGLE_BYTE) {
                return DecodeResult{ .code_units_consumed = code_units_count, .code_point = static_cast<char32_t>(b) };
            } else if (check_multi_byte(b)) {
                // multi byte without header
                return error;
            } else if ((b & MASK_HEADER_2_BYTES) == VALUE_HEADER_2_BYTES) {
                acc = (b & (~MASK_HEADER_2_BYTES));
                state = State::Awaiting1Byte;
            } else if ((b & MASK_HEADER_3_BYTES) == VALUE_HEADER_3_BYTES) {
                acc = (b & (~MASK_HEADER_3_BYTES));
                state = State::Awaiting2Bytes;
            } else if ((b & MASK_HEADER_4_BYTES) == VALUE_HEADER_4_BYTES) {
                acc = (b & (~MASK_HEADER_4_BYTES));
                state = State::Awaiting3Bytes;
            } else {
                // invalid encoding header
                return error;
            }
        } break;
        case State::Awaiting1Byte: {
            if (!check_multi_byte(b)) { return error; }
            acc = add_multi_byte(acc, b);
            return DecodeResult{ .code_units_consumed = code_units_count, .code_point = acc };
        } break;
        case State::Awaiting2Bytes: {
            if (!check_multi_byte(b)) { return error; }
            acc = add_multi_byte(acc, b);
            state = State::Awaiting1Byte;
        } break;
        case State::Awaiting3Bytes: {
            if (!check_multi_byte(b)) { return error; }
            acc = add_multi_byte(acc, b);
            state = State::Awaiting2Bytes;
        } break;
        default: return error;
        }
    }
    return error;
}

Utf8Encode encodeUtf32ToUtf8(char32_t c) {
    constexpr Utf8Encode const error = { .number_of_code_units = 0, .encode = {} };
    if (c < 0x80) {
        return Utf8Encode{ .number_of_code_units = 1, .encode = { static_cast<char8_t>(c)} };
    } else if (c < 0x800) {
        return Utf8Encode{ .number_of_code_units = 2, .encode = {
            static_cast<char8_t>(((c >> 6)  & 0b0001'1111) | 0b1100'0000),
            static_cast<char8_t>((c        & 0b0011'1111) | 0b1000'0000)
        } };
    } else if (c < 0x10000) {
        return Utf8Encode{ .number_of_code_units = 3, .encode = {
            static_cast<char8_t>(((c >> 12) & 0b0000'1111) | 0b1110'0000),
            static_cast<char8_t>(((c >>  6) & 0b0011'1111) | 0b1000'0000),
            static_cast<char8_t>((c         & 0b0011'1111) | 0b1000'0000)
        } };
    } else if (c <= 0x10ffff) {
        return Utf8Encode{ .number_of_code_units = 4, .encode = {
            static_cast<char8_t>(((c >> 18) & 0b0000'0111) | 0b1111'0000),
            static_cast<char8_t>(((c >> 12) & 0b0011'1111) | 0b1000'0000),
            static_cast<char8_t>(((c >>  6) & 0b0011'1111) | 0b1000'0000),
            static_cast<char8_t>((c         & 0b0011'1111) | 0b1000'0000)
        } };
    }
    return error;
}

bool checkValidUtf8(std::span<std::byte const> range) {
    std::span<char8_t const> u8range(reinterpret_cast<char8_t const*>(range.data()), range.size());
    for (;;) {
        if (u8range.empty()) { return true; }
        DecodeResult const r = decodeUtf8(u8range);
        if (r.code_units_consumed == 0) { return false; }
        u8range = u8range.subspan(r.code_units_consumed);
    }
}

bool checkValidUtf8(std::string_view str) {
    return checkValidUtf8(std::span<std::byte const>(reinterpret_cast<std::byte const*>(str.data()), str.size()));
}

std::u8string assumeUtf8(std::string_view str) {
    std::u8string ret;
    ret.reserve(str.size() + 1);
    for (auto const& c : str) { ret.push_back(c); }
    return ret;
}

std::u8string convertToUtf8(std::u16string_view str) {
    std::u8string ret;
    while (!str.empty()) {
        auto const decode_result = decodeUtf16(str);
        str = str.substr(decode_result.code_units_consumed);
        auto const utf8_encode = encodeUtf32ToUtf8(decode_result.code_point);
        ret.append(utf8_encode.encode, utf8_encode.number_of_code_units);
    }
    return ret;
}

std::u16string convertToUtf16(std::u8string_view str) {
    std::u16string ret;
    while (!str.empty()) {
        auto const decode_result = decodeUtf8(str);
        str = str.substr(decode_result.code_units_consumed);
        auto const utf16_encode = encodeUtf32ToUtf16(decode_result.code_point);
        ret.append(utf16_encode.encode, utf16_encode.number_of_code_units);
    }
    return ret;
}

std::wstring convertToWstring(std::u8string_view str) {
    return std::wstring(reinterpret_cast<wchar_t const*>(convertToUtf16(str).c_str()));
}

}
