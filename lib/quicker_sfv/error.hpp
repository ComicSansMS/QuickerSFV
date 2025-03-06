#ifndef INCLUDE_GUARD_QUICKER_SFV_ERROR_HPP
#define INCLUDE_GUARD_QUICKER_SFV_ERROR_HPP

#include <stdexcept>
#include <string_view>

namespace quicker_sfv {

/** Error codes.
 */
enum class Error {
    Failed          = 1,    ///< A requested operation failed to complete.
    SystemError     = 3,    ///< An error in a lower-level system facility.
    FileIO          = 5,    ///< Error while performing file I/O.
    HasherFailure   = 10,   ///< Error in a lower-level hashing facility.
    ParserError     = 11,   ///< Error while parsing a checksum file.
    PluginError     = 12,   ///< Error raised by an ffi-plugin.
};

/** Raise an exception with the provided error code.
 */
[[noreturn]] void throwException(Error e);

/** Exception.
 * Exceptions can not be constructed directly but must be raised via the
 * throwException() function.
 */
class Exception: public std::exception {
private:
    Error m_error;
private:
    explicit Exception(Error e) noexcept;
public:
    /** Retrieve the utf-8 error message associated with the exception.
     * @note what() does not give a meaningful error message for this type of
     *       exceptions. Make sure to always use what8() instead.
     */
    [[nodiscard]] std::u8string_view what8() const noexcept;

    /** Retrieve the error code associated with the exception.
     * This is intended for handling the exception programmatically. For a readable
     * description of the error that can be displayed to users, use what8() instead.
     */
    [[nodiscard]] Error code() const noexcept;

    friend void throwException(Error e);
};


}
#endif
