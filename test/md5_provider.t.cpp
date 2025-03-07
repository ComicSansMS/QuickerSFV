#include <quicker_sfv/md5_provider.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/detail/md5.hpp>

#include <test_file_io.hpp>

#include <catch.hpp>

namespace {
std::vector<char> vecFromString(char const* str) {
    std::vector<char> ret;
    ret.insert(ret.end(), str, str + strlen(str));
    return ret;
}
}

TEST_CASE("MD5 Provider")
{
    using quicker_sfv::MD5Provider;
    using quicker_sfv::ChecksumFile;
    auto p = quicker_sfv::createMD5Provider();
    REQUIRE(p);

    SECTION("Capabilities") {
        CHECK(p->getCapabilities() == quicker_sfv::ProviderCapabilities::Full);
    }
    SECTION("Extension and Description") {
        CHECK(p->fileExtensions() == u8"*.md5");
        CHECK(p->fileDescription() == u8"MD5");
    }
    SECTION("Create Hasher") {
        auto h = p->createHasher(quicker_sfv::HasherOptions{ .has_sse42 = false, .has_avx512 = false});
        REQUIRE(h);
        CHECK(dynamic_cast<quicker_sfv::detail::MD5Hasher*>(h.get()));
    }
    SECTION("Digest from String") {
        CHECK(p->digestFromString(u8"14d739518e715e6e61c19eb05f58a8da").toString() ==
            u8"14d739518e715e6e61c19eb05f58a8da");
        CHECK(p->digestFromString(u8"93b885adfe0da089cdf634904fd59f71").toString() ==
            u8"93b885adfe0da089cdf634904fd59f71");
        CHECK_THROWS_AS(p->digestFromString(u8"Some Bogus String"), quicker_sfv::Exception);
    }
    SECTION("Write Checksum File") {
        ChecksumFile f;
        f.addEntry(u8"some/example/path", p->digestFromString(u8"14d739518e715e6e61c19eb05f58a8da"));
        f.addEntry(u8"some_file.rar", p->digestFromString(u8"93b885adfe0da089cdf634904fd59f71"));
        f.addEntry(u8"another_file.txt", p->digestFromString(u8"a6e25eeaf4af08b6baf6b2e31ceccfdb"));
        TestOutput out;
        SECTION("Normal Output") {
            p->writeNewFile(out, f);
            CHECK(out.contents == vecFromString(
                "14d739518e715e6e61c19eb05f58a8da *some/example/path" "\n"
                "93b885adfe0da089cdf634904fd59f71 *some_file.rar"     "\n"
                "a6e25eeaf4af08b6baf6b2e31ceccfdb *another_file.txt"  "\n"));
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
            in = "14d739518e715e6e61c19eb05f58a8da *some/example/path" "\n"
                 "93b885adfe0da089cdf634904fd59f71 *some_file.rar"     "\n"
                 "; comments are ignored"                              "\n"
                 "a6e25eeaf4af08b6baf6b2e31ceccfdb *another_file.txt"  "\n";
            ChecksumFile const f = p->readFromFile(in);
            REQUIRE(f.getEntries().size() == 3);
            CHECK((f.getEntries()[0].digest == p->digestFromString(u8"14d739518e715e6e61c19eb05f58a8da")));
            CHECK(f.getEntries()[0].path == u8"some/example/path");
            CHECK((f.getEntries()[1].digest == p->digestFromString(u8"93b885adfe0da089cdf634904fd59f71")));
            CHECK(f.getEntries()[1].path == u8"some_file.rar");
            CHECK((f.getEntries()[2].digest == p->digestFromString(u8"a6e25eeaf4af08b6baf6b2e31ceccfdb")));
            CHECK(f.getEntries()[2].path == u8"another_file.txt");
        }
        SECTION("Windows Line Endings (CRLF)") {
            TestInput in;
            in = "14d739518e715e6e61c19eb05f58a8da *some/example/path" "\r\n"
                 "93b885adfe0da089cdf634904fd59f71 *some_file.rar"     "\r\n"
                 "; comments are ignored"                              "\r\n"
                 "a6e25eeaf4af08b6baf6b2e31ceccfdb *another_file.txt"  "\r\n";
            ChecksumFile const f = p->readFromFile(in);
            REQUIRE(f.getEntries().size() == 3);
            CHECK((f.getEntries()[0].digest == p->digestFromString(u8"14d739518e715e6e61c19eb05f58a8da")));
            CHECK(f.getEntries()[0].path == u8"some/example/path");
            CHECK((f.getEntries()[1].digest == p->digestFromString(u8"93b885adfe0da089cdf634904fd59f71")));
            CHECK(f.getEntries()[1].path == u8"some_file.rar");
            CHECK((f.getEntries()[2].digest == p->digestFromString(u8"a6e25eeaf4af08b6baf6b2e31ceccfdb")));
            CHECK(f.getEntries()[2].path == u8"another_file.txt");
        }
        SECTION("Read error in file") {
            TestInput in;
            in = "14d739518e715e6e61c19eb05f58a8da *some/example/path" "\r\n"
                 "93b885adfe0da089cdf634904fd59f71 *some_file.rar"     "\r\n"
                 "a6e25eeaf4af08b6baf6b2e31ceccfdb *another_file.txt"  "\r\n";
            in.fault_after = 10;
            CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
        }
        SECTION("Invalid file formats") {
            SECTION("Multiple entries on one line") {
                TestInput in;
                in = "14d739518e715e6e61c19eb05f58a8da *some/example/path"
                     "93b885adfe0da089cdf634904fd59f71 *some_file.rar"     "\n"
                     "a6e25eeaf4af08b6baf6b2e31ceccfdb *another_file.txt"  "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing separator") {
                TestInput in;
                in = "14d739518e715e6e61c19eb05f58a8da *some/example/path" "\n"
                     "93b885adfe0da089cdf634904fd59f71 some_file.rar"      "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing digest #1") {
                TestInput in;
                in = " *some/example/path"                                 "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing digest #2") {
                TestInput in;
                in = "*some/example/path"                                  "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Invalid digest") {
                TestInput in;
                in = "14d739518e715e6e61c19eb05f58a8dz *some/example/path" "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing filename #1") {
                TestInput in;
                in = "14d739518e715e6e61c19eb05f58a8dz *" "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
            SECTION("Missing filename #2") {
                TestInput in;
                in = "14d739518e715e6e61c19eb05f58a8dz *             " "\n";
                CHECK_THROWS_AS(p->readFromFile(in), quicker_sfv::Exception);
            }
        }
        SECTION("Weird inputs") {
            SECTION("Trailing whitespace") {
                TestInput in;
                in = "14d739518e715e6e61c19eb05f58a8da *some/example/path    " "\n";
                auto const f = p->readFromFile(in);
                REQUIRE(f.getEntries().size() == 1);
                CHECK((f.getEntries()[0].digest == p->digestFromString(u8"14d739518e715e6e61c19eb05f58a8da")));
                CHECK(f.getEntries()[0].path == u8"some/example/path");
            }
            SECTION("Trailing whitespace before separator") {
                TestInput in;
                in = "14d739518e715e6e61c19eb05f58a8da      *some/example/path" "\n";
                auto const f = p->readFromFile(in);
                REQUIRE(f.getEntries().size() == 1);
                CHECK((f.getEntries()[0].digest == p->digestFromString(u8"14d739518e715e6e61c19eb05f58a8da")));
                CHECK(f.getEntries()[0].path == u8"some/example/path");
            }
            SECTION("Missing final linebreak") {
                TestInput in;
                in = "14d739518e715e6e61c19eb05f58a8da      *some/example/path";
                auto const f = p->readFromFile(in);
                REQUIRE(f.getEntries().size() == 1);
                CHECK((f.getEntries()[0].digest == p->digestFromString(u8"14d739518e715e6e61c19eb05f58a8da")));
                CHECK(f.getEntries()[0].path == u8"some/example/path");
            }
        }
    }
}
