#ifndef INCLUDE_GUARD_QUICKER_SFV_SFV_PROVIDER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_SFV_PROVIDER_HPP

#include <quicker_sfv/checksum_provider.hpp>
#include <quicker_sfv/checksum_file.hpp>

#include <string_view>
#include <memory>

namespace quicker_sfv {

/** Support for `*.sfv` files.
 * One line per file. Each line ends with a Crc32 checksum, preceded by the relative
 * path of the file.
 * File encoding must be UTF-8. Line endings must be either CRLF or LF on read
 * and will always be LF on write.
 */
class SfvProvider : public ChecksumProvider {
public:
    friend ChecksumProviderPtr createSfvProvider();
private:
    SfvProvider();
public:
    ~SfvProvider() override;
    [[nodiscard]] ProviderCapabilities getCapabilities() const noexcept override;
    [[nodiscard]] std::u8string_view fileExtensions() const noexcept override;
    [[nodiscard]] std::u8string_view fileDescription() const noexcept override;
    [[nodiscard]] HasherPtr createHasher(HasherOptions const& hasher_options) const override;
    [[nodiscard]] Digest digestFromString(std::u8string_view str) const override;

    [[nodiscard]] ChecksumFile readFromFile(FileInput& file_input) const override;
    void writeNewFile(FileOutput& file_output, ChecksumFile const& f) const override;
};

/** Creates an SfvProvider.
 */
ChecksumProviderPtr createSfvProvider();

}

#endif
