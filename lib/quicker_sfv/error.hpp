#ifndef INCLUDE_GUARD_QUICKER_SFV_ERROR_HPP
#define INCLUDE_GUARD_QUICKER_SFV_ERROR_HPP

#include <string_view>

namespace quicker_sfv {

enum class Error {
    Failed,
    HasherFailure,
};

class Exception {
    Error m_error;
public:
    explicit Exception(Error e) noexcept;

    std::u8string_view what8() const noexcept;
};

}
#endif
