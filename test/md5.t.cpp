#include <quicker_sfv/detail/md5.hpp>

#include <catch.hpp>

TEST_CASE("MD5")
{
    using quicker_sfv::detail::MD5Hasher;
    MD5Hasher hasher;
    SECTION("Hasher") {
        CHECK(hasher.finalize().toString() == u8"d41d8cd98f00b204e9800998ecf8427e");
        hasher.reset();

        std::byte zero[] = { std::byte{ 0x00 } };
        hasher.addData(zero);
        CHECK(hasher.finalize().toString() == u8"93b885adfe0da089cdf634904fd59f71");
        hasher.reset();

        std::byte abc[] = { std::byte{ 0x41 }, std::byte{ 0x42 }, std::byte{ 0x43 } };
        hasher.addData(abc);
        CHECK(hasher.finalize().toString() == u8"902fbdd2b1df0c4f70b4a5d23525e932");
        hasher.reset();

        std::byte data[] = {
            std::byte{ 0x1a }, std::byte{ 0x2b }, std::byte{ 0x3c },
            std::byte{ 0x4f }, std::byte{ 0x5a }, std::byte{ 0x6b },
            std::byte{ 0x7c }, std::byte{ 0x8d }, std::byte{ 0x9e },
            std::byte{ 0xa9 }, std::byte{ 0xb5 }, std::byte{ 0xc3 },
            std::byte{ 0xd9 }, std::byte{ 0xe1 }, std::byte{ 0xff },
            std::byte{ 0x89 }, std::byte{ 0x51 }, std::byte{ 0x4a },
            std::byte{ 0xaa }, std::byte{ 0x55 }, std::byte{ 0xcc }
        };
        hasher.addData(data);
        CHECK(hasher.finalize().toString() == u8"14d739518e715e6e61c19eb05f58a8da");
        hasher.reset();

        hasher.addData(std::span<std::byte const>(data, data + 5));
        CHECK(hasher.finalize().toString() == u8"a6e25eeaf4af08b6baf6b2e31ceccfdb");
        hasher.reset();
        
        hasher.addData(std::span<std::byte const>(data, data + 5));
        hasher.addData(std::span<std::byte const>(data + 5, data + sizeof(data)));
        CHECK(hasher.finalize().toString() == u8"14d739518e715e6e61c19eb05f58a8da");
    }

    SECTION("Digest from string")
    {
        CHECK(MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da").toString() ==
            u8"14d739518e715e6e61c19eb05f58a8da");
        CHECK(MD5Hasher::digestFromString(u8"93b885adfe0da089cdf634904fd59f71").toString() ==
            u8"93b885adfe0da089cdf634904fd59f71");
    }

    SECTION("Digest comparison")
    {
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") ==
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d0")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d1")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d2")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d3")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d4")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d5")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d6")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d7")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d8")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8d9")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8db")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8dc")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8dd")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8de")));
        CHECK((MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8da") !=
            MD5Hasher::digestFromString(u8"14d739518e715e6e61c19eb05f58a8df")));

    }
}
