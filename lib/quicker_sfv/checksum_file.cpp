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

#include <quicker_sfv/error.hpp>

#include <algorithm>

namespace quicker_sfv {

[[nodiscard]] std::span<const ChecksumFile::Entry> ChecksumFile::getEntries() const {
    return m_entries;
}

void ChecksumFile::addEntry(std::u8string_view path, Digest digest) {
    std::vector<DataPortion> d = { DataPortion{ .path = std::u8string{ path }, .data_offset = 0, .data_size = -1 } };
    addEntry(std::move(digest), path, std::move(d));
}

void ChecksumFile::addEntry(Digest digest, std::u8string_view display, std::vector<DataPortion> data) {
    if (m_entries.size() >= 4'294'967'295) { throwException(Error::Failed); }
    m_entries.emplace_back(std::u8string{ display }, std::move(digest), std::move(data));
}

void ChecksumFile::sortEntries() {
    std::sort(begin(m_entries), end(m_entries),
        [](Entry const& lhs, Entry const& rhs) -> bool {
            return lhs.display < rhs.display;
        });
}

void ChecksumFile::clear() {
    m_entries.clear();
}


}
