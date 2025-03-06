#include <quicker_sfv/quicker_sfv.hpp>

#include <catch.hpp>

TEST_CASE("QuickerSFV")
{
    SECTION("CPU Features") {
        CHECK_NOFAIL(quicker_sfv::supportsSse42());
        CHECK_NOFAIL(quicker_sfv::supportsAvx512());
    }

}
