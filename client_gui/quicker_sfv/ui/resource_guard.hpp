#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_RESOURCE_GUARD_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_RESOURCE_GUARD_HPP

#include <concepts>

namespace quicker_sfv::gui {

template<typename T, typename Func> requires(std::invocable<Func, T>)
class ResourceGuard {
private:
    T r;
    Func releaseFunc;
public:
    explicit ResourceGuard(T resource, Func release) :r(resource), releaseFunc(release) {}
    ~ResourceGuard() { releaseFunc(r); }
    ResourceGuard& operator=(ResourceGuard&&) = delete;
};

class HandleGuard {
private:
    HANDLE h;
public:
    explicit HandleGuard(HANDLE handle) :h(handle) {}
    ~HandleGuard() { CloseHandle(h); }
    HandleGuard& operator=(HandleGuard&&) = delete;
};

}
#endif
