#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_PLUGIN_SUPPORT_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_PLUGIN_SUPPORT_HPP
#ifndef QUICKER_SFV_BUILD_SELF_CONTAINED

#include <quicker_sfv/checksum_provider.hpp>
#include <quicker_sfv/plugin/plugin_sdk.h>

namespace quicker_sfv::gui {

QuickerSFV_ChecksumProvider_Callbacks* pluginCallbacks();

ChecksumProviderPtr loadPlugin(QuickerSFV_LoadPluginFunc plugin_load_function);

}

#endif // QUICKER_SFV_BUILD_SELF_CONTAINED
#endif
