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
#include <quicker_sfv/line_reader.hpp>
#include <quicker_sfv/string_utilities.hpp>

#include <quicker_sfv/detail/crc32.hpp>

namespace quicker_sfv {

ChecksumProviderPtr createSfvProvider() {
    return ChecksumProviderPtr(new SfvProvider());
}

SfvProvider::SfvProvider() = default;
SfvProvider::~SfvProvider() = default;

ProviderCapabilities SfvProvider::getCapabilities() const noexcept {
    return ProviderCapabilities::Full;
}

std::u8string_view SfvProvider::fileExtensions() const noexcept {
    return u8"*.sfv";
}

std::u8string_view SfvProvider::fileDescription() const noexcept {
    return u8"Sfv File";
}

HasherPtr SfvProvider::createHasher(HasherOptions const& hasher_options) const {
    return HasherPtr(new detail::Crc32Hasher(hasher_options));
}

Digest SfvProvider::digestFromString(std::u8string_view str) const {
    return detail::Crc32Hasher::digestFromString(str);
}

ChecksumFile SfvProvider::readFromFile(FileInput& file_input) const {
    LineReader reader(file_input);
    ChecksumFile ret;
    for (;;) {
        auto opt_line = reader.readLine();
        if (!opt_line) {
            if (reader.done()) {
                break;
            }
        }
        auto line = trim(std::u8string_view{ *opt_line });
        if (line.empty()) { continue; }
        // skip comments
        if (line.starts_with(u8";")) { continue; }

        if (line.size() < 10) { throwException(Error::ParserError); }
        std::size_t const separator_idx = line.size() - 8;
        if ((line[separator_idx - 1] != u8' ')) { throwException(Error::ParserError); }
        std::u8string_view filepath_sv = trim(line.substr(0, separator_idx - 1));
        if (filepath_sv.empty()) { throwException(Error::ParserError); }
        ret.addEntry(filepath_sv, detail::Crc32Hasher::digestFromString(line.substr(separator_idx)));
    }
    return ret;
}

void SfvProvider::writeNewFile(FileOutput& file_output, ChecksumFile const& f) const {
    for (auto const& e : f.getEntries()) {
        std::u8string out_str;
        out_str.reserve(e.path.size() + 11);
        out_str.append(e.path);
        out_str.push_back(u8' ');
        out_str.append(e.digest.toString());
        out_str.push_back(u8'\n');
        size_t const res =file_output.write(
            std::span<std::byte const>(reinterpret_cast<std::byte const*>(out_str.data()), out_str.size()));
        if (res == 0) {
            throwException(Error::FileIO);
        }
    }
}

}
