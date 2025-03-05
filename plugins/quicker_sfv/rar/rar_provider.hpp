#ifndef INCLUDE_GUARD_QUICKER_SFV_PLUGIN_RAR_RAR_PROVIDER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_PLUGIN_RAR_RAR_PROVIDER_HPP

#include <quicker_sfv/rar/rar_export.h>

#include <quicker_sfv/plugin/plugin_sdk.h>

#include <quicker_sfv/checksum_provider.hpp>

namespace quicker_sfv::rar {

class RarProvider final : public ChecksumProvider {
    friend struct RarPluginLoader;
private:
public:
    RarProvider();
    ~RarProvider() override;
    [[nodiscard]] ProviderCapabilities getCapabilities() const override;
    [[nodiscard]] std::u8string_view fileExtensions() const override;
    [[nodiscard]] std::u8string_view fileDescription() const override;
    [[nodiscard]] HasherPtr createHasher(HasherOptions const& hasher_options) const override;
    [[nodiscard]] Digest digestFromString(std::u8string_view str) const override;

    ChecksumFile readFromFile(FileInput& file_input) const override;
    void writeNewFile(FileOutput& file_output, ChecksumFile const& f) const override;
};

}

extern "C" {
QUICKER_SFV_PLUGIN_RAR_EXPORT void QuickerSFV_LoadPlugin_Cpp(QuickerSFV_CppPluginLoader** loader);
}

#endif
