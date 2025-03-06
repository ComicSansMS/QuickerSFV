#include <fast_crc32.hpp>

#include <algorithm>
#include <limits>
#include <iterator>
#include <random>
#include <span>
#include <vector>

#include <catch.hpp>

namespace {

void test_crc32(bool use_sse42, bool use_avx512) {
    using quicker_sfv::crc::crc32;
    auto crc_helper = [=](std::span<char const> data) {
        return crc32(data.data(), data.size() - 1, 0, use_sse42, use_avx512);
    };
    CHECK(crc_helper("") == 0);
    CHECK(crc_helper("\0") == 0xD202EF8D);
    CHECK(crc_helper("Hello World!") == 0x1C291CA3);
    constexpr std::mt19937::result_type const seed_value = 0x1234567;
    std::mt19937 mt{seed_value};
    std::vector<char> random_data;
    
    constexpr size_t const data_size_sse = 133;
    static_assert((data_size_sse % 16 != 0) && (data_size_sse > 64));
    std::generate_n(std::back_inserter(random_data), data_size_sse, [&mt]() { return static_cast<char>(mt() % 256); });
    CHECK(crc_helper(random_data) == 0xbeba3e51);
    random_data.clear();

    constexpr size_t const data_size_avx = 2007;
    static_assert((data_size_avx % 64 != 0) && (data_size_avx > 256));
    std::generate_n(std::back_inserter(random_data), data_size_avx, [&mt]() { return static_cast<char>(mt() % 256); });
    CHECK(crc_helper(random_data) == 0x1ac55393);
}

}

TEST_CASE("Fast CRC32")
{
    using namespace quicker_sfv::crc;
    bool const supports_sse42 = supportsSse42();
    bool const supports_avx512 = supportsAvx512();
    SECTION("No acceleration") {
        test_crc32(false, false);
    }
    SECTION("SSE 4.2") {
        test_crc32(supports_sse42, false);
    }
    SECTION("AVX-512") {
        test_crc32(supports_sse42, supports_avx512);
    }
    SECTION("AVX-512 only (which does not make sense on real machines, but whatevs)") {
        test_crc32(false, supports_avx512);
    }
}
