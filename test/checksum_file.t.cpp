#include <quicker_sfv/checksum_file.hpp>

#include <test_digest.hpp>

#include <catch.hpp>

TEST_CASE("Checksum File")
{
    using quicker_sfv::ChecksumFile;
    using quicker_sfv::Digest;
    SECTION("Construction") {
        ChecksumFile f;
        CHECK(f.getEntries().empty());
    }
    SECTION("Adding entries") {
        ChecksumFile f;
        f.addEntry(u8"a", TestDigest{ u8"123456" });
        f.addEntry(u8"b", TestDigest{ u8"7890ab" });
        f.addEntry(u8"c", TestDigest{ u8"cdef01" });
        REQUIRE(f.getEntries().size() == 3);
        CHECK(f.getEntries()[0].path == u8"a");
        CHECK(f.getEntries()[1].path == u8"b");
        CHECK(f.getEntries()[2].path == u8"c");
        CHECK(f.getEntries()[0].digest == Digest{ TestDigest{ u8"123456" } });
        CHECK(f.getEntries()[1].digest == Digest{ TestDigest{ u8"7890ab" } });
        CHECK(f.getEntries()[2].digest == Digest{ TestDigest{ u8"cdef01" } });
    }
    SECTION("Sort and clear")
    {
        ChecksumFile f;
        f.addEntry(u8"b", TestDigest{ u8"7890ab" });
        f.addEntry(u8"c", TestDigest{ u8"cdef01" });
        f.addEntry(u8"a", TestDigest{ u8"123456" });
        f.sortEntries();
        REQUIRE(f.getEntries().size() == 3);
        CHECK(f.getEntries()[0].path == u8"a");
        CHECK(f.getEntries()[1].path == u8"b");
        CHECK(f.getEntries()[2].path == u8"c");
        CHECK(f.getEntries()[0].digest == Digest{ TestDigest{ u8"123456" } });
        CHECK(f.getEntries()[1].digest == Digest{ TestDigest{ u8"7890ab" } });
        CHECK(f.getEntries()[2].digest == Digest{ TestDigest{ u8"cdef01" } });
        f.clear();
        CHECK(f.getEntries().empty());
    }
}
