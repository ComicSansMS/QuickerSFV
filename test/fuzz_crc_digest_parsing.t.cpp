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
#include <quicker_sfv/sfv_provider.hpp>
#include <quicker_sfv/error.hpp>

#include <cassert>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    auto sfv = quicker_sfv::createSfvProvider();
    try {
        std::u8string_view str{reinterpret_cast<char8_t const*>(Data), Size};
        auto const digest = sfv->digestFromString(str);
    } catch(quicker_sfv::Exception) {}
    return 0;
}
