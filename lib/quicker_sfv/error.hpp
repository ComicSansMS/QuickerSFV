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
#ifndef INCLUDE_GUARD_QUICKER_SFV_ERROR_HPP
#define INCLUDE_GUARD_QUICKER_SFV_ERROR_HPP

#include <source_location>
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

#define QUICKER_SFV_ERROR_USE_SOURCE_LOCATION 0

/** Raise an exception with the provided error code.
 */
[[noreturn]] void throwException(Error e
#if QUICKER_SFV_ERROR_USE_SOURCE_LOCATION
    , std::source_location = std::source_location::current()
#endif
);

/** Exception.
 * Exceptions can not be constructed directly but must be raised via the
 * throwException() function.
 */
class Exception: public std::exception {
private:
    Error m_error;
#if QUICKER_SFV_ERROR_USE_SOURCE_LOCATION
    std::source_location m_sourceLocation;
#endif
private:
    explicit Exception(Error e
#if QUICKER_SFV_ERROR_USE_SOURCE_LOCATION
        , std::source_location&& l
#endif
    ) noexcept;
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

    friend void throwException(Error e
#if QUICKER_SFV_ERROR_USE_SOURCE_LOCATION
        , std::source_location
#endif
    );
};


}
#endif
