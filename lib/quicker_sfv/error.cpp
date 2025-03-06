#include <quicker_sfv/error.hpp>

namespace quicker_sfv {

Exception::Exception(Error e) noexcept
    :m_error(e)
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

[[noreturn]] void throwException(Error e) {
    throw Exception(e);
}

}
