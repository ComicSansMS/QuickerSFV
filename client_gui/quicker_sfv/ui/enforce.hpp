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
#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_ENFORCE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_ENFORCE_HPP

namespace quicker_sfv::gui {

/** Enforces a critical condition.
 * This function as similar to an assert in that it terminates the program if the
 * condition does not hold. Unlike an assert, enforce is always checked and thus
 * suitable for enforcing conditions that could lead to bugs or security issues.
 * @param[in] condition The condition to enforce.
 */
void enforce(bool condition);

}

#endif
