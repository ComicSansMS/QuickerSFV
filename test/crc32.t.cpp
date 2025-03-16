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
#include <quicker_sfv/detail/crc32.hpp>

#include <quicker_sfv/error.hpp>

#include <span>

#include <catch.hpp>

TEST_CASE("CRC32")
{
    using quicker_sfv::detail::Crc32Hasher;

    SECTION("Hasher") {
        Crc32Hasher hasher{ quicker_sfv::HasherOptions{} };
        CHECK(hasher.finalize().toString() == u8"00000000");
        hasher.reset();

        std::byte const data[] = {
            std::byte{ 0x1a }, std::byte{ 0x2b }, std::byte{ 0x3c },
            std::byte{ 0x4f }, std::byte{ 0x5a }, std::byte{ 0x6b },
            std::byte{ 0x7c }, std::byte{ 0x8d }, std::byte{ 0x9e }
        };
        hasher.addData(data);
        CHECK(hasher.finalize().toString() == u8"b0c3bbc7");
        hasher.reset();

        hasher.addData(std::span<std::byte const>(data, data + 5));
        CHECK(hasher.finalize().toString() == u8"4a6fa7d5");
        hasher.reset();

        hasher.addData(std::span<std::byte const>(data, data + 5));
        hasher.addData(std::span<std::byte const>(data + 5, data + sizeof(data)));
        CHECK(hasher.finalize().toString() == u8"b0c3bbc7");
    }

    SECTION("Digest from string") {
        CHECK(Crc32Hasher::digestFromString(u8"b0c3bbc7").toString() == u8"b0c3bbc7");
        CHECK(Crc32Hasher::digestFromString(u8"01234567").toString() == u8"01234567");
        CHECK(Crc32Hasher::digestFromString(u8"89ABCDEF").toString() == u8"89abcdef");
        CHECK(Crc32Hasher::digestFromString(u8"89abcdef").toString() == u8"89abcdef");
        CHECK_THROWS_AS(Crc32Hasher::digestFromString(u8"Some Bogus String"), quicker_sfv::Exception);
        CHECK_THROWS_AS(Crc32Hasher::digestFromString(u8"89abcdez"), quicker_sfv::Exception);
    }

    SECTION("Digest from raw") {
        CHECK((Crc32Hasher::digestFromRaw(0x12345678) == Crc32Hasher::digestFromString(u8"12345678")));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) == Crc32Hasher::digestFromString(u8"9abcdef0")));
    }

    SECTION("Digest Equality and Inequality") {
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef1)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef2)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef3)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef4)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef5)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef6)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef7)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef8)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdef9)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdefa)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdefb)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdefc)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdefd)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdefe)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) != Crc32Hasher::digestFromRaw(0x9abcdeff)));
        CHECK((Crc32Hasher::digestFromRaw(0x9abcdef0) == Crc32Hasher::digestFromRaw(0x9abcdef0)));
    }
}
