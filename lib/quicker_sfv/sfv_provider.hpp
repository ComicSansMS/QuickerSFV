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
#ifndef INCLUDE_GUARD_QUICKER_SFV_SFV_PROVIDER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_SFV_PROVIDER_HPP

#include <quicker_sfv/checksum_provider.hpp>
#include <quicker_sfv/checksum_file.hpp>

#include <string_view>
#include <memory>

namespace quicker_sfv {

/** Support for `*.sfv` files.
 * One line per file. Each line ends with a Crc32 checksum, preceded by the relative
 * path of the file.
 * File encoding must be UTF-8. Line endings must be either CRLF or LF on read
 * and will always be LF on write.
 */
class SfvProvider : public ChecksumProvider {
public:
    friend ChecksumProviderPtr createSfvProvider();
private:
    SfvProvider();
public:
    ~SfvProvider() override;
    [[nodiscard]] ProviderCapabilities getCapabilities() const noexcept override;
    [[nodiscard]] std::u8string_view fileExtensions() const noexcept override;
    [[nodiscard]] std::u8string_view fileDescription() const noexcept override;
    [[nodiscard]] HasherPtr createHasher(HasherOptions const& hasher_options) const override;
    [[nodiscard]] Digest digestFromString(std::u8string_view str) const override;

    [[nodiscard]] ChecksumFile readFromFile(FileInput& file_input) const override;
    void writeNewFile(FileOutput& file_output, ChecksumFile const& f) const override;
};

/** Creates an SfvProvider.
 */
ChecksumProviderPtr createSfvProvider();

}

#endif
