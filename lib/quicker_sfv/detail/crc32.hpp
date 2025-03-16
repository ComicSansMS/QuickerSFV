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
#ifndef INCLUDE_GUARD_QUICKER_SFV_CRC32_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CRC32_HPP

#include <quicker_sfv/hasher.hpp>

#include <cstdint>

namespace quicker_sfv::detail {

/** CRC32 Hasher.
 * Hasher for the CRC32 checksum (CRC-32/ISO-HDLC) algorithm.
 */
class Crc32Hasher: public Hasher {
private:
    uint32_t m_state;
    bool m_useAvx512;
    bool m_useSse42;
public:
    explicit Crc32Hasher(HasherOptions const& opt);
    ~Crc32Hasher() override;
    void addData(std::span<std::byte const> data) override;
    Digest finalize() override;
    void reset() override;
    static Digest digestFromString(std::u8string_view str);
    static Digest digestFromRaw(uint32_t d);
};

}

#endif
