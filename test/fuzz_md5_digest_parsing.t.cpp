#include <quicker_sfv/md5_provider.hpp>
#include <quicker_sfv/error.hpp>

#include <cassert>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    auto md5 = quicker_sfv::createMD5Provider();
    try {
        std::u8string_view str{reinterpret_cast<char8_t const*>(Data), Size};
        auto const digest = md5->digestFromString(str);
    } catch(quicker_sfv::Exception) {}
    return 0;
}
