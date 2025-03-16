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
#ifndef INCLUDE_GUARD_QUICKER_SFV_PLUGIN_RAR_RAR_PROVIDER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_PLUGIN_RAR_RAR_PROVIDER_HPP

#include <quicker_sfv/rar/rar_export.h>

#include <quicker_sfv/plugin/plugin_sdk.h>

#include <quicker_sfv/checksum_provider.hpp>

namespace quicker_sfv::rar {

class RarProvider final : public ChecksumProvider {
    friend struct RarPluginLoader;
private:
public:
    RarProvider();
    ~RarProvider() override;
    [[nodiscard]] ProviderCapabilities getCapabilities() const noexcept override;
    [[nodiscard]] std::u8string_view fileExtensions() const noexcept override;
    [[nodiscard]] std::u8string_view fileDescription() const noexcept override;
    [[nodiscard]] HasherPtr createHasher(HasherOptions const& hasher_options) const override;
    [[nodiscard]] Digest digestFromString(std::u8string_view str) const override;

    ChecksumFile readFromFile(FileInput& file_input) const override;
    void writeNewFile(FileOutput& file_output, ChecksumFile const& f) const override;
};

}

extern "C" {
QUICKER_SFV_PLUGIN_RAR_EXPORT void QuickerSFV_LoadPlugin_Cpp(QuickerSFV_CppPluginLoader** loader);
}

#endif
