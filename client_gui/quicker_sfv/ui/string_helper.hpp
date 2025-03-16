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
#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_STRING_HELPER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_STRING_HELPER_HPP

#include <string>

#include <Windows.h>

namespace quicker_sfv::gui {

inline char16_t const* assumeUtf16(LPCWSTR z_str) {
    return reinterpret_cast<char16_t const*>(z_str);
}

inline LPCWSTR toWcharStr(std::u16string const& u16str) {
    return reinterpret_cast<LPCWSTR>(u16str.c_str());
}

inline LPCSTR toStr(std::u8string const& u8str) {
    return reinterpret_cast<LPCSTR>(u8str.c_str());
}

LPCTSTR formatString(LPTSTR out_buffer, size_t buffer_size, LPCTSTR format, ...);
std::u16string formatString(size_t buffer_size, LPCTSTR format, ...);

std::u16string extractBasePathFromFilePath(std::u16string_view checksum_file_path);
std::u16string resolvePath(std::u16string_view checksum_file_path, std::u8string_view relative_path);

}

#endif
