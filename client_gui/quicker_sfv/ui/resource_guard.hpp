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
