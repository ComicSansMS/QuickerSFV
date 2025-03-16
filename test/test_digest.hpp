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
