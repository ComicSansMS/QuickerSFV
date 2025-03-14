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
