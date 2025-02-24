#ifndef INCLUDE_GUARD_QUICKER_SFV_ERROR_HPP
#define INCLUDE_GUARD_QUICKER_SFV_ERROR_HPP

#include <string_view>

namespace quicker_sfv {

enum class Error {
    Failed,
    HasherFailure,
    ParserError,
    FileIO,
    InvalidArgument,
};

class Exception {
private:
    Error m_error;
private:
    explicit Exception(Error e) noexcept;
public:
    [[nodiscard]] std::u8string_view what8() const noexcept;

    [[nodiscard]] Error code() const noexcept;

    [[noreturn]] friend void throwException(Error e);
};

[[noreturn]] void throwException(Error e);

}
#endif
