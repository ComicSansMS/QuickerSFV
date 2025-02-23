#include <quicker_sfv/error.hpp>

namespace quicker_sfv {

Exception::Exception(Error e) noexcept
    :m_error(e)
{
}

std::u8string_view Exception::what8() const noexcept {
    switch (m_error) {
    case Error::HasherFailure: [[fallthrough]];
    case Error::Failed: [[fallthrough]];
    default: return u8"Failed";
    }
}


}
