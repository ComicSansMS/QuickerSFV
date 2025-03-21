/*
 *   QuickerSFV - A fast checksum verifier
 *   Copyright (C) 2025  Andreas Weis (quickersfv@andreas-weis.net)
 *
 *   This file is part of QuickerSFV.
 *
 *   QuickerSFV is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   QuickerSFV is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <fast_crc32/fast_crc32.hpp>

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
        return crc32(data.data(), data.size() - 1, 0, use_avx512, use_sse42);
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
