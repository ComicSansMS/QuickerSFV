#ifndef INCLUDE_GUARD_QUICKER_SFV_PLUGIN_SDK_CPP_INTERFACES_HPP
#define INCLUDE_GUARD_QUICKER_SFV_PLUGIN_SDK_CPP_INTERFACES_HPP
#ifdef __cplusplus

#include <memory>

namespace quicker_sfv {
class ChecksumProvider;
using ChecksumProviderPtr = std::unique_ptr<ChecksumProvider>;
}

struct QuickerSFV_CppPluginLoader {
    virtual quicker_sfv::ChecksumProviderPtr createChecksumProvider() const = 0;
};

#endif
#endif
