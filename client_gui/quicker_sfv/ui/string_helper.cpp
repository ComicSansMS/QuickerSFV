#include <quicker_sfv/ui/string_helper.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/utf.hpp>

#include <strsafe.h>

namespace quicker_sfv::gui {

LPCTSTR formatString(LPTSTR out_buffer, size_t buffer_size, LPCTSTR format, ...) {
    va_list args;
    va_start(args, format);
    HRESULT hres = StringCchVPrintf(out_buffer, buffer_size, format, args);
    if ((hres != S_OK) && (hres != STRSAFE_E_INSUFFICIENT_BUFFER)) {
        throwException(Error::Failed);
    }
    va_end(args);
    return out_buffer;
}

std::u16string formatString(size_t buffer_size, LPCTSTR format, ...) {
    va_list args;
    va_start(args, format);
    std::u16string ret;
    ret.resize(buffer_size);
    size_t remain = 0;
    HRESULT hres = StringCchVPrintfEx(reinterpret_cast<TCHAR*>(ret.data()), buffer_size - 1, nullptr, &remain, 0, format, args);
    if ((hres != S_OK) && (hres != STRSAFE_E_INSUFFICIENT_BUFFER)) {
        throwException(Error::Failed);
    }
    va_end(args);
    ret.resize(ret.size() - remain - 1);
    return ret;
}


std::u16string extractBasePathFromChecksumFilePath(std::u16string_view checksum_file_path) {
    auto it_slash = std::find(checksum_file_path.rbegin(), checksum_file_path.rend(), u'\\').base();
    return std::u16string{ checksum_file_path.substr(0, std::distance(checksum_file_path.begin(), it_slash)) };
}

std::u16string resolvePath(std::u16string_view checksum_file_path, std::u8string_view relative_path) {
    std::u16string const file_path = extractBasePathFromChecksumFilePath(checksum_file_path) + convertToUtf16(relative_path);
    wchar_t empty;
    DWORD const required_size = GetFullPathName(toWcharStr(file_path), 0, &empty, nullptr);
    wchar_t* buffer = new wchar_t[required_size];
    GetFullPathName(toWcharStr(file_path), required_size, buffer, nullptr);
    std::u16string ret{ assumeUtf16(buffer) };
    delete buffer;
    return ret;
}

}
