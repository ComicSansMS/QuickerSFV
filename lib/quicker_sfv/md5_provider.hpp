#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_PROVIDER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_PROVIDER_HPP

#include <quicker_sfv/checksum_provider.hpp>
#include <quicker_sfv/checksum_file.hpp>

#include <string_view>
#include <memory>

namespace quicker_sfv {

class MD5Provider : public ChecksumProvider {
public:
    friend ChecksumProviderPtr createMD5Provider();
private:
    MD5Provider();
public:
    ~MD5Provider() override;
    [[nodiscard]] std::u8string_view fileExtensions() const override;
    [[nodiscard]] std::u8string_view fileDescription() const override;
    [[nodiscard]] HasherPtr createHasher(HasherOptions const&) const override;
    [[nodiscard]] Digest digestFromString(std::u8string_view str) const override;

    [[nodiscard]] ChecksumFile readFromFile(FileInput& file_input) const override;
    void serialize(FileOutput& file_output, ChecksumFile const& f) const override;
};

ChecksumProviderPtr createMD5Provider();

}

#endif
