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
#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_PLUGIN_SUPPORT_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_PLUGIN_SUPPORT_HPP
#ifndef QUICKER_SFV_BUILD_SELF_CONTAINED

#include <quicker_sfv/checksum_provider.hpp>
#include <quicker_sfv/plugin/plugin_sdk.h>

namespace quicker_sfv::gui {

QuickerSFV_ChecksumProvider_Callbacks* pluginCallbacks();

ChecksumProviderPtr loadPlugin(QuickerSFV_LoadPluginFunc plugin_load_function);

ChecksumProviderPtr loadPluginCpp(QuickerSFV_LoadPluginCppFunc plugin_cpp_load_function);

}

#endif // QUICKER_SFV_BUILD_SELF_CONTAINED
#endif
