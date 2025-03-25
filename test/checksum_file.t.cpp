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
        CHECK(f.getEntries()[0].display == u8"a");
        CHECK(f.getEntries()[1].display == u8"b");
        CHECK(f.getEntries()[2].display == u8"c");
        CHECK((f.getEntries()[0].digest == Digest{ TestDigest{ u8"123456" } }));
        CHECK((f.getEntries()[1].digest == Digest{ TestDigest{ u8"7890ab" } }));
        CHECK((f.getEntries()[2].digest == Digest{ TestDigest{ u8"cdef01" } }));
        REQUIRE(f.getEntries()[0].data.size() == 1);
        CHECK(f.getEntries()[0].data[0].path == f.getEntries()[0].display);
        CHECK(f.getEntries()[0].data[0].data_offset == 0);
        CHECK(f.getEntries()[0].data[0].data_size == -1);
        REQUIRE(f.getEntries()[1].data.size() == 1);
        CHECK(f.getEntries()[1].data[0].path == f.getEntries()[1].display);
        CHECK(f.getEntries()[1].data[0].data_offset == 0);
        CHECK(f.getEntries()[1].data[0].data_size == -1);
        REQUIRE(f.getEntries()[2].data.size() == 1);
        CHECK(f.getEntries()[2].data[0].path == f.getEntries()[2].display);
        CHECK(f.getEntries()[2].data[0].data_offset == 0);
        CHECK(f.getEntries()[2].data[0].data_size == -1);
    }
    SECTION("Sort and clear")
    {
        ChecksumFile f;
        f.addEntry(u8"b", TestDigest{ u8"7890ab" });
        f.addEntry(u8"c", TestDigest{ u8"cdef01" });
        f.addEntry(u8"a", TestDigest{ u8"123456" });
        f.sortEntries();
        REQUIRE(f.getEntries().size() == 3);
        CHECK(f.getEntries()[0].display == u8"a");
        CHECK(f.getEntries()[1].display == u8"b");
        CHECK(f.getEntries()[2].display == u8"c");
        CHECK((f.getEntries()[0].digest == Digest{ TestDigest{ u8"123456" } }));
        CHECK((f.getEntries()[1].digest == Digest{ TestDigest{ u8"7890ab" } }));
        CHECK((f.getEntries()[2].digest == Digest{ TestDigest{ u8"cdef01" } }));
        f.clear();
        CHECK(f.getEntries().empty());
    }
    SECTION("Extended add entry")
    {
        ChecksumFile f;

        f.addEntry(TestDigest{ u8"7890ab" }, u8"display",
            {
                ChecksumFile::DataPortion{ u8"f1", 1, 100 },
                ChecksumFile::DataPortion{ u8"f2", 55, 200 },
                ChecksumFile::DataPortion{ u8"f3", 102, 333 },
            });
        REQUIRE(f.getEntries().size() == 1);
        CHECK((f.getEntries()[0].digest == Digest{ TestDigest{ u8"7890ab" } }));
        CHECK(f.getEntries()[0].display == u8"display");
        REQUIRE(f.getEntries()[0].data.size() == 3);
        CHECK(f.getEntries()[0].data[0].path == u8"f1");
        CHECK(f.getEntries()[0].data[0].data_offset == 1);
        CHECK(f.getEntries()[0].data[0].data_size == 100);

        CHECK(f.getEntries()[0].data[1].path == u8"f2");
        CHECK(f.getEntries()[0].data[1].data_offset == 55);
        CHECK(f.getEntries()[0].data[1].data_size == 200);

        CHECK(f.getEntries()[0].data[2].path == u8"f3");
        CHECK(f.getEntries()[0].data[2].data_offset == 102);
        CHECK(f.getEntries()[0].data[2].data_size == 333);
    }
}
