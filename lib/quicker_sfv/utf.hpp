#ifndef INCLUDE_GUARD_QUICKER_SFV_UTF_HPP
#define INCLUDE_GUARD_QUICKER_SFV_UTF_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace quicker_sfv {

/** Result of a decoding operation to UTF-32.
 * The number of bytes consumed in the encoding depends on the source encoding.
 * For UTF-8 it is `code_units_consumed`, for UTF-16 it is `code_units_consumed * 2`.
 * An invalid value is indicated by all fields being set to 0.
 */
struct DecodeResult {
    uint32_t code_units_consumed;       ///< Number of code units consumed in the decoding.
    char32_t code_point;                ///< Decoded UTF-32 code point.

    friend bool operator==(DecodeResult, DecodeResult) noexcept = default;
};

/** Result of a UTF-16 encoding operation.
 * A result may consist of one or two valid code units. Unused code units in the
 * encode field will be set to 0. If multiple fields are used, the most significant
 * byte is stored in index 0.
 * An invalid value is indicated by all fields being set to 0.
 */
struct Utf16Encode {
    uint32_t number_of_code_units;      ///< Number of valid code units in this result.
    char16_t encode[2];                 ///< Encoded UTF-16 code units.

    friend bool operator==(Utf16Encode, Utf16Encode) noexcept = default;
};

/** Result of a UTF-8 encoding operation.
 * A result may consist of one to four valid code units. Unused code units in the
 * encode field will be set to 0. If multiple fields are used, the most significant
 * byte is stored in index 0.
 * An invalid value is indicated by all fields being set to 0.
 */
struct Utf8Encode {
    uint32_t number_of_code_units;      ///< Number of valid code units in this result.
    char8_t encode[4];                  ///< Encoded UTF-8 code units.

    friend bool operator==(Utf8Encode, Utf8Encode) noexcept = default;
};

/** Decode a single code point from a range of UTF-16 code units.
 * @param[in] range A valid UTF-16 string.
 * @return On success, the first decoded UTF-32 code point from the string.
 *         On error, a result with number_of_code_units set to 0.
 */
DecodeResult decodeUtf16(std::span<char16_t const> range);
/** Decode a single code point from a range of potentially invalid UTF-16 code units.
 * This function is to support legacy UCS2-APIs that do not handle surrogate pairs
 * correctly. If a code unit sequence is encountered that is not a valid UTF-16
 * encoding, the individual code units are instead interpreted as raw code points.
 * For example, if the first character in the input range is `0xdc00` (which is only
 * a valid value for the second character in a surrogate pair), the function will
 * return with a code_point of `0xdc00` and code_units_consumed of `1`.
 * @param[in] range A potentially invalid UTF-16 string.
 * @return A UTF-32 code point, derived as described above.
 *         The only error condition is an empty input range, on which a result with
 *         number_of_code_units set to 0 is returned.
 */
DecodeResult decodeUtf16_non_strict(std::span<char16_t const> range);

/** Encode a single UTF-32 code point to UTF-16.
 * @param[in] c A UTF-32 code point (`0x00000000`-`0x0010FFFF`).
 * @return The UTF-16 encoded result. If the input value it out of range,
 *         returns a result with number_of_code_units set to 0.
 */
Utf16Encode encodeUtf32ToUtf16(char32_t c);

/** Decode a single code point from a range of UTF-8 code units.
 * @param[in] range A valid UTF-8 string.
 * @return On success, the first decoded UTF-32 code point from the string.
 *         On error, a result with number_of_code_units set to 0.
 */
DecodeResult decodeUtf8(std::span<char8_t const> range);

/** Encode a single UTF-32 code point to UTF-8.
 * @param[in] c A UTF-32 code point (`0x00000000`-`0x0010FFFF`).
 * @return The UTF-8 encoded result. If the input value it out of range,
 *         returns a result with number_of_code_units set to 0.
 */
Utf8Encode encodeUtf32ToUtf8(char32_t c);

/** Checks whether a byte range contains a valid UTF-8 encoded string.
 * The empty string is considered valid.
 */
bool checkValidUtf8(std::span<std::byte const> range);
/** Checks whether a string_view contains a valid UTF-8 encoded string.
 * The empty string is considered valid.
 */
bool checkValidUtf8(std::string_view str);

/** Checks whether a wchar_t range contains a valid UTF-16/UTF-32 encoded string.
 * The empty string is considered valid.
 * @note Depending on the platform, wchar_t can be 2 or 4 bytes in size.
 */
bool checkValidUtfWideString(std::span<wchar_t const> range);
/** Checks whether a wstring_view contains a valid UTF-16 encoded string.
 * The empty string is considered valid.
 * * @note Depending on the platform, wchar_t can be 2 or 4 bytes in size.
 */
bool checkValidUtfWideString(std::wstring_view str);

/** Creates a u8string from a string_view without validating the encoding.
 * @pre `checkValidUtf8(str) == true`.
 * @attention Incorrect use of this function may lead to undefined behavior.
 */
std::u8string assumeUtf8(std::string_view str);

/** Converts a UTF-16 string to UTF-8.
 * @pre str must contain a valid UTF-16 encoded string.
 */
std::u8string convertToUtf8(std::u16string_view str);

/** Converts a UTF-8 string to UTF-16.
 * @pre str must contain a valid UTF-8 encoded string.
 */
std::u16string convertToUtf16(std::u8string_view str);

}
#endif
