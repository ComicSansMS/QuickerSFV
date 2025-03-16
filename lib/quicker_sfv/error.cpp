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
#include <quicker_sfv/error.hpp>

namespace quicker_sfv {

Exception::Exception(Error e
#if QUICKER_SFV_ERROR_USE_SOURCE_LOCATION
    , std::source_location&& l
#endif
) noexcept
    :m_error(e)
#if QUICKER_SFV_ERROR_USE_SOURCE_LOCATION
    , m_sourceLocation(std::move(l))
#endif
{
}

[[nodiscard]] std::u8string_view Exception::what8() const noexcept {
    switch (m_error) {
    case Error::SystemError:
        return u8"System error";
    case Error::FileIO:
        return u8"File i/o error";
    case Error::HasherFailure:
        return u8"Failed to hash";
    case Error::ParserError:
        return u8"Invalid file format";
    case Error::PluginError:
        return u8"Plugin failed";
    case Error::Failed: [[fallthrough]];
    default: return u8"Failed";
    }
}

[[nodiscard]] Error Exception::code() const noexcept {
    return m_error;
}

[[noreturn]] void throwException(Error e
#if QUICKER_SFV_ERROR_USE_SOURCE_LOCATION
    , std::source_location l
#endif
) {
    throw Exception(e
#if QUICKER_SFV_ERROR_USE_SOURCE_LOCATION
        , std::move(l)
#endif
    );
}

}
