#include <quicker_sfv/sfv_provider.hpp>
#include <quicker_sfv/error.hpp>

#include <cassert>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    auto sfv = quicker_sfv::createSfvProvider();
    try {
        std::u8string_view str{reinterpret_cast<char8_t const*>(Data), Size};
        auto const digest = sfv->digestFromString(str);
    } catch(quicker_sfv::Exception) {}
    return 0;
}
