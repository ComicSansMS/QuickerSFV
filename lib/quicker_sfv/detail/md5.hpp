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
#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_HPP

#include <quicker_sfv/hasher.hpp>

#include <memory>
#include <span>

namespace quicker_sfv::detail {

/** MD5 hasher.
 */
class MD5Hasher: public Hasher {
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> m_impl;
public:
    MD5Hasher();

    ~MD5Hasher() override;
    void addData(std::span<std::byte const> data) override;
    Digest finalize() override;
    void reset() override;
    static Digest digestFromString(std::u8string_view str);
};

}
#endif
