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
