#include <quicker_sfv/ui/enforce.hpp>

#ifdef _WIN32
#   include <Windows.h>
#else
#   include <cstdlib>
#endif

namespace quicker_sfv::gui {

void enforce(bool condition) {
    if (!condition) {
#ifdef _WIN32
        ExitProcess(999);
#else
        std::exit(999);
#endif
    }
}

}
