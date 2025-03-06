#ifndef INCLUDE_GUARD_QUICKER_SFV_TESTING_TEST_DIGEST_HPP
#define INCLUDE_GUARD_QUICKER_SFV_TESTING_TEST_DIGEST_HPP

#include <quicker_sfv/digest.hpp>

#include <string>

struct TestDigest {
    std::u8string s;

    std::u8string toString() const { return s; }
    friend bool operator==(TestDigest const&, TestDigest const&) noexcept = default;
};

static_assert(quicker_sfv::IsDigest<TestDigest>);

#endif
