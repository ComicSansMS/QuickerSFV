#include <quicker_sfv/sfv_provider.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/detail/crc32.hpp>

#include <test_file_io.hpp>

#include <catch.hpp>

namespace {
std::vector<char> vecFromString(char const* str) {
    std::vector<char> ret;
    ret.insert(ret.end(), str, str + strlen(str));
    return ret;
}
}

TEST_CASE("Sfv Provider")
{
    using quicker_sfv::SfvProvider;
    using quicker_sfv::ChecksumFile;
    auto p = quicker_sfv::createSfvProvider();
    REQUIRE(p);

    SECTION("Capabilities") {
        CHECK(p->getCapabilities() == quicker_sfv::ProviderCapabilities::Full);
    }
    SECTION("Extension and Description") {
        CHECK(p->fileExtensions() == u8"*.sfv");
        CHECK(p->fileDescription() == u8"Sfv File");
    }
    SECTION("Create Hasher") {
        auto h = p->createHasher(quicker_sfv::HasherOptions{ .has_sse42 = false, .has_avx512 = false});
        REQUIRE(h);
        CHECK(dynamic_cast<quicker_sfv::detail::Crc32Hasher*>(h.get()));
    }
    SECTION("Digest from String") {
        CHECK(p->digestFromString(u8"b0c3bbc7").toString() == u8"b0c3bbc7");
        CHECK(p->digestFromString(u8"89ABCDEF").toString() == u8"89abcdef");
        CHECK_THROWS_AS(p->digestFromString(u8"Some Bogus String"), quicker_sfv::Exception);
    }
    SECTION("Write Checksum File") {
        ChecksumFile f;
        f.addEntry(u8"some/example/path", p->digestFromString(u8"b0c3bbc7"));
        f.addEntry(u8"some_file.rar", p->digestFromString(u8"4a6fa7d5"));
        f.addEntry(u8"another_file.txt", p->digestFromString(u8"9abcdef0"));
        TestOutput out;
        SECTION("Normal Output") {
            p->writeNewFile(out, f);
            CHECK(out.contents == vecFromString(
                "some/example/path b0c3bbc7" "\n"
                "some_file.rar 4a6fa7d5"     "\n"
                "another_file.txt 9abcdef0"  "\n"));
        }
        SECTION("Fault during first write") {
            out.fault_after = 10;
            CHECK_THROWS_AS(p->writeNewFile(out, f), quicker_sfv::Exception);
        }
        SECTION("Fault during second write") {
            out.fault_after = 70;
            CHECK_THROWS_AS(p->writeNewFile(out, f), quicker_sfv::Exception);
        }
    }
    SECTION("Read Checksum File") {
        SECTION("Linux Line Endings (LF)") {
            TestInput in;
            in = "some/example/path b0c3bbc7" "\n"
                 "; comments are ignored"     "\n"
                 "some_file.rar 4a6fa7d5"     "\n"
                 "another_file.txt 9abcdef0"  "\n";
            ChecksumFile const f = p->readFromFile(in);
            REQUIRE(f.getEntries().size() == 3);
            CHECK((f.getEntries()[0].digest == p->digestFromString(u8"b0c3bbc7")));
            CHECK(f.getEntries()[0].path == u8"some/example/path");
            CHECK((f.getEntries()[1].digest == p->digestFromString(u8"4a6fa7d5")));
            CHECK(f.getEntries()[1].path == u8"some_file.rar");
            CHECK((f.getEntries()[2].digest == p->digestFromString(u8"9abcdef0")));
            CHECK(f.getEntries()[2].path == u8"another_file.txt");
        }
        SECTION("Windows Line Endings (CRLF)") {
            TestInput in;
            in = "some/example/path b0c3bbc7" "\r\n"
                 "; comments are ignored"     "\r\n"
                 "some_file.rar 4a6fa7d5"     "\r\n"
                 "another_file.txt 9abcdef0"  "\r\n";
            ChecksumFile const f = p->readFromFile(in);
            REQUIRE(f.getEntries().size() == 3);
            CHECK((f.getEntries()[0].digest == p->digestFromString(u8"b0c3bbc7")));
            CHECK(f.getEntries()[0].path == u8"some/example/path");
            CHECK((f.getEntries()[1].digest == p->digestFromString(u8"4a6fa7d5")));
            CHECK(f.getEntries()[1].path == u8"some_file.rar");
            CHECK((f.getEntries()[2].digest == p->digestFromString(u8"9abcdef0")));
            CHECK(f.getEntries()[2].path == u8"another_file.txt");
        }
        SECTION("Read error in file") {
            TestInput in;
            in = "some/example/path b0c3bbc7" "\n"
                 "; comments are ignored"     "\n"
                 "some_file.rar 4a6fa7d5"     "\n"
                 "another_file.txt 9abcdef0"  "\n";
            in.fault_after = 10;
            CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
        }
        SECTION("Invalid file formats") {
            SECTION("Missing separator") {
                TestInput in;
                in = "some/example/pathb0c3bbc7" "\n"
                     "some_file.rar 4a6fa7d5"    "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing digest #1") {
                TestInput in;
                in = "some/example/path" "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing digest #2") {
                TestInput in;
                in = "some/example/path " "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Invalid digest") {
                TestInput in;
                in = "some/example/path b0c3bbcz" "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing filename #1") {
                TestInput in;
                in = " 4a6fa7d5" "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing filename #2") {
                TestInput in;
                in = "   4a6fa7d5" "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
        }
        SECTION("Weird inputs") {
            SECTION("Multiple entries on one line") {
                TestInput in;
                in = "some/example/path b0c3bbc7" "\n"
                    "; comments are ignored"     "\n"
                    "some_file.rar 4a6fa7d5 "
                    "another_file.txt 9abcdef0"  "\n";
                auto f = p->readFromFile(in);
                REQUIRE(f.getEntries().size() == 2);
                CHECK((f.getEntries()[0].digest == p->digestFromString(u8"b0c3bbc7")));
                CHECK(f.getEntries()[0].path == u8"some/example/path");
                CHECK((f.getEntries()[1].digest == p->digestFromString(u8"9abcdef0")));
                CHECK(f.getEntries()[1].path == u8"some_file.rar 4a6fa7d5 another_file.txt");
            }
            SECTION("Trailing whitespace") {
                TestInput in;
                in = "some_file.rar 4a6fa7d5           " "\n";
                auto const f = p->readFromFile(in);
                REQUIRE(f.getEntries().size() == 1);
                CHECK((f.getEntries()[0].digest == p->digestFromString(u8"4a6fa7d5")));
                CHECK(f.getEntries()[0].path == u8"some_file.rar");
            }
            SECTION("Trailing whitespace before separator") {
                TestInput in;
                in = "some_file.rar                4a6fa7d5" "\n";
                auto const f = p->readFromFile(in);
                REQUIRE(f.getEntries().size() == 1);
                CHECK((f.getEntries()[0].digest == p->digestFromString(u8"4a6fa7d5")));
                CHECK(f.getEntries()[0].path == u8"some_file.rar");
            }
            SECTION("Missing final linebreak") {
                TestInput in;
                in = "some_file.rar 4a6fa7d5";
                auto const f = p->readFromFile(in);
                REQUIRE(f.getEntries().size() == 1);
                CHECK((f.getEntries()[0].digest == p->digestFromString(u8"4a6fa7d5")));
                CHECK(f.getEntries()[0].path == u8"some_file.rar");
            }
        }
    }
}
