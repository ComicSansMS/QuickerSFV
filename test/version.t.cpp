#include <quicker_sfv/version.hpp>

#include <catch.hpp>

TEST_CASE("Version")
{
    quicker_sfv::Version const version = quicker_sfv::getVersion();
    CHECK(version.major >= 0);
    CHECK(version.major < 65535);
    CHECK(version.minor >= 0);
    CHECK(version.minor < 65535);
    CHECK(version.patch >= 0);
    CHECK(version.patch < 65535);
}
