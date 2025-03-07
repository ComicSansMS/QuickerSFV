#include <quicker_sfv/string_utilities.hpp>

#include <cassert>
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
        } return error;
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
        }
    }
    return (range.empty()) ? error : DecodeResult{ .code_units_consumed = 1, .code_point = static_cast<char32_t>(range[0]) };
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
    while (!u8range.empty()) {
        DecodeResult const r = decodeUtf8(u8range);
        if (r.code_units_consumed == 0) { return false; }
        u8range = u8range.subspan(r.code_units_consumed);
    }
    return true;
}

bool checkValidUtf8(std::string_view str) {
    return checkValidUtf8(std::span<std::byte const>(reinterpret_cast<std::byte const*>(str.data()), str.size()));
}

bool checkValidUtfWideString(std::span<wchar_t const> range) {
    static_assert((sizeof(wchar_t) == 2) || (sizeof(wchar_t) == 4), "Unexpected wchar_t size");
    if constexpr (sizeof(wchar_t) == 4) {
        std::span<char32_t const> u32range(reinterpret_cast<char32_t const*>(range.data()), range.size());
        for (auto const& c : u32range) {
            if (encodeUtf32ToUtf8(c).number_of_code_units == 0) { return false; }
        }
        return true;
    } else if constexpr (sizeof(wchar_t) == 2) {
        std::span<char16_t const> u16range(reinterpret_cast<char16_t const*>(range.data()), range.size());
        while (!u16range.empty()) {
            DecodeResult const r = decodeUtf16(u16range);
            if (r.code_units_consumed == 0) { return false; }
            u16range = u16range.subspan(r.code_units_consumed);
        }
        return true;
    }
}

bool checkValidUtfWideString(std::wstring_view str) {
    return checkValidUtfWideString(std::span<wchar_t const>(str.begin(), str.end()));
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
        assert(decode_result.code_units_consumed != 0);
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
        assert(decode_result.code_units_consumed != 0);
        str = str.substr(decode_result.code_units_consumed);
        auto const utf16_encode = encodeUtf32ToUtf16(decode_result.code_point);
        ret.append(utf16_encode.encode, utf16_encode.number_of_code_units);
    }
    return ret;
}

std::u8string_view trim(std::u8string_view sv) {
    while (sv.ends_with(u8' ')
        || sv.ends_with(u8'\t')
        || sv.ends_with(u8'\n')
        || sv.ends_with(u8'\r')
        || sv.ends_with(u8'\f')
        || sv.ends_with(u8'\v')) {
        sv = sv.substr(0, sv.size() - 1);
    }
    while (sv.starts_with(u8' ')
        || sv.starts_with(u8'\t')
        || sv.starts_with(u8'\n')
        || sv.starts_with(u8'\r')
        || sv.starts_with(u8'\f')
        || sv.starts_with(u8'\v')) {
        sv = sv.substr(1);
    }
    return sv;
}

std::u8string_view trimAllUtf(std::u8string_view sv) {
    auto is_whitespace = [](char32_t c) -> bool {
        return (c == U'\t') || (c == U'\n') || (c == U'\v') || (c == U'\f') ||
            (c == U'\r') || (c == U' ') || (c == 0x0085) || (c == 0x00A0) ||
            (c == 0x1680) || ((c >= 0x2000) && (c <= 0x200A)) || (c == 0x2028) ||
            (c == 0x2029) || (c == 0x202F) || (c == 0x205F) || (c == 0x3000);
    };
    // trim from beginning
    while (!sv.empty()) {
        auto const d = decodeUtf8(sv);
        if (is_whitespace(d.code_point)) {
            sv.remove_prefix(d.code_units_consumed);
        } else {
            break;
        }
    }
    // trim from back
    while (!sv.empty()) {
        DecodeResult decoded{};
        for (size_t back_index = 1; back_index <= 4; ++back_index) {
            auto const suffix = sv.substr(sv.size() - back_index);
            decoded = decodeUtf8(suffix);
            if (decoded.code_units_consumed != 0) {
                break;
            }
        }
        if (is_whitespace(decoded.code_point)) {
            sv.remove_suffix(decoded.code_units_consumed);
        } else {
            break;
        }
    }
    return sv;
}

}
