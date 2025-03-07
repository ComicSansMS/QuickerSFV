#include <quicker_sfv/md5_provider.hpp>

#include <test_file_io.hpp>

#include <quicker_sfv/error.hpp>

#include <cassert>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    auto md5 = quicker_sfv::createMD5Provider();
    TestInput in;
    in = std::string_view(reinterpret_cast<char const*>(Data), Size);
    try {
        auto f = md5->readFromFile(in);
    } catch(quicker_sfv::Exception) {}
    return 0;
}
