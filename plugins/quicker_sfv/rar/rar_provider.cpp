#include <quicker_sfv/rar/rar_provider.hpp>

#include <quicker_sfv/error.hpp>
#include <windows.h>

void QuickerSFV_LoadPlugin_Cpp() {
    MessageBoxA(nullptr, "Hey!", "", MB_OK);
}

namespace quicker_sfv::rar {

RarProvider::RarProvider() = default;
RarProvider::~RarProvider() = default;
ProviderCapabilities RarProvider::getCapabilities() const {
    return ProviderCapabilities::VerifyOnly;
}

std::u8string_view RarProvider::fileExtensions() const {
    return u8"*.rar";
}

std::u8string_view RarProvider::fileDescription() const {
    return u8"RAR Archive";
}

HasherPtr RarProvider::createHasher(HasherOptions const& hasher_options) const {
    return nullptr;
}

Digest RarProvider::digestFromString(std::u8string_view str) const {
    return {};
}

ChecksumFile RarProvider::readFromFile(FileInput& file_input) const {
    return {};
}

void RarProvider::writeNewFile(FileOutput& file_output, ChecksumFile const& f) const {
    throwException(Error::Failed);
}

struct RarPluginLoader : QuickerSFV_CppPluginLoader {
    quicker_sfv::ChecksumProviderPtr createChecksumProvider() const override {
        return std::make_unique<RarProvider>();
    }
};

}

extern "C"
void QuickerSFV_LoadPlugin_Cpp(QuickerSFV_CppPluginLoader** loader) {
    static quicker_sfv::rar::RarPluginLoader l;
    *loader = &l;
}

