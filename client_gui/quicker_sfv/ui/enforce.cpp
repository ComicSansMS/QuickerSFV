#include <quicker_sfv/ui/enforce.hpp>

#include <Windows.h>

namespace quicker_sfv::gui {

void enforce(bool condition) {
    if (!condition) { ExitProcess(999); }
}

}
