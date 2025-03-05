#ifndef INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_PROVIDER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_PROVIDER_HPP

#include <quicker_sfv/checksum_file.hpp>
#include <quicker_sfv/digest.hpp>
#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/hasher.hpp>

#include <memory>
#include <string_view>

namespace quicker_sfv {

class ChecksumProvider;

using HasherPtr = std::unique_ptr<Hasher>;
using ChecksumProviderPtr = std::unique_ptr<ChecksumProvider>;

struct HasherOptions {
    bool has_sse42;
    bool has_avx512;
};

enum class ProviderCapabilities {
    Full,
};

class ChecksumProvider {
public:
    ChecksumProvider& operator=(ChecksumProvider&&) = delete;

    virtual ~ChecksumProvider() = 0;
    [[nodiscard]] virtual ProviderCapabilities getCapabilities() const = 0;
    [[nodiscard]] virtual std::u8string_view fileExtensions() const = 0;
    [[nodiscard]] virtual std::u8string_view fileDescription() const = 0;
    [[nodiscard]] virtual HasherPtr createHasher(HasherOptions const& hasher_options) const = 0;
    [[nodiscard]] virtual Digest digestFromString(std::u8string_view str) const = 0;

    virtual ChecksumFile readFromFile(FileInput& file_input) const = 0;
    virtual void writeNewFile(FileOutput& file_output, ChecksumFile const& f) const = 0;
};

}

#endif
