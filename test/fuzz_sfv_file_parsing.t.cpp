#include <quicker_sfv/sfv_provider.hpp>

#include <test_file_io.hpp>

#include <quicker_sfv/error.hpp>

#include <cassert>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    auto sfv = quicker_sfv::createSfvProvider();
    TestInput in;
    in = std::string_view(reinterpret_cast<char const*>(Data), Size);
    try {
        auto f = sfv->readFromFile(in);
    } catch(quicker_sfv::Exception) {}
    return 0;
}
